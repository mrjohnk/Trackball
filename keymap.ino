/************************************************************
"system"   reserved and unavailable for assignment
"X"        available for assignment, but currently unassigned
"M[x]"     mouse buttons.  [x] is replaced by which one.
"R1"       resolution selection button
"S[x]"     scroll buttons. (1=up, 2=down)
KEY_[x]    keyboard strokes.  Uses standard Teensyduino key map.  Note: No quote marks.
KEY_[x]*-1 multiply by -1 to use <SHIFT> modifier.  Letters are lowercase by default without modifier.

Pollmap values refer to length of time between polls in milliseconds.
Shorter times get higher priority and are checked more often. 

Advice:
Polling not often enough causes slow/sluggish/missed response.

CPU resources are limited.  Polling many buttons frequently
can cause slow/sluggish/missed responses for all buttons.  
Low priority buttons should be set with higher poll delay times.

Mark all unused pins with "X" to disable polling on this pin,
conserve CPU resources and enhance detection precision on other pins.

*************************************************************/


const String keymap[25] = {
"system",   //Pin 0 "system - SS"
"system",   //Pin 1 "system - SCLK"
"system",   //Pin 2 "system - MOSI"
"system",   //Pin 3 "system - MISO"
"system",   //Pin 4 "system - DIGITAL/PWM/RED LED"
"system",   //Pin 5 "system - DIGITAL/OPTICAL INTERRUPT-0"
"system",   //Pin 6 "system - DIGITAL/INTERRUPT-1"
"system",   //Pin 7 "system - DIGITAL/INTERRUPT-2"
"system",   //Pin 8 "system - DIGITAL/INTERRUPT-3"
"system",   //Pin 9  "system - DIGITAL/PWM/GREEN LED"
"system",   //Pin 10 "system - DIGITAL/PWM/BLUE LED"
"system",   //Pin 11 "system - ANALOG/AMBER LED (ON-BOARD)"
KEY_F5,        //Pin 12 "programmable button"
KEY_F6,        //Pin 13 "programmable button"
KEY_F7,        //Pin 14 "programmable button"
KEY_F8,      //Pin 15 "programmable button"
"S2",       //Pin 16 "programmable button"
"S1",       //Pin 17 "programmable button"
"M3",       //Pin 18 "programmable button"
"M2",       //Pin 19 "programmable button"
"M1",       //Pin 20 "programmable button"
"R1",       //Pin 21 "programmable button"
KEY_F9,        //Pin 22 "programmable button"
"system",        //Pin 23  "system - DIGITAL"
"system",        //Pin 24  "system - DIGITAL"
};

const unsigned long keypoll[25] = {
0,    //Pin 0 "system"
0,    //Pin 1 "system"
0,    //Pin 2 "system"
0,    //Pin 3 "system"
0,    //Pin 4 "system"
0,    //Pin 5 "system"
0,    //Pin 6 "system"
0,    //Pin 7 "system"
0,    //Pin 8 "system"
0,    //Pin 9 "system"
0,    //Pin 10 "system"
0,    //Pin 11 "system"
25,    //Pin 12 "programmable button"
10,    //Pin 13 "programmable button"
10,    //Pin 14 "programmable button"
10,    //Pin 15 "programmable button"
30,   //Pin 16 "programmable button"
30,   //Pin 17 "programmable button"
10,    //Pin 18 "programmable button"
10,    //Pin 19 "programmable button"
10,    //Pin 20 "programmable button"
10,   //Pin 21 "programmable button"
10,    //Pin 22 "programmable button"
10,    //Pin 23  "system"
10,    //Pin 24  "system"
};


