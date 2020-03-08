//======================================================================
//
//  Keep Talking and Arduino Explodes!
//
//======================================================================
//
//  version 0.3.0
//
//  Goal for this version: Complete Game Master, enabling tracking of
//    module successes, strikes and game win/lose conditions.
//
//======================================================================
//
//  Modules supported: Button, Simple wires.
//
//======================================================================

//**********************************************************************
// LIBRARIES
//**********************************************************************

#include <Wire.h> // Enable this line if using Arduino Uno, Mega, etc.
#include <Adafruit_GFX.h>
#include <Adafruit_LEDBackpack.h>
#include <Entropy.h>
#include <LiquidCrystal595.h>

//**********************************************************************
// GLOBAL VARIABLES
//**********************************************************************

// Temp pin variables:
byte pin_buzzer = 2;
byte pin_lcd_d = 3;
byte pin_lcd_l = 4;
byte pin_lcd_c = 5;
//byte pin_button_button = 31;
//byte pin_button_led_r = 29;
//byte pin_button_led_g = 27;
//byte pin_button_led_b = 25;
//byte pin_swires_d = 53;
//byte pin_swires_l = 51;
//byte pin_swires_c = 49;
byte ind_car_led = 7;
byte ind_frk_led = 8;

Adafruit_7segment timerdisp = Adafruit_7segment();
LiquidCrystal595 lcd(pin_lcd_d, pin_lcd_l, pin_lcd_c); // Data pin, latch pin, clock pin

byte gamemode = 0;

/*
  //Track allocation of pins, as inputs or outputs
  byte pinsi[5][4]; // Set max number of modules to 6, max number of pins per module to 5.
  byte pinso[5][4]; // Set max number of modules to 6, max number of pins per module to 5.
  byte inputstates[5][4]; //Set array of output pin values to be able to track state changes.
  byte inputchanges[5][4];

  // Pins dynamically set counting up from d pin #2, a pin #14
  byte thisdpin = 2;
  byte thisapin = 14;
  byte thismodule = 0;
*/

// Module Game Master array - holds data for each module to be referenced/passed in functions
byte module_master[6][8]; // First row reserved for timer/misc
// First dimension: maximum number of modules, excluding timer
// Second dimension: maximum number of arguments to define a module
//
// Module codes:      0         1        2          3
//                No module   Timer    Button  Simple wires
//
//                         i =  0           1         2         3           4               5               6              7
// Timer layout         = {module type,   buzzer , lcd data, lcd ltch,  lcd clock,     ind_car_led,     ind_frk_led}
// Button layout        = {module type, completed, strike R, strike G, button input,      LED R,          LED G,         LED B}
// Simple wires layout  = {module type, completed, strike R, strike G, register data, register latch,  register clock}
// Keypad layout        = {module type, completed, strike R, strike G,  button 1,        button 2,       button 3,      button 4}
// Simon says layout    = {module type, completed, strike R, strike G,  button R,        button Y,       button G,      button B}
//

long timeleft;
long gamelength = 300; //seconds
long thismillis;
long delta_t;
char timestr[5] = "----";
byte strikenumber = 0;
byte strikelimit = 3;
char sec_tick_over;
long buzz_timer;
byte time_scale; // Quadruple the time scale: 4 = 1x speed (normal), 5 = 1.25x speed (1 strike), etc...
long blinktime;
long blinkperiod = 250; // Milliseconds
bool blinkbool;
long buzzerinterrupt;

//byte module_button = 1; //Hard-coded number of button modules.
//byte module_swires = 1; //Hard-coded number of simple wires modules.

long lastDebounceTime = 0;  // the last time the output pin was toggled

bool track_button_state = HIGH;
bool track_button_interaction = false; //tracking if the button has ever been pressed, therefore logic for played interaction
bool shortpushneeded;
bool isthisapress;
bool button_trigger;

byte button_colour;
byte button_label;
byte strip_colour;
String button_labels[4] = {"Abort", "Detonate", "Hold", "Press"};
String button_colours[6] = {"Blue", "White", "Yellow", "Red", "Black", "---"}; //Black included so simple wires can use this array too. Button should not call black.
char button_colours_s[6] = {'B', 'W', 'Y', 'R', 'K', '_'};
String module_labels[6] = {"Missingno", "Time", "Button", "S Wires", "Keypad", "Simon"};

long wrong_flash_time;

byte track_wire_state = B00111111;
byte register_inputs;
byte wire_colours[6] = {5, 5, 5, 5, 5, 5}; //5 equates to no wire present. 0-4 are colours defined above.
byte wire_no;
byte wire_select = B00111111;
byte correct_wire;

char serial_number[7];
bool serial_vowel;
bool serial_odd;
bool serial_even;

byte battery_number;
bool ind_frk;
bool ind_car;

//**********************************************************************
// FUNCTIONS: Main
//**********************************************************************

void setup() {
  Serial.begin(9600);
  Entropy.initialize();
  randomSeed(Entropy.random());

  initialisehardware(); //initialise hardware
  Serial.println(F("Initialisation done!"));
}

void loop() {
  // put your main code here, to run repeatedly:
  switch (gamemode) {
    case 1: //game running
      Serial.println(F("Game starting!"));
      gamerunning();
      break;
    case 2: //also stand-by post-game
      gamesetup();
      break;
    default: //stand-by state - stop inputs and hold timer post-game, also generate single round and prep game. 0 if completed initialisation, otherwise 2 after game has ran.
      Serial.println(F("Building new game:"));
      gamesetup();
      break;
  }
}

//**********************************************************************
// FUNCTIONS: Game Managers
//**********************************************************************

void gamesetup() {
  if (gamemode == 2) {

  } else if (gamemode == 0) {
    general_setup();
    button_setup();
    Serial.println(F("Button setup done..."));
    simple_wires_setup();
    Serial.println(F("Simple wires setup done..."));

    char timestr[5] = "0500";
    timerdisp.writeDigitNum(0, int(timestr[0] - 48));
    timerdisp.writeDigitNum(1, int(timestr[1] - 48));
    timerdisp.writeDigitNum(3, int(timestr[2] - 48));
    timerdisp.writeDigitNum(4, int(timestr[3] - 48));
    timerdisp.drawColon(true);
    timerdisp.writeDisplay();

    digitalWrite(module_master[1][2], LOW);
    digitalWrite(module_master[2][2], LOW);
    digitalWrite(module_master[1][3], LOW);
    digitalWrite(module_master[2][3], LOW);

    place_button();
    place_swires();

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(F("Serial #: "));
    lcd.print(serial_number);
    lcd.setCursor(0, 1);
    lcd.print(F("Battery #: "));
    lcd.print(battery_number);

    track_button_state = HIGH; //Button starts out not pressed
    track_button_interaction = false; //Button was never interacted with
    lastDebounceTime = millis();

    module_master[1][1]=0;
    module_master[2][1]=0;

    strikenumber = 0;
    updatestrikes(0);

    delay(3000);

    gamemode = 1;
  }
}

void gamerunning() {
  timeleft = long(gamelength);
  timeleft = timeleft * 1000;
  time_scale = 4;
  thismillis = millis();

  do {
    timerupdate();
    buzzerupdate();
    checkinputs();
    updatestrikes(0);
    checkwin();

  } while (gamemode == 1);
  gamemode = 0;
}

//**********************************************************************
// FUNCTIONS: Physical preparation
//**********************************************************************

void place_button() {
  button_trigger = false;

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(F("Button: ")); // 8 characters
  lcd.print(button_colours[button_colour]);
  lcd.setCursor(0, 1);
  lcd.print(F("Label: ")); // 7 characters
  lcd.print(button_labels[button_label]);
  delay(5000);
  track_button_interaction = false;

  while (!button_trigger) {
    timerupdate();
    check_button();
  }

}

void place_swires() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Simple wires:");
  lcd.setCursor(2, 1);
  lcd.print(button_colours_s[wire_colours[0]]);
  lcd.setCursor(4, 1);
  lcd.print(button_colours_s[wire_colours[1]]);
  lcd.setCursor(6, 1);
  lcd.print(button_colours_s[wire_colours[2]]);
  lcd.setCursor(8, 1);
  lcd.print(button_colours_s[wire_colours[3]]);
  lcd.setCursor(10, 1);
  lcd.print(button_colours_s[wire_colours[4]]);
  lcd.setCursor(12, 1);
  lcd.print(button_colours_s[wire_colours[5]]);
  delay(10000);

  while (wire_select != register_inputs) {
    delay(1000);
    check_swires();
  }
}

//**********************************************************************
// FUNCTIONS: Game Generators
//**********************************************************************

void general_setup() {

  for (int i = 0; i < 6; i++) {
    if (i == 0) {
      serial_number[i] = char_generator(1);
    } else if (i == 5) {
      serial_number[i] = char_generator(0);
      if (serial_number[i] % 2 == 1) {
        serial_even = 1;
      } else {
        serial_odd = 1;
      }
    } else {
      serial_number[i] = char_generator(1 - (byte(random(36) / 26)));
    }
  }
  serial_number[6] = '\0';
  Serial.print(F("Serial #: "));
  Serial.println(serial_number);
  //  Serial.print("Vowels?: ");
  //  Serial.println(serial_vowel);
  //  Serial.print("Even?: ");
  //  Serial.println(serial_odd);
  //  Serial.print("Odds?: ");
  //  Serial.println(serial_even);

  ind_frk = random(2);
  ind_car = random(2);
  battery_number = random(6);

  Serial.print(F("FRK: "));
  Serial.println(ind_frk);
  if (ind_frk) {
    digitalWrite(ind_frk_led, HIGH);
  } else {
    digitalWrite(ind_frk_led, LOW);
  }
  Serial.print(F("CAR: "));
  Serial.println(ind_car);
  if (ind_car) {
    digitalWrite(ind_car_led, HIGH);
  } else {
    digitalWrite(ind_car_led, LOW);
  }
  digitalWrite(ind_car_led, ind_car);
  Serial.print(F("Battery number: "));
  Serial.println(battery_number);

}

void button_setup() {
  button_colour = random(4);
  button_label = random(4);

  Serial.print(F("Button colour: "));
  Serial.println(button_colours[button_colour]);
  Serial.print(F("Button label: "));
  Serial.println(button_labels[button_label]);

  if (button_colour == 0 && button_label == 0) {
    shortpushneeded = false;
  } else if (battery_number > 1 && button_label == 1) {
    shortpushneeded = true;
  } else if (ind_car == 1 && button_colour == 1) {
    shortpushneeded = false;
  } else if (battery_number > 2 && ind_frk == 1) {
    shortpushneeded = true;
  } else if (button_colour == 2) {
    shortpushneeded = false;
  } else if (button_colour == 3 && button_label == 2) {
    shortpushneeded = true;
  } else {
    shortpushneeded = false;
  }

  Serial.print(F("Short push?: "));
  Serial.println(shortpushneeded, DEC);

  strip_colour = random(4);
  Serial.print(F("Strip colour: "));
  Serial.println(button_colours[strip_colour]);

  Serial.print(F("Release on "));
  if (strip_colour == 0) {
    Serial.println("4.");
  } else if (strip_colour == 2) {
    Serial.println("5.");
  } else {
    Serial.println("1.");
  }

}

void simple_wires_setup() {

  wire_select = B00111111;
  wire_no = (random(120) % 4) + 3;
  byte removed_number = 6;
  Serial.print(F("Number of simple wires: "));
  Serial.println(wire_no);
  byte wire_counter[5] = {0, 0, 0, 0, 0};
  while (removed_number > wire_no) {
    wire_deleter();
    removed_number = 6;
    for (int i = 0; i < 6; i++) {
      removed_number = removed_number + bitRead(wire_select, i) - 1;
    }
  }

  Serial.println(wire_select, BIN);

  byte this_colour;
  for (byte i = 0; i < 6; i++) {
    if (bitRead(wire_select, i) == 1) {
      this_colour = random(5);
      wire_colours[i] = this_colour;
      wire_counter[this_colour]++;
    } else {
      wire_colours[i] = 5;
    }
    Serial.print("Wire ");
    Serial.print(i + 1);
    Serial.print(": ");
    Serial.println(button_colours[wire_colours[i]]);
  }

  byte temp_counter = 0;
  byte wires_present[wire_no];
  for (int i = 0; i < 6; i++) {
    if (bitRead(wire_select, i) == 1) {
      wires_present[temp_counter] = i;
      temp_counter++;
    }
  }

  Serial.print(F("Wires present:"));
  for (byte i = 0; i < wire_no; i++) {
    Serial.print(" ");
    Serial.print(wires_present[i] + 1);
  }
  Serial.println();

  if (wire_no == 3) {
    if (wire_counter[3] == 0) { // if there are no red wires...
      correct_wire = wires_present[1]; // cut the 2nd wire.
    } else if (wire_colours[wires_present[2]] == 1) { // otherwise if the last wire is white...
      correct_wire = wires_present[2]; // cut the last wire.
    } else if (wire_colours[0] > 1) { // otherwise if there is more than one blue wire...
      if (wire_colours[wires_present[2]] == 0) { // (check if last wire is blue)
        correct_wire = wires_present[2]; // cut the last blue wire (3rd wire)
      } else {
        correct_wire = wires_present[1]; // cut the last blue wire (2nd wire)
      }
    } else { // otherwise...
      correct_wire = wires_present[2]; // cut the last wire
    }
  } else if (wire_no == 4) {
    if (wire_counter[3] > 1 && serial_odd == true) { // If there is more than one red wire and the last digit of the serial is odd...
      if (wire_colours[wires_present[3]] == 3) { // (check if last wire is red)
        correct_wire = wires_present[3]; // cut the last red wire (4th wire)
      } else if (wire_colours[wires_present[2]] == 3) { // (check if 3rd wire is red)
        correct_wire = wires_present[2]; // cut the last red wire (3rd wire)
      } else {
        correct_wire = wires_present[1]; // cut the last red wire (2nd wire)
      } // Cut the last red wire
    } else if ((wire_colours[wires_present[3]] == 2 && wire_counter[3] == 0) || wire_counter[0] == 1) { // Otherwise if the last wire is yellow and there are no red wires... or Otherwise if there is exactly one blue wire...
      correct_wire = wires_present[0]; // Cut the first wire
    } else if (wire_counter[2] > 1) { // Otherwise if there is more than one yellow wire...
      correct_wire = wires_present[3]; // Cut the last wire
    } else { // Otherwise...
      correct_wire = wires_present[1]; // cut the second wire.
    }
  } else if (wire_no == 5) {
    if (wire_colours[wires_present[4]] == 4 && serial_odd == true) { // If the last wire is black and the last digit of the serial is odd...
      correct_wire = wires_present[3]; // Cut the 4th wire
    } else if (wire_counter[3] == 1 && wire_counter[2] > 1) { // Otherwise if there is exactly one red wire and more than one yellow wire...
      correct_wire = wires_present[0]; // Cut the first wire
    } else if (wire_counter[4] == 0) { // Otherwise if there are no black wires...
      correct_wire = wires_present[1]; // Cut the 2nd wire
    } else { // Otherwise...
      correct_wire = wires_present[0]; // cut the first wire
    }
  } else {
    if (wire_counter[2] == 0 && serial_odd == true) { // If there are no yellow wires and the last digit of the serial is odd...
      correct_wire = wires_present[2]; // Cut the 3rd wire
    } else if (wire_counter[2] == 1 && wire_counter[1] > 1) { // Otherwise if there is exactly one yellow wire and there is more than one white wire...
      correct_wire = wires_present[3]; // Cut the fourth wire
    } else if (wire_counter[3] == 0) { // Otherwise if there are no red wires...
      correct_wire = wires_present[5]; // Cut the last wire
    } else { // otherwise...
      correct_wire = wires_present[3]; // Cut the fourth wire
    }
  }

  Serial.print(F("Wire to cut in position: "));
  Serial.println(correct_wire + 1);

}

char char_generator(bool letter) {
  byte temp;
  if (letter == 0) {
    temp = (random(10) + 48);
    return temp;
  } else {
    temp = (random(26) + 65);
    if (temp == 65 || temp == 69 || temp == 73 || temp == 79 || temp == 85) {
      serial_vowel = 1;
    }
    return temp;
  }
}

void wire_deleter() {
  byte looking_at_wire;
  looking_at_wire = random(5); // pick a number from 0-5 to choose as a blank space instead of a wire.
  if (bitRead(wire_select, looking_at_wire) == 1) { //if this wire is here
    bitClear(wire_select, looking_at_wire);
  }
}

//**********************************************************************
// FUNCTIONS: Game Core
//**********************************************************************

void timerupdate() {
  //Timer update
  delta_t = millis() - thismillis;
  thismillis += delta_t;

  if (gamemode == 1) {
    timeleft = timeleft - (delta_t * time_scale / 4); // Updates the time left with the change in the time since last updating, modified by the time scale due to strikes

    if (timeleft >= 60000) { // Over 1 minute on the clock...
      timestr[0] = (int)(timeleft / 600000) + 48;
      timestr[1] = (int)((timeleft % 600000) / 60000) + 48;
      timestr[2] = (int)((timeleft % 60000) / 10000) + 48;
      timestr[3] = (int)((timeleft % 10000) / 1000) + 48;
    }
    else if (timeleft < 0) { // Timer ran out
      timeleft = 0;
      for (int i = 0; i < 4; i++) {
        timestr[i] = "0";
      }
      game_lose(1);
    }
    else { // Under 1 minute left...
      timestr[0] = (int)(timeleft / 10000) + 48;
      timestr[1] = (int)((timeleft % 10000) / 1000) + 48;
      timestr[2] = (int)((timeleft % 1000) / 100) + 48;
      timestr[3] = (int)((timeleft % 100) / 10) + 48;
    }
    timerdisp.writeDigitNum(0, (int)(timestr[0] - 48));
    timerdisp.writeDigitNum(1, (int)(timestr[1] - 48));
    timerdisp.writeDigitNum(3, (int)(timestr[2] - 48));
    timerdisp.writeDigitNum(4, (int)(timestr[3] - 48));
    //timerdisp.drawColon(true);
    timerdisp.writeDisplay();

    //  Serial.println(int(timeleft / 1000));
    //  Serial.println(int(timestr[3]-48));
    //  Serial.print(int(timeleft / 600000) + 48);
    //  Serial.print(" ");
    //  Serial.print(int((timeleft % 600000) / 60000) + 48);
    //  Serial.print(" ");
    //  Serial.print(int((timeleft % 60000) / 10000) + 48);
    //  Serial.print(" ");
    //  Serial.print(int((timeleft % 10000) / 1000) + 48);
    //  Serial.println(" ");
    //  Serial.println(timestr);
  }
}

void buzzerupdate() {
  //Buzzer
  if (timeleft == 0) {
    noTone(pin_buzzer);
  } else if (buzzerinterrupt < thismillis) {

    digitalWrite(module_master[1][2], LOW);
    digitalWrite(module_master[2][2], LOW);

    if (sec_tick_over != (int)((timeleft % 1000) / 100) + 48 && sec_tick_over == 48) { // If the tenth of the second number has changed from a 9 to 0...
      buzz_timer = thismillis;
      tone(pin_buzzer, 2093, 100); // Low tone on start of new second

    } else if (thismillis - buzz_timer > 750) {
      tone(pin_buzzer, 3520, 100); // High tone near end of current second
      buzz_timer = thismillis;

    }
  }



  sec_tick_over = (int)((timeleft % 1000) / 100) + 48;
}


void updatestrikes(byte module_index) {

  if (module_index != 0 && gamemode == 1) {
    tone(pin_buzzer, 110, 500);
    strikenumber ++;
    Serial.println(strikenumber);
    buzzerinterrupt = thismillis + 500;
    digitalWrite(module_master[module_index][2], HIGH);
    time_scale = 4 + strikenumber;
    Serial.print(F("Strike from "));
    Serial.println(module_labels[module_master[module_index][0]]);
    Serial.println(module_master[module_index][2]);
    Serial.println(module_index);
  }

  if (strikelimit == 3) { // Normal mode - 3 strikes = game over
    switch (strikenumber) {
      case 0:
        lcd.setLED1Pin(LOW);
        lcd.setLED2Pin(LOW);
        break;
      case 1:
        lcd.setLED1Pin(HIGH);
        lcd.setLED2Pin(LOW);
        break;
      case 2:
        blinktime = blinktime + delta_t;
        if (blinktime > blinkperiod) {
          if (blinkbool) {
            lcd.setLED1Pin(LOW);
            lcd.setLED2Pin(LOW);
            blinkbool = false;
          } else {
            lcd.setLED1Pin(HIGH);
            lcd.setLED2Pin(HIGH);
            blinkbool = true;
          }
          blinktime = blinktime - blinkperiod;
        }
        break;
      case 3:
        game_lose(module_index);
        break;
    }
  } else { // Hardcore mode - one error = game over
    if (strikenumber == 0) {
      lcd.setLED1Pin(LOW);
      lcd.setLED2Pin(LOW);
    } else {
      game_lose(module_index);
    }
  }
  lcd.shift595();

}

void game_lose(byte module_index) {
  lcd.clear();
  lcd.setCursor(3, 0);
  lcd.print(F("EXPLODED!"));
  lcd.setCursor(0, 1);
  lcd.print(F("Cause: "));
  lcd.print(module_labels[module_master[module_index][0]]);
  timerdisp.print(10000);
  delay(15000);
  gamemode = 0;
}

void moduledefuzed(byte module_index) {
  module_master[module_index][1] = 1;
  digitalWrite(module_master[module_index][3], HIGH);
  Serial.print(F("Defused "));
    Serial.println(module_labels[module_master[module_index][0]]);
    Serial.println(module_master[module_index][3]);
    Serial.println(module_index);
}

void checkwin() {
  if ((module_master[1][0] != 0 && module_master[1][1] == 1) && (module_master[2][0] != 0 && module_master[2][1] == 1)) {

    lcd.clear();
    lcd.setCursor(4, 0);
    lcd.print(F("DEFUSED!"));
    lcd.setCursor(0, 1);
    lcd.print(F("Time: "));
    lcd.print(timestr[0]);
    lcd.print(timestr[1]);
    lcd.print(":");
    lcd.print(timestr[2]);
    lcd.print(timestr[3]);
    timerdisp.setBrightness(0);
    delay(750);
    timerdisp.setBrightness(15);
    delay(750);
    timerdisp.setBrightness(0);
    delay(750);
    timerdisp.setBrightness(15);
    delay(7750);
    gamemode = 0;
  }
}


//**********************************************************************
// FUNCTIONS: Input Handlers
//**********************************************************************

void checkinputs() {

  check_button();
  check_swires();

}

void check_button() {   //BUTTON CHECK
  bool thisread = digitalRead(module_master[1][4]);
  const long debounceDelay = 25;
  const long buttonpressorhold = 1000;

  //  Serial.println(button_colours[strip_colour]);
  //  Serial.println(shortpushneeded);

  if ((thisread != track_button_state) && (thismillis > lastDebounceTime + debounceDelay)) { // If the input has changed from what we have previously recorded, and time has moved on beyond any debounce...
    lastDebounceTime = thismillis; // Remember this change time
    track_button_state = thisread; // Remember the new state
    if (thisread == LOW) { // If button is pressed..
      track_button_interaction = true; // Then this module has been interacted with.
    }
  }

  //    Serial.print(track_button_interaction == true);
  //    Serial.print(":");
  //    Serial.print(track_button_state == HIGH);
  //    Serial.print(":");
  //    Serial.println(thismillis - lastDebounceTime);
  //    Serial.println(isthisapress);

  if (track_button_state == LOW) { // The button is pressed...
    if (thismillis - lastDebounceTime > buttonpressorhold) { // and has been pressed long enough to be considered a hold...
      isthisapress = false;
      //      if (shortpushneeded) { // Needed to push quickly, not do this hold...
      //        if (track_button_interaction) { // Don't overpenalise for every cycle of the hold
      //          Serial.println(F("BUZZ! Only need short press!"));
      //          updatestrikes(2);
      //          track_button_interaction = false;
      //        }
      //      } else { // This hold is correct so trigger strip...
      if (gamemode == 1) {
        if (strip_colour == 0) {
          updateRGBLED(false, false, true);
        } else if (strip_colour == 1) {
          updateRGBLED(true, true, true);
        } else if (strip_colour == 2) {
          updateRGBLED(true, true, false);
        } else {
          updateRGBLED(true, false, false);
        }
      }
    } else { // has been pressed only recently so not yet a hold
      isthisapress = true;
    }
  } else if (track_button_state == HIGH) { // The button is not pressed...
    if (track_button_interaction) { // But it has been interacted with, and so it is now released...
      if (gamemode != 1) {
        button_trigger = true;
      } else {
        track_button_interaction = false;
        updateRGBLED(false, false, false);

        if (shortpushneeded) { // Needs only short push
          if (isthisapress) { // Short press released - correct
            Serial.println(F("Success! Short press"));
            moduledefuzed(1);
          } else { // Long hold release - wrong
            Serial.println(F("BUZZ! Do not hold"));
            updatestrikes(1);
          }
        } else { // Needs long hold
          if (isthisapress) { // Short press released - wrong
            Serial.println(F("BUZZ! Hold longer"));
            updatestrikes(1);
          } else { // Long hold release - correct
            Serial.println(timestr);
            if (strip_colour == 0) {
              if (timestr[0] == '4' || timestr[1] == '4' || timestr[2] == '4' || timestr[3] == '4') {
                Serial.println(F("Success!"));
                moduledefuzed(1);
              } else {
                Serial.println(F("BUZZ! Should be 4"));
                updatestrikes(1);
              }
            } else if (strip_colour == 2) {
              if (timestr[0] == '5' || timestr[1] == '5' || timestr[2] == '5' || timestr[3] == '5') {
                Serial.println(F("Success!"));
                moduledefuzed(1);
              } else {
                Serial.println(F("BUZZ! Should be 5"));
                updatestrikes(1);
              }
            } else {
              if (timestr[0] == '1' || timestr[1] == '1' || timestr[2] == '1' || timestr[3] == '1') {
                Serial.println(F("Success!"));
                moduledefuzed(1);
              } else {
                Serial.println(F("BUZZ! Should be 1"));
                updatestrikes(1);
              }
            }
          }
        }
      }
    }
  }
}


void check_swires() { //SIMPLE WIRES CHECK
  digitalWrite(module_master[2][5], HIGH);
  digitalWrite(module_master[2][6], HIGH);
  digitalWrite(module_master[2][5], LOW);
  register_inputs = shiftIn(module_master[2][4], module_master[2][6], LSBFIRST);
  //register_inputs = register_inputs >> 2;

  byte byte_compare;

  if (register_inputs != track_wire_state) { // change in inputs

    Serial.println(register_inputs, BIN);

    byte_compare = register_inputs ^ track_wire_state;
    Serial.println(byte_compare, BIN);

    if (gamemode == 1) {

      for (byte i = 0; i < 6; i++) { // Look through all bits
        if (bitRead(byte_compare, i) == 1) { // Find the changed bit = the cut wire position
          if (correct_wire == i) { // If the cut wire is the correct wire...
            Serial.println(F("Success!"));
            moduledefuzed(2);
          } else { // If the cut wire isn't the correct wire
            Serial.println(F("BUZZ!"));
            updatestrikes(2);
          }
        }
      }
    }

    track_wire_state = register_inputs;

  }

}

void updateRGBLED (bool r, bool g, bool b) {
  if (r) {
    digitalWrite(module_master[1][5], HIGH);
  } else {
    digitalWrite(module_master[1][5], LOW);
  }
  if (g) {
    digitalWrite(module_master[1][6], HIGH);
  } else {
    digitalWrite(module_master[1][6], LOW);
  }
  if (b) {
    digitalWrite(module_master[1][7], HIGH);
  } else {
    digitalWrite(module_master[1][7], LOW);
  }
}


//**********************************************************************
// FUNCTIONS: Hardware Allocation
//**********************************************************************

void initialisehardware() { // For testing, use the other to dynamically assign pins

  // Manually set timer
  module_master[0][0] = 1;
  module_master[0][1] = 2;
  module_master[0][2] = 3;
  module_master[0][3] = 4;
  module_master[0][4] = 5;
  module_master[0][5] = 7;
  module_master[0][6] = 8;
  pinMode(module_master[0][1], OUTPUT);
  pinMode(module_master[0][2], OUTPUT);
  pinMode(module_master[0][3], OUTPUT);
  pinMode(module_master[0][4], OUTPUT);
  pinMode(module_master[0][5], OUTPUT);
  pinMode(module_master[0][6], OUTPUT);

  // Manually set button
  module_master[1][0] = 2;
  module_master[1][1] = 0;
  module_master[1][2] = 25;
  module_master[1][3] = 23;
  module_master[1][4] = 33; // Button input
  module_master[1][5] = 27; // LED R
  module_master[1][6] = 29; // LED G
  module_master[1][7] = 31; // LED B
  pinMode(module_master[1][2], OUTPUT);
  pinMode(module_master[1][3], OUTPUT);

  // Manually set simple wires
  module_master[2][0] = 3;
  module_master[2][1] = 0;
  module_master[2][2] = 47;
  module_master[2][3] = 45;
  module_master[2][4] = 53; // Data
  module_master[2][5] = 51; // Latch
  module_master[2][6] = 49; // Clock
  pinMode(module_master[2][2], OUTPUT);
  pinMode(module_master[2][3], OUTPUT);

  pinMode(pin_buzzer, OUTPUT);
  digitalWrite(pin_buzzer, LOW); //Buzzer

  pinMode(module_master[1][4], INPUT_PULLUP); //Button
  pinMode(module_master[1][5], OUTPUT);
  digitalWrite(module_master[1][5], LOW); //RGB LED R
  pinMode(module_master[1][6], OUTPUT);
  digitalWrite(module_master[1][6], LOW); //RGB LED G
  pinMode(module_master[1][7], OUTPUT);
  digitalWrite(module_master[1][7], LOW); //RGB LED B

  timerdisp.begin(0x70); // Timer uses I2C pins @ 0x70
  timerdisp.print(10000);
  timerdisp.writeDisplay();

  lcd.begin(16, 2);

  pinMode(module_master[2][4], INPUT); // Data pin
  pinMode(module_master[2][5], OUTPUT); // Latch pin
  pinMode(module_master[2][6], OUTPUT); // Clock pin

}

/*
  void initialisehardware() {
  // Bomb needs to identify the modules connected so it can generate scenario and interact with the connect devices.

  // Timer:
  for (byte j = 0; j < 4; j++) { // 4x output [Strike LED #1, Strike LED #2, Success LED, Pizeo buzzer].
    assigndo(thismodule, j);
  }
  thismodule++;

  timerdisp.begin(0x70); //Timer uses I2C pins
  timerdisp.print(10000);
  timerdisp.writeDisplay();



  // Button:
  for (byte i = 0; i < module_button; i++) {
    for (byte j = 0; j < 1; j++) { // 1x input [Button]
      assigndi(i + thismodule, j);
    }
    for (byte j = 0; j < 5; j++) { // 5x output [Success LED R, Success LED G, RGB LED R, RGB LED G, RGB LED B]
      assigndo(i + thismodule, j);
    }
    thismodule ++;
  }

  // Simple wires:
  for (byte i = 0; i < module_swires; i++) {
    for (byte j = 0; j < 1; j++) { // 1x input [Analog mix of all 6 wires]
      assignai(i, j);
    }
    for (byte j = 0; j < 2; j++) { // 2x output [Success LED R, Success LED G]
      assigndo(i, j);
    }
  }

  }



  void assigndi(byte moduleno, byte pinno) {
  pinsi[moduleno][pinno] = thisdpin;
  pinMode(pinsi[moduleno][pinno], INPUT_PULLUP);
  thisdpin++;
  }

  void assigndo(byte moduleno, byte pinno) {
  pinso[moduleno][pinno] = thisdpin;
  // pinsostate[moduleno][pinno] = LOW;
  pinMode(pinso[moduleno][pinno], OUTPUT);
  digitalWrite(pinso[moduleno][pinno], LOW);
  thisdpin++;
  }

  void assignai(byte moduleno, byte pinno) {
  pinsi[moduleno][pinno] = thisapin;
  pinMode(pinsi[moduleno][pinno], INPUT);
  thisapin++;
  }

*/
