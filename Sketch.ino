

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
byte pin_button_button = 9;
byte pin_button_led_r = 10;
byte pin_button_led_g = 11;
byte pin_button_led_b = 12;
byte pin_swires_d = 6;
byte pin_swires_l = 7;
byte pin_swires_c = 8;

Adafruit_7segment timerdisp = Adafruit_7segment();
LiquidCrystal595 lcd(pin_lcd_d, pin_lcd_l, pin_lcd_c); // Data pin, latch pin, clock pin

byte gamemode = 0;

//Track allocation of pins, as inputs or outputs
//byte pinsi[5][4]; // Set max number of modules to 6, max number of pins per module to 5.
//byte pinso[5][4]; // Set max number of modules to 6, max number of pins per module to 5.
//byte inputstates[5][4]; //Set array of output pin values to be able to track state changes.
//byte inputchanges[5][4];

// Pins dynamically set counting up from d pin #2, a pin #14
byte thisdpin = 2;
byte thisapin = 14;
byte thismodule = 0;

long timeleft;
int gamelength = 300; //seconds
long thismillis;
long delta_t;
char timestr[5] = "----";
// byte strikenumber = 0;
char sec_tick_over[1];
long buzz_timer;
byte time_scale; // Quadruple the time scale: 4 = 1x speed (normal), 5 = 1.25x speed (1 strike), etc...

byte module_button = 1; //Hard-coded number of button modules.
byte module_swires = 1; //Hard-coded number of simple wires modules.

long lastDebounceTime = 0;  // the last time the output pin was toggled
const long debounceDelay = 25;
const long buttonpressorhold = 1000;

bool track_button_state = HIGH;
bool track_button_interaction = false; //tracking if the button has ever been pressed, therefore logic for played interaction
bool shortpushneeded;
byte isthisapress;

byte button_colour;
byte button_label;
byte strip_colour;
String button_labels[4] = {"Abort", "Detonate", "Hold", "Press"};
String button_colours[6] = {"Blue", "White", "Yellow", "Red", "Black", "---"}; //Black included so simple wires can use this array too. Button should not call black.
char button_colours_s[6] = {'B', 'W', 'Y', 'R', 'K', '_'};

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
  Serial.println("Initialisation done!");
}

void loop() {
  // put your main code here, to run repeatedly:
  switch (gamemode) {
    case 1: //game running
      Serial.println("Game starting!");
      gamerunning();
      break;
    case 2: //also stand-by post-game
      gamesetup();
      break;
    default: //stand-by state - stop inputs and hold timer post-game, also generate single round and prep game. 0 if completed initialisation, otherwise 2 after game has ran.
      Serial.println("Building new game:");
      gamesetup();
      break;
  }
}

//**********************************************************************
// FUNCTIONS: Game Managers
//**********************************************************************

void gamesetup() {
  if (gamemode == 2) {

  } else {
    general_setup();
    button_setup();
    Serial.println("Button setup done...");
    simple_wires_setup();
    Serial.println("Simple wires setup done...");

    char timestr[5] = "0500";
    timerdisp.writeDigitNum(0, int(timestr[0] - 48));
    timerdisp.writeDigitNum(1, int(timestr[1] - 48));
    timerdisp.writeDigitNum(3, int(timestr[2] - 48));
    timerdisp.writeDigitNum(4, int(timestr[3] - 48));
    timerdisp.drawColon(true);
    timerdisp.writeDisplay();

    place_swires();

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Button: "); // 8 characters
    lcd.print(button_colours[button_colour]);
    lcd.setCursor(0, 1);
    lcd.print("Label: "); // 7 characters
    lcd.print(button_labels[button_label]);

    track_button_state = HIGH; //Button starts out not pressed
    track_button_interaction = false; //Button was never interacted with
    lastDebounceTime = millis();

    gamemode = 1;
  }
}

void gamerunning() {
  Serial.println("Game starting...");
  timeleft = long(gamelength);
  timeleft = timeleft * 1000;
  time_scale = 4;
  thismillis = millis();

  do {
    timerupdate();
    buzzerupdate();
    checkinputs();
    updatestrikes();
    // lightcycle(); // Calls test LED colour function

  } while (timeleft > 0);
  gamemode = 0;
}

//**********************************************************************
// FUNCTIONS: Physical preparation
//**********************************************************************

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
    delay(5000);
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
  Serial.print("Serial #: ");
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

  Serial.print("FRK: ");
  Serial.println(ind_frk);
  Serial.print("CAR: ");
  Serial.println(ind_car);
  Serial.print("Battery number: ");
  Serial.println(battery_number);

}

void button_setup() {
  button_colour = random(4);
  button_label = random(4);

  Serial.print("Button colour: ");
  Serial.println(button_colours[button_colour]);
  Serial.print("Button label: ");
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

  Serial.print("Short push?: ");
  Serial.println(shortpushneeded,DEC);

  strip_colour = random(4);

  if (strip_colour == 0) {
    Serial.println("Release on 4.");
  } else if (strip_colour == 2) {
    Serial.println("Release on 5.");
  } else {
    Serial.println("Release on 1.");
  }

}

void simple_wires_setup() {

  wire_select = B00111111;
  wire_no = (random(120) % 4) + 3;
  byte removed_number = 6;
  Serial.print("Number of simple wires: ");
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

  Serial.print("Wires present:");
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

  Serial.print("Wire to cut in position: ");
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
  timeleft = timeleft - (delta_t * time_scale / 4); // Updates the time left with the change in the time since last updating, modified by the time scale due to strikes

  if (timeleft >= 60000) { // Over 1 minute on the clock...
    timestr[0] = int(timeleft / 600000) + 48;
    timestr[1] = int((timeleft % 600000) / 60000) + 48;
    timestr[2] = int((timeleft % 60000) / 10000) + 48;
    timestr[3] = int((timeleft % 10000) / 1000) + 48;
  }
  else if (timeleft < 0) { // Timer ran out
    timeleft = 0;
    for (int i = 0; i < 4; i++) {
      timestr[i] = "0";
    }
  }
  else { // Under 1 minute left...
    timestr[0] = int(timeleft / 10000) + 48;
    timestr[1] = int((timeleft % 10000) / 1000) + 48;
    timestr[2] = int((timeleft % 1000) / 100) + 48;
    timestr[3] = int((timeleft % 100) / 10) + 48;
  }
  timerdisp.writeDigitNum(0, int(timestr[0] - 48));
  timerdisp.writeDigitNum(1, int(timestr[1] - 48));
  timerdisp.writeDigitNum(3, int(timestr[2] - 48));
  timerdisp.writeDigitNum(4, int(timestr[3] - 48));
  //timerdisp.drawColon(true);
  timerdisp.writeDisplay();

  //  Serial.println(int(timeleft/1000));
  //  Serial.println(int(timestr[3]-48));

}

void buzzerupdate() {
  //Buzzer
  if (timeleft == 0) {
    noTone(pin_buzzer);
  } else if (sec_tick_over[1] != (int((timeleft % 1000) / 100) + 48) && sec_tick_over[1] == 48) { // If the tenth of the second number has changed from a 9 to 0...
    buzz_timer = thismillis;
    tone(pin_buzzer, 2093, 100); //Low tone on start of new second
    //    Serial.println("-boop");
  } else if (thismillis - buzz_timer > 750) {
    tone(pin_buzzer, 3520, 100); //High tone near end of current second
    buzz_timer = thismillis;
    //    Serial.print("Beep");
  }

  //  Serial.print("buzz_timer:              ");
  //  Serial.println(buzz_timer);
  //  Serial.print("thismillis - buzz_timer:   ");
  //  Serial.println(thismillis - buzz_timer);

  //  Serial.println(sec_tick_over[1] == 48);
  //  Serial.print("int((timeleft % 10000) / 1000) + 48): ");
  //  Serial.println(char(int((timeleft % 10000) / 1000) + 48));
  //  Serial.print("sec_tick_over: ");
  //  Serial.println(sec_tick_over[1]);
  //  Serial.println(sec_tick_over[1] != (int((timeleft % 10000) / 1000) + 48));

  sec_tick_over[1] = int((timeleft % 1000) / 100) + 48;

  //  if (timeleft == 0) {
  //    noTone(pin_buzzer);
  //  }
  //  else if ((timeleft % 1000) < 150) {
  //    tone(pin_buzzer, 2093); //Low tone on start of new second
  //  }
  //  else if ((timeleft % 1000) >= 750 && (timeleft % 1000) < 850) {
  //    tone(pin_buzzer, 3520); //High tone near end of current second
  //  }
  //  else {
  //    noTone(pin_buzzer);
  //  }
  //}

  // Test different colours on LED
  //void lightcycle() {
  //  if (int((timeleft / 1000) % 6) == 0) {
  //    updateRGBLED(true, false, false);
  //  } else if (int((timeleft / 1000) % 6) == 1) {
  //    updateRGBLED(true, true, false);
  //  } else if (int((timeleft / 1000) % 6) == 2) {
  //    updateRGBLED(false, true, false);
  //  } else if (int((timeleft / 1000) % 6) == 3) {
  //    updateRGBLED(false, true, true);
  //  } else if (int((timeleft / 1000) % 6) == 4) {
  //    updateRGBLED(false, false, true);
  //  } else if (int((timeleft / 1000) % 6) == 5) {
  //    updateRGBLED(true, false, true);
  //  }
  //}
}

/*
  void checkinputs() {
  // read the input state and state changes of all buttons
  static unsigned long lastButtonTime; // time stamp for remembering the time when the button states were last read
  memset(buttonChange, 0, sizeof(buttonChange)); // reset all old state changes
  if (millis() - lastButtonTime < BOUNCETIME) return; // within bounce time: leave the function
  lastButtonTime = millis(); // remember the current time
  for (int i = 0; i < NUMBUTTONS; i++)
  {
    byte curState = digitalRead(buttonPins[i]);      // current button state
    if (INPUTMODE == INPUT_PULLUP) curState = !curState; // logic is inverted with INPUT_PULLUP
    if (curState != buttonState[i])                  // state change detected
    {
      if (curState == HIGH) buttonChange[i] = BUTTONDOWN;
      else buttonChange[i] = BUTTONUP;
    }
    buttonState[i] = curState; // save the current button state
  }
  }
*/

void updatestrikes() {

}


//**********************************************************************
// FUNCTIONS: Input Handlers
//**********************************************************************

void checkinputs() {

  check_button();
  check_swires();

}

void check_button() {   //BUTTON CHECK
  bool thisread = digitalRead(pin_button_button);

  if ((thisread != track_button_state) && (millis() > lastDebounceTime + debounceDelay)) { // If the input has changed from what we have previously recorded, and time has moved on beyond any debounce...
    lastDebounceTime = millis(); // Remember this change time
    track_button_state = thisread; // Remember the new state
  }

//    Serial.print(track_button_interaction == true);
//    Serial.print(":");
//    Serial.print(track_button_state == HIGH);
//    Serial.print(":");
//    Serial.println(millis() - lastDebounceTime);

  if (track_button_state == LOW) { // The button is pressed...
    if (millis() - lastDebounceTime > buttonpressorhold) { // and has been pressed for long enough to be considered a hold...
      //      Serial.println("Button held...");
      isthisapress = false;
      Serial.println(isthisapress);
      if (track_button_interaction == true) {
        button_logic();
      }
    }
    else if ((millis() - lastDebounceTime) > debounceDelay) { // and has been pressed for long enough to filter out bounce...
      //      Serial.println("Button pressed...");
      isthisapress = true;
      Serial.println(isthisapress);
      track_button_interaction = true;
    }
  }
  else if ((track_button_state == HIGH) && (track_button_interaction == true)) { // The button is not pressed but has been interacted with (does not consider state at start of game)...
    if ((millis() - lastDebounceTime) > debounceDelay) { // and has been released long enough ago to ignore bounce...
//      if (isthisapress == 1) { // and the button was pressed briefly...
//                Serial.println("Short push!");
//      }
//      else if (isthisapress == 0) { // and the button was held down...
//                Serial.println("Long hold!");
//      }
    }
    button_logic();
    track_button_interaction = false;
  }

}


void check_swires() { //SIMPLE WIRES CHECK
  digitalWrite(pin_swires_l, HIGH);
  digitalWrite(pin_swires_c, HIGH);
  digitalWrite(pin_swires_l, LOW);
  register_inputs = shiftIn(pin_swires_d, pin_swires_c, LSBFIRST);
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
            Serial.println("Success!");
          } else { // If the cut wire isn't the correct wire
            Serial.println("BUZZ!");
          }
        }
      }
    }

    track_wire_state = register_inputs;

  }

}

void updateRGBLED (bool r, bool g, bool b) {
  if (r == true) {
    digitalWrite(pin_button_led_r, HIGH);
  } else {
    digitalWrite(pin_button_led_r, LOW);
  }
  if (g == true) {
    digitalWrite(pin_button_led_g, HIGH);
  } else {
    digitalWrite(pin_button_led_g, LOW);
  }
  if (b == true) {
    digitalWrite(pin_button_led_b, HIGH);
  } else {
    digitalWrite(pin_button_led_b, LOW);
  }
}

void button_logic () {
  if (track_button_state == LOW && !isthisapress) { // Button is currently being held
    if (!shortpushneeded) { // Button needs to be held
      if (strip_colour == 0) {
        updateRGBLED(false, false, true);
      } else if (strip_colour == 1) {
        updateRGBLED(true, true, true);
      } else if (strip_colour == 2) {
        updateRGBLED(true, true, false);
      } else {
        updateRGBLED(true, false, false);
      }
    } else { // Button needs only short press
      Serial.println("BUZZ! Only need short press!");
      track_button_interaction = false;
    }
  } else if (track_button_state == HIGH) { // Button is released
    Serial.println("track");
    Serial.println(isthisapress);
    if (isthisapress) { // Button was short pressed
      Serial.println("press");
      if (shortpushneeded) { //Button needs to be short pressed
        Serial.println("Success!");
      } else { // Button needs holding
        Serial.println("BUZZ! Need to hold the button!");
      }
    } else { // Button was held
      if (!shortpushneeded) { // Button needs to be held
        Serial.println(timestr);
        if (strip_colour == 0) {
          if (timestr[0] == '4' || timestr[1] == '4' || timestr[2] == '4' || timestr[3] == '4') {
            Serial.println("Success!");
          } else {
            Serial.println("BUZZ! wrong time");
          }
        } else if (strip_colour == 2) {
          if (timestr[0] == '5' || timestr[1] == '5' || timestr[2] == '5' || timestr[3] == '5') {
            Serial.println("Success!");
          } else {
            Serial.println("BUZZ!  wrong time");
          }
        } else {
          if (timestr[0] == '1' || timestr[1] == '1' || timestr[2] == '1' || timestr[3] == '1') {
            Serial.println("Success!");
          } else {
            Serial.println("BUZZ!   wrong time");
          }
        }
      }
    }
    updateRGBLED(false, false, false); // turn off LED after release
  }
}

//**********************************************************************
// FUNCTIONS: Hardware Allocation
//**********************************************************************

void initialisehardware() { // For testing, use the other to dynamically assign pins
  pinMode(pin_buzzer, OUTPUT);
  digitalWrite(pin_buzzer, LOW); //Buzzer
  pinMode(pin_button_button, INPUT_PULLUP); //Button
  pinMode(pin_button_led_r, OUTPUT);
  digitalWrite(pin_button_led_r, LOW); //RGB LED R
  pinMode(pin_button_led_g, OUTPUT);
  digitalWrite(pin_button_led_g, LOW); //RGB LED G
  pinMode(pin_button_led_b, OUTPUT);
  digitalWrite(pin_button_led_b, LOW); //RGB LED B

  timerdisp.begin(0x70); //Timer uses I2C pins
  timerdisp.print(10000);
  timerdisp.writeDisplay();

  lcd.begin(16, 2);

  pinMode(pin_swires_d, INPUT); // Data pin
  pinMode(pin_swires_l, OUTPUT); // Latch pin
  pinMode(pin_swires_c, OUTPUT); // Clock pin
  digitalWrite(6, LOW);
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
