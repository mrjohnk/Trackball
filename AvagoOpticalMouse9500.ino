#include <SPI.h>
#include <avr/pgmspace.h>

// Registers
#define REG_Product_ID                           0x00
#define REG_Revision_ID                          0x01
#define REG_Motion                               0x02
#define REG_Delta_X_L                            0x03
#define REG_Delta_X_H                            0x04
#define REG_Delta_Y_L                            0x05
#define REG_Delta_Y_H                            0x06
#define REG_SQUAL                                0x07
#define REG_Pixel_Sum                            0x08
#define REG_Maximum_Pixel                        0x09
#define REG_Minimum_Pixel                        0x0a
#define REG_Shutter_Lower                        0x0b
#define REG_Shutter_Upper                        0x0c
#define REG_Frame_Period_Lower                   0x0d
#define REG_Frame_Period_Upper                   0x0e
#define REG_Configuration_I                      0x0f
#define REG_Configuration_II                     0x10
#define REG_Frame_Capture                        0x12
#define REG_SROM_Enable                          0x13
#define REG_Run_Downshift                        0x14
#define REG_Rest1_Rate                           0x15
#define REG_Rest1_Downshift                      0x16
#define REG_Rest2_Rate                           0x17
#define REG_Rest2_Downshift                      0x18
#define REG_Rest3_Rate                           0x19
#define REG_Frame_Period_Max_Bound_Lower         0x1a
#define REG_Frame_Period_Max_Bound_Upper         0x1b
#define REG_Frame_Period_Min_Bound_Lower         0x1c
#define REG_Frame_Period_Min_Bound_Upper         0x1d
#define REG_Shutter_Max_Bound_Lower              0x1e
#define REG_Shutter_Max_Bound_Upper              0x1f
#define REG_LASER_CTRL0                          0x20
#define REG_Observation                          0x24
#define REG_Data_Out_Lower                       0x25
#define REG_Data_Out_Upper                       0x26
#define REG_SROM_ID                              0x2a
#define REG_Lift_Detection_Thr                   0x2e
#define REG_Configuration_V                      0x2f
#define REG_Configuration_IV                     0x39
#define REG_Power_Up_Reset                       0x3a
#define REG_Shutdown                             0x3b
#define REG_Inverse_Product_ID                   0x3f
#define REG_Motion_Burst                         0x50
#define REG_SROM_Load_Burst                      0x62
#define REG_Pixel_Burst                          0x64

const int ncs = 0;
const int redLED = 4;
const int greenLED = 9;
const int blueLED = 10;
const int LEDintensity = 55;

// Declare variables & set initial values
const int buttonAvgSamples = 12;
int initComplete = 0;
int currResolution=4;
int buttonstatus[25];
int bptr[25];
int bCapAvg[25][buttonAvgSamples];
byte kbpress[25] = {0}; //indicates what pin has a key pressed (index) and what button slot it is using (value).
byte kbptr[7]={0}; //keeps track which keyboard button slots are used/available (index) and what key (value)
byte activeButtonPtr=0; //Counts how many active mapped buttons we have
byte abuttons[25];  //lists which pins have active buttons mapped to them
byte mbuttons[6];  //tracks status of standard mouse buttons
unsigned long currTimestamp; //used to store current timstamp for timing purposes
unsigned long timestamps[25];  //tracks next check of this pin for polling timing

extern const String keymap[25];
extern const unsigned long keypoll[25];
extern const unsigned short firmware_length;
extern prog_uchar firmware_data[];

void setup() {
  Serial.begin(38400);

  attachInterrupt(0, UpdatePointer, FALLING);
  
  SPI.begin();
  SPI.setDataMode(SPI_MODE3);
  SPI.setBitOrder(MSBFIRST);
  SPI.setClockDivider(4);
}

void adns_com_begin(){
  digitalWrite(ncs, LOW);
}

void adns_com_end(){
  digitalWrite(ncs, HIGH);
}

byte adns_read_reg(byte reg_addr){
  adns_com_begin();
  
  // send adress of the register, with MSBit = 0 to indicate it's a read
  SPI.transfer(reg_addr & 0x7f );
  delayMicroseconds(100); // tSRAD
  // read data
  byte data = SPI.transfer(0);
  
  delayMicroseconds(1); // tSCLK-NCS for read operation is 120ns
  adns_com_end();
  delayMicroseconds(19); //  tSRW/tSRR (=20us) minus tSCLK-NCS

  return data;
}

void adns_write_reg(byte reg_addr, byte data){
  adns_com_begin();
  
  //send adress of the register, with MSBit = 1 to indicate it's a write
  SPI.transfer(reg_addr | 0x80 );
  //sent data
  SPI.transfer(data);
  
  delayMicroseconds(20); // tSCLK-NCS for write operation
  adns_com_end();
  delayMicroseconds(100); // tSWW/tSWR (=120us) minus tSCLK-NCS. Could be shortened, but is looks like a safe lower bound 
}

void adns_upload_firmware(){
  // send the firmware to the chip, cf p.18 of the datasheet
  Serial.println("Uploading firmware...");
  // set the configuration_IV register in 3k firmware mode
  adns_write_reg(REG_Configuration_IV, 0x02); // bit 1 = 1 for 3k mode, other bits are reserved 
  
  // write 0x1d in SROM_enable reg for initializing
  adns_write_reg(REG_SROM_Enable, 0x1d); 
  
  // wait for more than one frame period
  delay(10); // assume that the frame rate is as low as 100fps... even if it should never be that low
  
  // write 0x18 to SROM_enable to start SROM download
  adns_write_reg(REG_SROM_Enable, 0x18); 
  
  // write the SROM file (=firmware data) 
  adns_com_begin();
  SPI.transfer(REG_SROM_Load_Burst | 0x80); // write burst destination adress
  delayMicroseconds(15);
  
  // send all bytes of the firmware
  unsigned char c;
  for(int i = 0; i < firmware_length; i++){ 
    c = (unsigned char)pgm_read_byte(firmware_data + i);
    SPI.transfer(c);
    delayMicroseconds(15);
  }
  adns_com_end();
  }


void performStartup(void){
  adns_com_end(); // ensure that the serial port is reset
  adns_com_begin(); // ensure that the serial port is reset
  adns_com_end(); // ensure that the serial port is reset
  adns_write_reg(REG_Power_Up_Reset, 0x5a); // force reset
  delay(50); // wait for it to reboot
  // read registers 0x02 to 0x06 (and discard the data)
  adns_read_reg(REG_Motion);
  adns_read_reg(REG_Delta_X_L);
  adns_read_reg(REG_Delta_X_H);
  adns_read_reg(REG_Delta_Y_L);
  adns_read_reg(REG_Delta_Y_H);
  // upload the firmware
  adns_upload_firmware();
  delay(10);
  //enable laser(bit 0 = 0b), in normal mode (bits 3,2,1 = 000b)
  // reading the actual value of the register is important because the real
  // default value is different from what is said in the datasheet, and if you
  // change the reserved bytes (like by writing 0x00...) it would not work.
  byte laser_ctrl0 = adns_read_reg(REG_LASER_CTRL0);
  adns_write_reg(REG_LASER_CTRL0, laser_ctrl0 & 0xf0 );
  
  delay(1);

  Serial.println("Optical Chip Initialized");
  }

void dispRegisters(void){
  int oreg[7] = {
    0x00,0x3F,0x2A,0x02  };
  char* oregname[] = {
    "Product_ID","Inverse_Product_ID","SROM_Version","Motion"  };
  byte regres;

  digitalWrite(ncs,LOW);

  int rctr=0;
  for(rctr=0; rctr<4; rctr++){
    SPI.transfer(oreg[rctr]);
    delay(1);
    Serial.println("---");
    Serial.println(oregname[rctr]);
    Serial.println(oreg[rctr],HEX);
    regres = SPI.transfer(0);
    Serial.println(regres,BIN);  
    Serial.println(regres,HEX);  
    delay(1);
  }
  digitalWrite(ncs,HIGH);
}

int readXY(int *xy){
  digitalWrite(ncs,LOW);
  
  xy[0] = (int)adns_read_reg(REG_Delta_X_L);
  xy[1] = (int)adns_read_reg(REG_Delta_Y_L);

  //Convert from 2's complement
  if(xy[1] & 0x80){
    xy[1] = -1 * ((xy[1] ^ 0xff) + 1);
    }
  if(xy[0] & 0x80){
    xy[0] = -1 * ((xy[0] ^ 0xff) + 1);
    }
  xy[1] = xy[1] * -1;
  digitalWrite(ncs,HIGH);     
  } 

void chResolution(byte newRes){
  digitalWrite(ncs,LOW);
  adns_write_reg(REG_Configuration_I, newRes);
  digitalWrite(ncs,HIGH);
  }

void changeResolution(void){
  switch (currResolution){
  case 0:
    currResolution = 1;
    analogWrite(greenLED,LEDintensity);
    analogWrite(redLED,0);
    analogWrite(blueLED,0);
    chResolution(0x19);
    break;
  case 1:
    currResolution = 2;
    analogWrite(greenLED,0);
    analogWrite(redLED,LEDintensity);
    analogWrite(blueLED,0);
    chResolution(0x25);
    break;
  case 2:
    currResolution = 3;
    analogWrite(greenLED,LEDintensity);
    analogWrite(redLED,0);
    analogWrite(blueLED,LEDintensity/3);
    chResolution(0x32);
    break;
  case 3:
    currResolution = 4;
    analogWrite(greenLED,0);      
    analogWrite(redLED,LEDintensity/2);
    analogWrite(blueLED,LEDintensity);
    chResolution(0x3F);
    break;
  case 4:
    currResolution = 0;
    analogWrite(greenLED,0);
    analogWrite(redLED,0);
    analogWrite(blueLED,LEDintensity);
    chResolution(0xC);
    break;
    }

  }
  
void UpdatePointer(void){
  if(initComplete==9){
    int xydat[2];
    readXY(&xydat[0]);
    Mouse.move(xydat[0],xydat[1]);
    }
  }

int getButtonStat(int b){
  int i=0;
  int riseRead=0;
  int riseAvg = 0;
  cli();  //disable interrupts for more accurate read
  digitalWrite(b,HIGH);
  for(i=0; i<2; i++){
    riseRead += analogRead(b);
    }
  riseAvg = riseRead/2;
  digitalWrite(b,LOW);
  sei(); //re-enable interrupts
  
  //if(b==14){riseAvg-=12;} //pin 14 needs a slight offset to work properly
  //if(b==12){Serial.println(riseAvg);}
  return(riseAvg);
  }

int getButtonStat2(int b){
  cli();  //disable interrupts for more accurate read
  digitalWrite(b,HIGH);
  int capCharge = analogRead(b);
  digitalWrite(b,LOW);
  sei(); //re-enable interrupts
  
  //if(b==14){riseAvg-=12;} //pin 14 needs a slight offset to work properly
  //if(b==12){Serial.println(capCharge);}
  return(capCharge);
  }

byte readStatus2(int b){
  timestamps[b] = currTimestamp + keypoll[b];
  if(getButtonStat2(b) > 980){
    return(0);
    }
  else {
    //Serial.println(b);
    return(1);
    }

  }

byte readStatus(int b){
  int butsum=0;
  int avgPtr=0;
    
  if (bptr[b]>buttonAvgSamples){
    bptr[b]=0;
    }
  if ((getButtonStat(b)) < 1014){
    bCapAvg[b][bptr[b]] = 110;
    }
  else{
    bCapAvg[b][bptr[b]] = -15;
    }

  for(avgPtr=0;avgPtr<buttonAvgSamples;avgPtr++){
    butsum += bCapAvg[b][avgPtr];
    }
  bptr[b]++;

  timestamps[b] = currTimestamp + keypoll[b];

  if(butsum / buttonAvgSamples > 0){
    return 1;
    }
  else{
    return 0;
    }
  }

void setActiveButtons(void){
  int bptr=0;
  Serial.println("determining active mapped pins");
  for(bptr=0;bptr<25;bptr++){
    if(keymap[bptr] != "X" && keymap[bptr] != "system"){
      Serial.println(keymap[bptr]);
      activeButtonPtr++;
      abuttons[activeButtonPtr] = bptr;
      Serial.println(abuttons[activeButtonPtr]);
      Serial.println("---");
      }  
    }
  }

void pushKey(byte ctr, byte num){
   switch(ctr) {
     case 1:
       Keyboard.set_key1(num);
       break;
     case 2:
       Keyboard.set_key2(num);
       break;
     case 3:
       Keyboard.set_key3(num);
       break;
     case 4:
       Keyboard.set_key4(num);
       break;
     case 5:
       Keyboard.set_key5(num);
       break;
     case 6:
       Keyboard.set_key6(num);
       break;       
     }
  }


void changeKey(byte stat, byte pin){
  /*
  byte kbpress[25] //indicates what pin has a key pressed (index) and what button slot it is using (value).
  byte kbptr[7] //keeps track which button slots are used/available (index) and what key (value)
  numb = numeric version of the key to push

   */
  byte num=0;

  String keystr=keymap[pin];
  if(stat==1){  
    if(keystr.startsWith("-")){      
      Keyboard.set_modifier(MODIFIERKEY_SHIFT);
      keystr=keystr.substring(1,keystr.length());
      }
    char buf[keystr.length()+1];
    keystr.toCharArray(buf,keystr.length()+1);
    num=atoi(buf);
 
    //find and available key slot to assign this letter to
    int ctr=0;
    for(ctr=1;ctr<=6;ctr++){
      if(kbptr[ctr] == 0){
        kbptr[ctr] = num;
        kbpress[pin] = ctr;
        break;
        }
      }

    pushKey(ctr,num);
    Keyboard.send_now();
    Keyboard.set_modifier(0);
    }
  else{
    pushKey(kbpress[pin],0);
    Keyboard.send_now();
    kbptr[kbpress[pin]]=0;
    kbpress[pin]=0;
    }
  }

void loop() {

  performStartup();  
  Serial.println("Initialization Complete, starting main loop");
  initComplete=9;
  dispRegisters();
  changeResolution();
  setActiveButtons();
  int bchk=0;
  
  while(1){

    currTimestamp = millis();
    for(bchk=1;bchk<=activeButtonPtr;bchk++){
      if( timestamps[abuttons[bchk]] < currTimestamp){
        byte newbstat = readStatus2(abuttons[bchk]);
        if(newbstat != buttonstatus[abuttons[bchk]]){
          buttonstatus[abuttons[bchk]] = newbstat;
          if(keymap[abuttons[bchk]] == "M1"){
              mbuttons[1]=newbstat;
              Mouse.set_buttons(mbuttons[1],mbuttons[2],mbuttons[3]);
              }
           else if(keymap[abuttons[bchk]] == "M2"){
              mbuttons[2]=newbstat;
              Mouse.set_buttons(mbuttons[1],mbuttons[2],mbuttons[3]);
              }
           else if(keymap[abuttons[bchk]] == "M3"){
              mbuttons[3]=newbstat;
              Mouse.set_buttons(mbuttons[1],mbuttons[2],mbuttons[3]);
              }
           else if(keymap[abuttons[bchk]] == "S1"){
              buttonstatus[abuttons[bchk]] = 2;
              Mouse.scroll(newbstat);
              }
           else if(keymap[abuttons[bchk]] == "S2"){
              buttonstatus[abuttons[bchk]] = 2;
              Mouse.scroll( (newbstat * -1) );
              }
           else if(keymap[abuttons[bchk]] == "R1"){
              if(newbstat==0){
                changeResolution();
                }
              }
           else { //<--Must be a keystroke.  No starting delimiter to match
              changeKey(newbstat,abuttons[bchk]);
              }  

            }
          }
        }

  }
}


