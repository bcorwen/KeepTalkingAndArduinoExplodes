
#include <Wire.h> // Enable this line if using Arduino Uno, Mega, etc.
#include <Adafruit_GFX.h>
#include "Adafruit_LEDBackpack.h"

Adafruit_7segment timerdisp = Adafruit_7segment();

byte gamemode = 0x01;

//Track allocation of pins, as inputs or outputs
byte pinsi[5][4]; // Set max number of modules to 6, max number of pins per module to 5.
byte pinso[5][4]; // Set max number of modules to 6, max number of pins per module to 5.
byte inputstates[5][4]]; //Set array of output pin values to be able to track state changes.
byte inputchanges[5][4]];
enum{UNCHANGED,BUTTONUP,BUTTONDOWN};

// Pins dynamically set counting up from d pin #2, a pin #14
byte thisdpin = 2;
byte thisapin = 14;
byte thismodule = 0;

//long timeleft = 300000;
int gamelength = 300; //seconds
long endmillis
long timeleft;
byte strikenumber = 0;

byte module_button = 1; //Hard-coded number of button modules.
byte module_swires = 0; //Hard-coded number of simple wires modules.

unsigned long lastDebounceTime = 0;  // the last time the output pin was toggled
const byte debounceDelay = 25;
const short buttonpressorhold = 500;

bool buttondown08 = false; //tracking if the button has ever been pressed, therefore logic for played interaction
bool shortpushneeded = true;

//
// Start of functions:
//

void setup() {
  
  Serial.begin(9600);
  initialisehardware(); //initialise hardware

  
}

void loop() {
  // put your main code here, to run repeatedly:
  switch (gamemode) {
    case 0x01: //game running
      gamerunning();
      break;
    default: //stand-by state - stop inputs and hold timer post-game, also generate single round and prep game. 0x00 if completed initialisation, otherwise 0x02 after game has ran.
      gamesetup();
      break;
  }
}


void gamesetup() {
  //gamemode = 0x01;
}

void gamerunning() {

  endmillis = millis()+(long(gamelength)*1000); // Find the time when the timer will expire
  
  do {
    timerupdate();
    //buzzerupdate();

    checkinputs();
    updatestrikes();
    
  } while (timeleft > 0);
  gamemode = 0x00;
}

void timerupdate() {
    //Timer update
    timeleft = (endmillis -millis());
    
    if (timeleft >= 60000) { // Over 1 minute on the clock...
      timerdisp.print(int(timeleft/60000)*100+int((timeleft%60000)/1000));
      if (timeleft < 600000) { // Over 10 minutes...
        timerdisp.writeDigitNum(0,0); // Fill in leading zero
      }
    }
    else if (timeleft<0) { // Timer ran out
      timeleft=0;
      timerdisp.writeDigitNum(0,0);
      timerdisp.writeDigitNum(1,0);
      timerdisp.writeDigitNum(3,0);
      timerdisp.writeDigitNum(4,0);
    }
    else { // Under 1 minute left...
      timerdisp.print(timeleft/10);
      if (timeleft<10000) { // Under 10 seconds...
        timerdisp.writeDigitNum(0,0); // Fill in leading zero
      }
    }
    timerdisp.drawColon(true);
    timerdisp.writeDisplay();
}

void buzzerupdate() {
    //Buzzer
    if ((timeleft%1000)<100) {
      tone(pintable[0][1],800);
    }
    else if ((timeleft%1000)>=700 && (timeleft%1000)<800) {
      tone(pintable[0][1],400);
    }
    else {
      noTone(pintable[0][1]);
    }
}

void checkinputs() {
  // read the input state and state changes of all buttons
  static unsigned long lastButtonTime; // time stamp for remembering the time when the button states were last read
  memset(buttonChange,0,sizeof(buttonChange)); // reset all old state changes
  if (millis()-lastButtonTime<BOUNCETIME) return;  // within bounce time: leave the function
  lastButtonTime=millis(); // remember the current time
  for (int i=0;i<NUMBUTTONS;i++)
  {
    byte curState=digitalRead(buttonPins[i]);        // current button state
    if (INPUTMODE==INPUT_PULLUP) curState=!curState; // logic is inverted with INPUT_PULLUP
    if (curState!=buttonState[i])                    // state change detected
    {
      if (curState==HIGH) buttonChange[i]=BUTTONDOWN;
      else buttonChange[i]=BUTTONUP;
    }
    buttonState[i]=curState;  // save the current button state
  }
}

void checkinputs_SHITTY() {
  //Hard-code looking at pin 8 for the button
  bool thisread = digitalRead(8);
  //Hard-code requiring a short press
  shortpushneeded = true;  
  
  if (thisread != buttonstate08) {
    lastDebounceTime = millis();
    buttonstate08 = thisread;
  }

  if ((millis() - lastDebounceTime) > buttonpressorhold) {
    if ((buttonstate08 == LOW) && (buttondown08 == true)) {
      //button has been pressed long enough to be considered a hold
      if (shortpushneeded == false) {
        //correct! now light LED...
      }
      else {
        //wrong! add a strike...
        strikenumber ++;
      }
    }
    else if ((buttonstate08 == HIGH) && (buttondown08 == true)) {
      //button has been held and now released
      if (shortpushneeded == false) {
        //check timer to see if matching number showing
        //timeleft...
      }
      else {
        //wrong, reset button
        buttondown08 = false;
      }
    }
  }
  else if ((millis() - lastDebounceTime) > debounceDelay) {
    // whatever the reading is at, it's been there for longer than the debounce
    // delay, so take it as the actual current state:

    // if the button state has changed:
    if ((buttonstate08 == LOW) && (buttondown08 == false)) {
      //button first pressed
      buttondown08 = true;
    }
    else if ((buttonstate08 == HIGH) && (buttondown08 == true)) {
      //button released before being considered a hold
      if (shortpushneeded == true) {
        //correct! module done
      }
      else {
        //wrong! button released too quickly to be considered a hold, add strike...
        strikenumber ++;
        buttondown08 = false;
      }
    }
  }  
}

void updatestrikes() {
  
}

void initialisehardware() {
  // Bomb needs to identify the modules connected so it can generate scenario and interact with the connect devices.

  // Timer:
  for (byte j = 0; j < 4; j++) { // 4x output [Strike LED #1, Strike LED #2, Success LED, Pizeo buzzer].
    assigndo(thismodule,j);
  }
  thismodule++;

  timerdisp.begin(0x70); //Timer uses I2C pins
  timerdisp.print(10000);
  timerdisp.writeDisplay();

  // Button:
  for (byte i = 0; i < module_button; i++) {
    for (byte j = 0; j < 1; j++) { // 1x input [Button]
      assigndi(i,j);
    }
    for (byte j = 0; j < 5; j++) { // 5x output [Success LED R, Success LED G, RGB LED R, RGB LED G, RGB LED B]
      assigndo(i,j);
    }
    thismodule ++;
  }

  // Simple wires:
  for (byte i = 0; i < module_swires; i++) {
    for (byte j = 0; j < 1; j++) { // 1x input [Analog mix of all 6 wires]
      assignai(i,j)
    }
    for (byte j = 0; j < 2; j++) { // 2x output [Success LED R, Success LED G]
      assigndo(i,j);
    }
  }
}

void assigndi(moduleno, pinno) {
  pinsi[moduleno][pinno] = thisdpin;
  pinMode(pinsi[moduleno][pinno], INPUT_PULLUP);
  thisdpin++;
}

void assigndo(moduleno, pinno) {
  pinso[moduleno][pinno] = thisdpin;
  pinsostate[moduleno][pinno] = LOW;
  pinMode(pinso[moduleno][pinno], OUTPUT);
  digitalWrite(pinso[moduleno][pinno], LOW);
  thisdpin++;
}

void assignai(moduleno, pinno) {
  pinsi[moduleno][pinno] = thisapin;
  pinMode(pinsi[moduleno][pinno], INPUT);
  thisapin++;
}
