/*
Copyright (c) 2008-2009, Luis Quesada - https://github.com/lquesada , and
                         Manuel Martín - https://github.com/DraXus
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * The name of its contributors may not be used to endorse or promote
      products derived from this software without specific prior written
      permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#define TIME 0x35

#ifdef SDCC
#include <pic16f628a.h>

unsigned int __at 0x2007 CONFIG = _CP_OFF&_BOREN_ON&_DATA_CP_OFF&_PWRTE_ON&_WDT_ON&_LVP_OFF&_MCLRE_OFF&_INTRC_OSC_NOCLKOUT;
#define config() {CMCON = 0x07;TRISB = 0x00;TRISA = 0xFF;} // Configure ports
#define configtimer() {T0CS = 0;PSA = 0;PS2 = 1;PS1 = 1;PS0 = 1;INTCON = 0;GIE = 1;T0IE = 1;resettimer();}
#define resettimer() { TMR0 = TIME; T0IF = 0; }
#define read() in = PORTA^0xFF; // Read to in
#define write() PORTB = out; // Write to out
#define clrwdt() __asm clrwdt __endasm;

#else

#include <16f84a.h>

#define ROBOT

#fuses WDT
#byte port_a = 5
#byte port_b = 6
#ifdef ROBOT
#define config() {set_tris_a(0x00);set_tris_b(0xFF);}
#define read() in = port_b^0xFF; // Read to in
#define write() port_a = out; // Write to out
#else
#define config() {set_tris_a(0xFF);set_tris_b(0x00);}
#define read() in = port_a^0xFF; // Read to in
#define write() port_b = out; // Write to out
#endif

#byte STATUS=0x03
#byte TMR0=0x01
#byte INTCON=0x0B
#byte OPTION_REG=0x01
#define RP0 05

#inline
configtimer(){
#asm
bsf STATUS,RP0
movlw 0x07
movwf OPTION_REG
bcf STATUS,RP0
movlw 0xA0
movwf INTCON
#endasm
//enable_interrupts(INT_TIMER0);
//enable_interrupts(GLOBAL);
}

resettimer(){
#asm
bcf STATUS,RP0
movlw TIME
movwf TMR0
#endasm
//set_rtcc(128);
}
#inline
clrwdt(){
#asm
clrwdt
#endasm
}

#endif


volatile unsigned char in;
volatile unsigned char out;
volatile unsigned char aux;
volatile unsigned char auxd;
volatile unsigned long auxl;

#define clear() out = 0x00; // Clear out
#define setall() out = 0xFF; // Sets every bit in out
#define setto(BITS) out = BITS;

#define setbits(BITS) out = out|BITS;
#define unsetbits(BITS) out = out&(0xFF^BITS);

#define setbit(BIT) out = out|(0x01<<BIT); //0-7
#define unsetbit(BIT) out = out&((0x01<<BIT)^0xFF); //0-7
#define unsetallsetbit(BIT) out = 0x01<<BIT; //0-7
#define setallunsetbit(BIT) out = 0xFF^(0x01<<BIT); //0-7

#define testbit(BIT) (in&(0x01<<BIT)) //0-7
#define waitfortrue(BIT) while (!testbit(BIT)) { clrwdt(); read(); }
#define waitforfalse(BIT) while (testbit(BIT)) { clrwdt(); read(); }
void wait(unsigned long time) { // Wait time miliseconds
  while(time--) {
    clrwdt();
    for (auxd=0;auxd<51;auxd++) {
    }
  }
}


//------------------------
volatile unsigned char walkspeed;	// Walk speed
volatile unsigned char walklimit;	// Walk speed limit
volatile unsigned char walkcounter;     // Walk speed counter
volatile unsigned char walkdirection;   // Walk direction
#define forward() walkdirection = 0x09; // 0101 0110 1001
#define backward() walkdirection = 0x06; // 1010 1001 0110
#define right() walkdirection = 0x05; // 0110 0101 1010
#define left() walkdirection = 0x0A; // 1001 1010 0101
#define stop() walkdirection = 0x00; // 0000
#define setspeed(SPEED) walkspeed = SPEED;
#define setlimit(LIM) {walklimit = LIM;walkcounter=LIM;}
void motor() {
  if (walkcounter == 0) {
    walkcounter=walklimit;
  }
  if (walkcounter < walkspeed) {
    out = walkdirection;
  }
  else {
    out = out&0xF0;
  }
  walkcounter--;
}
//------------------------

//------------------------
volatile unsigned char timeraux;
volatile unsigned long timer;

#define inittimer() {timeraux = 0;timer = 0;resettimer();configtimer();}
#define gettimer() timer

#ifdef SDCC
void isr() __interrupt 0 {
#else
#INT_TIMER0
isr() {
#endif
  clrwdt();
  timeraux++;
  if (timeraux >= 3) {
    timeraux = 0;
    timer++;
  }
  resettimer();
}
//------------------------


//------------------------
//#define sensors() (!testbit(1) || !testbit(2))
#define sensors() (!testbit(1))
//------------------------

//------------------------
volatile unsigned long random;
#define rand(n) (random = (random*17)+13)%n;
//------------------------

//------------------------
#define setbuzzer() unsetbit(4);
#define unsetbuzzer() setbit(4);
//------------------------


volatile unsigned long turntime;
volatile unsigned long waittime;
volatile unsigned int i;
void main() {

  // INITIALIZATION
  config();                    // CONFIGURES THE MICROCONTROLLER
  inittimer();                 // INITIALIZES THE TIMER SYSTEM
  turntime = 0;                // DISABLE TURN STATE
  waittime = 0;                // DISABLE WAIT STATE
                               // DEFAULT STATE IS ENABLED
  setlimit(20);                // SPEED LIMIT = 20
  stop();                      // STARTS STOPPED
  motor();

  // STARTUP NOISE
  for (i = 200;i>0;i = i-4) {
    setbuzzer();
    write();
    wait(i);
    unsetbuzzer();
    write();
    wait(i);
  }

  // MAIN LOOP
  while(1) {

    // READ
    clrwdt();                      // CLEAR WATCHDOG
    read();                        // READ
    clear();                       // CLEAR OUTPUT
    unsetbuzzer();		   // STOP BUZZER

    // WAIT STATE
    if (waittime > timer) {
      setbit(7);                   // SHOW STATE = 2
      backward();                  // WALK BACKWARDS
      turntime = timer+10+rand(6); // TURN FOR 1.3s +- 0.3s
    }

    // TURN STATE
    else if (turntime>timer) {
      setbit(6);                   // SHOW STATE = 1
      left();                      // TURN LEFT
      setspeed(20);                // SPEED = 20
    }

    // DEFAULT STATE
    else {
      setbit(5);                   // SHOW STATE 0 0
      forward();                   // WALK FORWARD
      setspeed(20);                // SPEED = 20
      if (sensors())               // IF SENSORS DETECT END OF BOARD
        {waittime = timer+2;}      // SET WAIT STATE FOR 0.2s, THEN TURN.
    }

    // WRITE
    rand(1);                       // CYCLES RANDOM GENERATOR
    motor();                       // RUN MOTOR INFORMATION
    write();                       // WRITES OUTPUT

  }

}
