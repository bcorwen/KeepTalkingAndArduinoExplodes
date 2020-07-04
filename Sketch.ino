//======================================================================
//
//  Keep Talking and Arduino Explodes!
//
//======================================================================
//
//  version 0.4.0
//
//  Goal for this version: Firm up game functions, include Simon Says
//                          and include shift register support for mods
//
//======================================================================
//
//  Modules supported: Button, Simple wires, Simon says
//
//======================================================================

//**********************************************************************
// LIBRARIES
//**********************************************************************

#include <Wire.h> // Enable this line if using Arduino Uno, Mega, etc.
#include <Adafruit_GFX.h>
#include <Adafruit_LEDBackpack.h>
#include <Entropy.h>
#include <LiquidCrystal.h>
//#include <LiquidCrystal595.h>

//**********************************************************************
// GLOBAL VARIABLES
//**********************************************************************

// Temp pin variables:
byte pin_buzzer = 2;
//byte pin_lcd_d = 3;
//byte pin_lcd_l = 4;
//byte pin_lcd_c = 5;
//byte pin_button_button = 31;
//byte pin_button_led_r = 29;
//byte pin_button_led_g = 27;
//byte pin_button_led_b = 25;
//byte pin_swires_d = 53;
//byte pin_swires_l = 51;
//byte pin_swires_c = 49;
byte ind_car_led = 40;
byte ind_frk_led = 42;

byte pin_lcd_rs = 7;
byte pin_lcd_e = 8;
byte pin_lcd_d4 = 3;
byte pin_lcd_d5 = 4;
byte pin_lcd_d6 = 5;
byte pin_lcd_d7 = 6;

Adafruit_7segment timerdisp = Adafruit_7segment();
//LiquidCrystal595 lcd(pin_lcd_d, pin_lcd_l, pin_lcd_c); // Data pin, latch pin, clock pin
LiquidCrystal lcd(pin_lcd_rs, pin_lcd_e, pin_lcd_d4, pin_lcd_d5, pin_lcd_d6, pin_lcd_d7);

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
// Module codes:      0         1        2          3           4         5          6
//                No module   Timer    Button  Simple wires   Keypad    Simon   Compl wires
//
//                         i =  0           1         2         3           4               5               6              7
// Timer layout         = {module type,   buzzer , strike 1, strike 2,     ---  ,     ind_car_led,     ind_frk_led}
// Button layout        = {module type, completed, strike R, strike G, button input,      LED R,          LED G,         LED B}
// Simple wires layout  = {module type, completed, strike R, strike G, register data, register latch,  register clock}
// Keypad layout        = {module type, completed, strike R, strike G,  in reg #,      in reg bits,     out reg #,   out reg bits}
// Simon says layout    = {module type, completed, strike R, strike G,  in reg #,      in reg bits,     out reg #,   out reg bits}
// Compl wires layout   = {module type, completed, strike R, strike G, register data, register latch,  register clock}
//
// For 4-input modules, reference to a shift-in register and position of the bits will suffice for pin allocations

byte shiftin_master[6];
byte shiftout_master[6];

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

byte simon_sequence[5];
byte simon_length;
byte simon_stage; // Which stage the game is at (e.g. how many lights are flashing)
byte simon_step; // Which step in the sequence the player is at (e.g. ready for the 3rd in the sequence)
long simon_led_timing; // Tracks time for lamp lights
byte simon_disp_step; // Tracks the current part of the sequence which is being lit
long simon_reset_timing; // Tracks time for module to restart flashing sequence
bool simon_button_track;
bool simon_buzz;
// Temp output pins while no shift out reg to use:
byte simon_led_r = 42;
byte simon_led_b = 40;
byte simon_led_y = 38;
byte simon_led_g = 36;

byte keypad_stage;
byte keypad_symbols[4];
byte symbol_index[7];
byte keypad_order[4];
byte keypad_column_choice;
bool keypad_track_state;
byte lcd_custom1[8];
byte lcd_custom2[8];
byte lcd_custom3[8];
byte lcd_custom4[8];

long wrong_flash_time;

byte track_wire_state = B00111111;
byte register_inputs;
byte wire_colours[6] = {5, 5, 5, 5, 5, 5}; //5 equates to no wire present. 0-4 are colours defined above.
byte wire_no;
byte wire_select = B00111111;
byte correct_wire;

byte track_cwire_state = B00111111;
byte cwire_setting[6];
byte cwire_correct;
byte register_cwires;

char serial_number[7];
bool serial_vowel;
bool serial_odd;
bool serial_even;

byte battery_number;
byte port_number;
byte ind_number;
byte widget_types[5];

bool parallel_port;
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

    // Reset timer
    char timestr[5] = "0500";
    timerdisp.writeDigitNum(0, int(timestr[0] - 48));
    timerdisp.writeDigitNum(1, int(timestr[1] - 48));
    timerdisp.writeDigitNum(3, int(timestr[2] - 48));
    timerdisp.writeDigitNum(4, int(timestr[3] - 48));
    timerdisp.drawColon(true);
    timerdisp.writeDisplay();

    // Turn off module success LEDs
    //    digitalWrite(module_master[1][2], LOW);
    //    digitalWrite(module_master[2][2], LOW);
    //    digitalWrite(module_master[1][3], LOW);
    //    digitalWrite(module_master[2][3], LOW);
    //    bitWrite(shiftout_master[3], 2, 0);
    //    bitWrite(shiftout_master[3], 3, 0);
    //    bitWrite(shiftout_master[3], 4, 0);
    //    bitWrite(shiftout_master[3], 5, 0);
    //    bitWrite(shiftout_master[3], 6, 0);
    //    bitWrite(shiftout_master[3], 7, 0);

    digitalWrite(module_master[0][2], LOW);
    digitalWrite(module_master[0][3], LOW);
    digitalWrite(module_master[1][2], LOW);
    digitalWrite(module_master[1][3], LOW);
    digitalWrite(module_master[2][2], LOW);
    digitalWrite(module_master[2][3], LOW);
    digitalWrite(module_master[3][2], LOW);
    digitalWrite(module_master[3][3], LOW);

    //    shiftout_master[1] = 0;
    //    shiftout_master[2] = 0;
    shift_out();

    // Create game win-conditions
    general_setup();
    button_setup();
    Serial.println(F("Button setup done..."));
    simple_wires_setup();
    Serial.println(F("Simple wires setup done..."));
    simon_setup();
    Serial.println(F("Simon Says setup done..."));
    keypad_setup();
    Serial.println(F("Keypad setup done..."));
    //        cwires_setup();
    //        Serial.println(F("Complicated wires setup done..."));

    // Physical set-up for button and simple wires...
    place_button();
    place_swires();
    place_keypad();
    //    place_cwires();

    // Ready LCD for game to start
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(F("Serial #: "));
    lcd.print(serial_number);
    lcd.setCursor(0, 1);
    lcd.print(F("Battery #: "));
    lcd.print(battery_number);

    // Reset game tracking variables
    track_button_state = HIGH; //Button starts out not pressed
    track_button_interaction = false; //Button was never interacted with
    thismillis = millis();
    lastDebounceTime = thismillis;
    module_master[1][1] = 0;
    module_master[2][1] = 0;
    module_master[3][1] = 0;
    module_master[4][1] = 0;
    strikenumber = 0;
    updatestrikes(0);

    simon_stage = 0;
    //    simon_stage = simon_length-1; // TESTING!
    simon_step = 0;
    simon_led_timing = thismillis;
    simon_disp_step = 10;
    simon_button_track = false;
    digitalWrite(simon_led_r, LOW);
    digitalWrite(simon_led_b, LOW);
    digitalWrite(simon_led_y, LOW);
    digitalWrite(simon_led_g, LOW);

    keypad_stage = 0;

    // Wait and then start the game
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
    simonupdate();
    buzzerupdate();
    checkinputs();
    updatestrikes(0);
    shift_out();
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
  delay(3000);
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
  delay(5000);

  while (wire_select != register_inputs) {
    delay(1000);
    check_swires();
  }
}

//void place_cwires() {
//  lcd.clear();
//  lcd.setCursor(0, 0);
//  lcd.print("Compl' wires:");
//  lcd.setCursor(1, 1);
//  lcd.print("Remove wires...");
//  delay(5000);
//
//  while (register_cwires != 0) {
//    delay(1000);
////    check_cwires();
//  }
//
//  for (byte i = 0; i < 6; i++) {
//    lcd.clear();
//    lcd.setCursor(0, 0);
//    lcd.print("Wire #");
//    lcd.print(i + 1);
//    lcd.setCursor(1, 1);
//    if (bitRead(cwires_setting[i], 0) == 1) {
//      lcd.print("W");
//    }
//    if (bitRead(cwires_setting[i], 1) == 1) {
//      lcd.print("R");
//    }
//    if (bitRead(cwires_setting[i], 2) == 1) {
//      lcd.print("B");
//    }
//    if (bitRead(cwires_setting[i], 3) == 1) {
//      lcd.print(" *");
//    }
//    while (bitRead(register_cwires, 6 - i) != 1) {
//      delay(1000);
////      check_cwires();
//    }
//  }

//}

void place_keypad() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Keypad:");
  lcd.setCursor(13, 0);
  //  lcd.print(keypad_symbols[0]);
  lcd.write(byte(1));
  lcd.setCursor(15, 0);
  //  lcd.print(keypad_symbols[1]);
  lcd.write(byte(2));
  lcd.setCursor(13, 1);
  //  lcd.print(keypad_symbols[2]);
  lcd.write(byte(3));
  lcd.setCursor(15, 1);
  //  lcd.print(keypad_symbols[3]);
  lcd.write(byte(4));
  lcd.setCursor(0, 1);
  lcd.print("C:");
  lcd.print(keypad_column_choice + 1);
  delay(5000);

  check_shiftin();
  //  check_keypad();
  Serial.println(byte(shiftin_master[1]) / 16);
  while (byte(shiftin_master[1]) / 16 == 0) {
    delay(100);
    check_shiftin();
    check_keypad();
  }
}

//**********************************************************************
// FUNCTIONS: Game Generators
//**********************************************************************

void general_setup() {

  for (int i = 0; i < 6; i++) {
    if (i == 3 || i == 4) {
      serial_number[i] = char_generator(1);
    } else if (i == 2) {
      serial_number[i] = char_generator(0);
    } else if (i == 5) {
      serial_number[i] = char_generator(0);
      if (serial_number[i] % 2 == 1) {
        serial_even = 1;
      } else {
        serial_odd = 1;
      }
    } else {
      serial_number[i] = char_generator(1 - (byte(random(35) / 25)));
    }
  }
  serial_number[6] = '\0';
  Serial.print(F("Serial #: "));
  Serial.println(serial_number);
  //    Serial.print("Vowels?: ");
  //    Serial.println(serial_vowel);
  //    Serial.print("Even?: ");
  //    Serial.println(serial_odd);
  //    Serial.print("Odds?: ");
  //    Serial.println(serial_even);

  //  byte widget_number = random(2) + 4;
  //  battery_number = random(3) + random(3); // Number from 0-4, with a bias towards 2
  //  if (widget_number > battery_number) {
  //    ind_number = random(1 + widget_number - battery_number);
  //    port_number = widget_number - (battery_number + ind_number);
  //  } else {
  //    port_number = 0;
  //    ind_number = 0;
  //  }
  //  ind_car = false;
  //  ind_frk = false;
  //  bool isthisacopy = false;
  //  lcd.clear();
  //  lcd.setCursor(0, 0);
  //  for (byte i = 0; i < widget_number; i++) {
  //    if (i < battery_number) { // Add battery
  //      widget_types[i] = random(2); // AA or D
  //    } else if (i < (battery_number + ind_number)) { // Add indicator
  //      if (i > 0) {
  //        do {
  //          isthisacopy = false;
  //          widget_types[i] = random(23) + 10; // All inds (11 types), off and on (x2)
  //          for (byte j = 0; j < i; j++) {
  //            if (widget_types[i] % 2 == widget_types[j] % 2) {
  //              isthisacopy = true;
  //            }
  //          }
  //        } while (isthisacopy = true);
  //        if (widget_types[i] == 15) {
  //          ind_car = true; // 14 = off, 15 = on
  //        } else if (widget_types[i] == 17) {
  //          ind_frk = true; // 16 = off, 17 = on
  //        }
  //      }
  //    } else { // Add port
  //      widget_types[i] = random(4) + 40; // Parallel, serial, other
  //      if (widget_types[i] == 40) {
  //        parallel_port = true;
  //      }
  //    }
  //    if (i > 0) {
  //      lcd.print(":");
  //    }
  //    //    lcd.sprintf(widget_types[i], "%02d");
  //    lcd.print(widget_types[i], DEC);
  //  }
  //
  //  delay(3000);

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

//void cwires_setup() {
//  cwire_correct = 0;
//  byte cwires_logic = {1, serial_even, serial_even, serial_even, 1, 1, 0, parallel_port, 0, battery_check, parallel_port, battery_check, battery_check, battery_check, parallel_port, 0};
//  byte battery_check = 0;
//
//  for (byte i = 0; i < 6; i++) {
//    for (byte j = 0; j < 5; j++) { // White, Red, Blue, *, LED
//      bitWrite(cwire_setting[i], j, byte(random(2)));
//    }
//
//    if (battery_number > 1) {
//      battery_check = 1;
//    } else {
//      battery_check = 0;
//    }
//    if (cwire_setting[i] % 8 == 0) {
//      cwire_setting[i]++; // If no colours selected, default to white.
//    } else if (cwire_setting[i] % 8 == 7) {
//      cwire_setting[i]--; // If all colours selected, remove white.
//    }
//    cwire_correct = cwires_logic[byte(cwire_setting[i] / 2)];
//  }
//}

void simon_setup() {
  // R=0, B=1, Y=2, G=3.
  Serial.println(F("Simon says:"));

  for (byte i = 0; i < 5; i++) {
    simon_sequence[i] = 0;
  }

  simon_length = random(3) + 3;
  for (byte i = 0; i < simon_length; i++) {
    simon_sequence[i] = random(4);
    Serial.println(simon_sequence[i]);
  }
}

void keypad_setup() {

  byte col_select = B00000000;
  byte symbols_picked = 0;
  byte this_pick;

  keypad_column_choice = random(6);
  Serial.print(F("Keypad column: "));
  Serial.println(keypad_column_choice);

  switch (keypad_column_choice) {
    case 0:
      symbol_index[0] = 1;
      symbol_index[1] = 2;
      symbol_index[2] = 3;
      symbol_index[3] = 4;
      symbol_index[4] = 5;
      symbol_index[5] = 6;
      symbol_index[6] = 7;
      break;
    case 1:
      symbol_index[0] = 8;
      symbol_index[1] = 1;
      symbol_index[2] = 7;
      symbol_index[3] = 9;
      symbol_index[4] = 10;
      symbol_index[5] = 6;
      symbol_index[6] = 11;
      break;
    case 2:
      symbol_index[0] = 12;
      symbol_index[1] = 13;
      symbol_index[2] = 9;
      symbol_index[3] = 14;
      symbol_index[4] = 15;
      symbol_index[5] = 3;
      symbol_index[6] = 11;
      break;
    case 3:
      symbol_index[0] = 16;
      symbol_index[1] = 17;
      symbol_index[2] = 18;
      symbol_index[3] = 5;
      symbol_index[4] = 14;
      symbol_index[5] = 11;
      symbol_index[6] = 19;
      break;
    case 4:
      symbol_index[0] = 20;
      symbol_index[1] = 19;
      symbol_index[2] = 18;
      symbol_index[3] = 21;
      symbol_index[4] = 17;
      symbol_index[5] = 22;
      symbol_index[6] = 23;
      break;
    case 5:
      symbol_index[0] = 16;
      symbol_index[1] = 8;
      symbol_index[2] = 24;
      symbol_index[3] = 25;
      symbol_index[4] = 20;
      symbol_index[5] = 26;
      symbol_index[6] = 27;
      break;
  }

  while (symbols_picked < 4) {
    this_pick = keypad_picker(col_select);
    if (this_pick < 7) {
      keypad_symbols[symbols_picked] = this_pick;
      bitSet(col_select, this_pick);
      symbols_picked++;
    }
  }
  Serial.print(keypad_symbols[0]);
  Serial.print(" ");
  Serial.print(keypad_symbols[1]);
  Serial.print(" ");
  Serial.print(keypad_symbols[2]);
  Serial.print(" ");
  Serial.println(keypad_symbols[3]);

  byte greater_number;
  for (byte i = 0; i < 4; i++) { // i is the element we are looking at
    greater_number = 0;
    for (byte j = 0; j < 4 ; j++) { // j is the other element we are comparing against
      if (keypad_symbols[i] > keypad_symbols[j]) {
        greater_number++;
      }
    }
    keypad_order[i] = greater_number;
  }

  byte symbol_mat[][8] = {{B01110, B10001, B10001, B10001, B01110, B00100, B00100, B00000},
    {B00100, B01010, B10001, B11111, B10101, B10101, B10101, B00000},
    {B11000, B00110, B01100, B00100, B01010, B10010, B10011, B00000},
    {B10000, B01001, B10011, B10101, B11001, B10010, B00001, B00000},
    {B10111, B10101, B10010, B11110, B10010, B10111, B10111, B00000},
    {B00000, B10001, B01010, B01110, B01010, B01001, B10010, B00000},
    {B01110, B10001, B00001, B00101, B00001, B10001, B01110, B00000},
    {B01010, B00000, B01110, B00001, B00111, B00001, B01110, B00000},
    {B01000, B10010, B10101, B10101, B10101, B10010, B01101, B00000},
    {B00100, B00100, B11011, B10001, B01010, B01110, B10001, B00000},
    {B00100, B00000, B00100, B01000, B10000, B10001, B01110, B00000},
    {B01110, B10001, B10111, B11001, B10111, B10001, B01110, B00000},
    {B00100, B01010, B00000, B10101, B10001, B10101, B01010, B00000},
    {B10101, B10101, B01110, B00100, B01110, B10101, B10101, B00000},
    {B01100, B10010, B00010, B01100, B00010, B00010, B00001, B00000},
    {B00000, B01111, B10000, B11110, B10001, B10001, B01110, B00000},
    {B01111, B10011, B10011, B01111, B00011, B00011, B00011, B00000},
    {B01000, B11100, B01000, B01110, B01001, B01001, B01110, B00000},
    {B01010, B00000, B10001, B11110, B00100, B01010, B00100, B00000},
    {B10101, B10101, B10101, B10101, B01110, B00100, B01110, B00000},
    {B01110, B10001, B10000, B10100, B10000, B10001, B01110, B00000},
    {B01010, B01110, B10001, B00110, B00001, B11110, B01100, B00000},
    {B00100, B00100, B11111, B11111, B01110, B01110, B10001, B00000},
    {B00100, B00100, B11111, B00100, B11111, B00100, B00100, B00000},
    {B00000, B11010, B00101, B01111, B10100, B10101, B01010, B00000},
    {B01010, B00100, B10001, B10011, B10101, B11001, B10001, B00000},
    {B00000, B01110, B10001, B10001, B10001, B01010, B11011, B00000}
  };

  byte this_symbol[8];
  for (byte i = 0; i < 4; i++) {
    for (byte j = 0; j < 8; j++) {
      this_symbol[j] = symbol_mat[symbol_index[keypad_symbols[i]] - 1][j];
    }
    lcd.createChar(i + 1 , this_symbol);
  }

}

char char_generator(bool letter) {
  byte temp;
  if (letter == 0) {
    temp = (random(10) + 48);
    return temp;
  } else {
    temp = (random(25) + 65);
    if (temp == 65 || temp == 69 || temp == 73 || temp == 79 || temp == 85) {
      serial_vowel = 1;
      if (temp == 79) {
        temp = 69;
      }
    } else if (temp == 89) {
      temp = 90;
    }
    return temp;

  }
}

void wire_deleter() {
  byte looking_at_wire;
  looking_at_wire = random(6); // pick a number from 0-5 to choose as a blank space instead of a wire.
  if (bitRead(wire_select, looking_at_wire) == 1) { //if this wire is here
    bitClear(wire_select, looking_at_wire);
  }
}

byte keypad_picker(byte col_select) {
  byte looking_at_symbol;
  byte results;
  looking_at_symbol = random(7); // pick a number from 0-6 to choose from the symbol column.
  if (bitRead(col_select, looking_at_symbol) == 0) { //if this symbol hasn't been picked yet
    //    bitSet(col_select, looking_at_symbol);
    results = looking_at_symbol;
  } else {
    results = 7;
  }
  //  Serial.println(col_select, BIN);
  //  Serial.println(looking_at_symbol);
  return results;
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
      //      for (int i = 0; i < 4; i++) {
      //        timestr[i] = "0";
      //      }
      timestr[0] = '-';
      timestr[1] = '-';
      timestr[2] = '-';
      timestr[3] = '-';
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
    timerdisp.drawColon(true);
    timerdisp.writeDisplay();

    // Serial.println(timestr);

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
  //  if (timeleft == 0) {
  //    noTone(pin_buzzer);
  //  } else

  //  Serial.println(timeleft);
  if (buzzerinterrupt < thismillis) { // Time now ticked over beyond end of buzzer interrupt

    //      digitalWrite(module_master[1][2], LOW);
    //      digitalWrite(module_master[2][2], LOW);

    // Reset the red module warning light
    //    bitWrite(shiftout_master[3], 2, 0);
    //    bitWrite(shiftout_master[3], 4, 0);
    //    bitWrite(shiftout_master[3], 6, 0);
    digitalWrite(module_master[1][2], LOW);
    digitalWrite(module_master[2+][2], LOW);
    digitalWrite(module_master[3][2], LOW);

    //    if (sec_tick_over != (int)((timeleft % 1000) / 100) + 48 && sec_tick_over == 48) { // If the tenth of the second number has changed from a 9 to 0...
    //      buzz_timer = thismillis;
    //      tone(pin_buzzer, 2093, 100); // Low tone on start of new second
    //
    //    } else if (thismillis - buzz_timer > 750) {
    //      tone(pin_buzzer, 3520, 100); // High tone near end of current second
    //      buzz_timer = thismillis;
    //    }
    //  sec_tick_over = (int)((timeleft % 1000) / 100) + 48;

    if ((timeleft % 1000) < 50) {
      tone(pin_buzzer, 3520);
      //      Serial.println(":Beep");
    }
    else if ((timeleft % 1000) >= 750 && (timeleft % 1000) < 900) {
      tone(pin_buzzer, 2093);
      //      Serial.println(":Boop");
    }
    else {
      noTone(pin_buzzer);
      //      Serial.println(" ");
    }
  }
}


void simon_lights(byte colour) {
  int freq_buzz;
  const byte simon_led_on_time = 750; // Time LED will light up

  if (colour < 4) { // Light up this colour
    simon_led_timing = thismillis + simon_led_on_time; // Next time to flag is when the light is due to turn off
    switch (colour) {
      case 0:
        //        digitalWrite(simon_led_r, HIGH);
        bitWrite(shiftout_master[module_master[3][6]], 0, 1);
        freq_buzz = 494;
        break;
      case 1:
        //        digitalWrite(simon_led_b, HIGH);
        bitWrite(shiftout_master[module_master[3][6]], 1, 1);
        freq_buzz = 587;
        break;
      case 2:
        //        digitalWrite(simon_led_y, HIGH);
        bitWrite(shiftout_master[module_master[3][6]], 2, 1);
        freq_buzz = 698;
        break;
      case 3:
        //        digitalWrite(simon_led_g, HIGH);
        bitWrite(shiftout_master[module_master[3][6]], 3, 1);
        freq_buzz = 880;
        break;
    }
    if (buzzerinterrupt <= thismillis) { // Buzzer is not being interrupted already
      buzzerinterrupt = thismillis + simon_led_on_time;
      tone(pin_buzzer, freq_buzz, simon_led_on_time);
    }
  } else { // Turn lights off
    //    digitalWrite(simon_led_r, LOW);
    //    digitalWrite(simon_led_b, LOW);
    //    digitalWrite(simon_led_y, LOW);
    //    digitalWrite(simon_led_g, LOW);
    bitWrite(shiftout_master[module_master[3][6]], 0, 0);
    bitWrite(shiftout_master[module_master[3][6]], 1, 0);
    bitWrite(shiftout_master[module_master[3][6]], 2, 0);
    bitWrite(shiftout_master[module_master[3][6]], 3, 0);
  }
}

void simonupdate() {
  //Red tone = 494Hz (B4)
  //Blue tone = 587 (D4)
  //Yellow tone = 698 (F4)
  //Green tone = 880 (A5)
  const byte simon_led_on_time = 750; // Time LED will light up
  const byte simon_led_off_time = 250; // Time between LEDs lighting
  const long simon_repeat_time = 4000; // Time between LED sequence repeats
  const long simon_advance_time = 2000; // Time between finishing previous input and displaying next stage
  const long simon_reset_time = 4000; // Time between last user input and LEDs repeating sequence

  //  Serial.print("Display step: ");
  //  Serial.println(simon_disp_step);

  // if (module_master[3][1] == 0) { // Is this module not disarmed yet?
  if (simon_led_timing <= thismillis) { // Due for the next led to light
    if (simon_disp_step > 9) { // Display step > 9 means this is between flashes
      simon_disp_step = simon_disp_step - 10; // Transition step under 10 and so directly stating which step of the sequence it is lighting up
      //      if (buzzerinterrupt > thismillis) { // Buzzer is being interrupted already
      //        simon_buzz = false;
      //      } else {
      //        buzzerinterrupt = thismillis + simon_led_on_time;
      //        simon_buzz = true;
      //      }
      //      simon_led_timing = thismillis + simon_led_on_time; // Next time to flag is when the light is due to turn off
      if (simon_disp_step < 5) { // Display of sequence
        simon_step = 0;
        simon_lights(simon_sequence[simon_disp_step]);
      } else { // Player interacting with buttons
        switch (shiftin_master[1] % 16) {
          case 1:
            simon_lights(0);
            break;
          case 2:
            simon_lights(1);
            break;
          case 4:
            simon_lights(2);
            break;
          case 8:
            simon_lights(3);
            break;
        }
      }
    } else if (simon_disp_step < 9) { // Display step currently lighting up a button
      if (simon_disp_step == simon_stage) { // This is the last light in the sequence
        simon_disp_step = 10; // Stage 10 puts in preparation for starting the sequence over next time.
        simon_led_timing = thismillis + simon_repeat_time;
      } else if (simon_disp_step < simon_stage) { // Progressing through steps...
        simon_disp_step = simon_disp_step + 11; // In waiting for next light
        simon_led_timing = thismillis + simon_led_off_time;
      } else { // Holding for player pressing buttons
        if (simon_step == 0 || simon_step == 5) {
          simon_led_timing = thismillis + simon_advance_time;
        } else {
          simon_led_timing = thismillis + simon_reset_time;
        }
        simon_disp_step = 10;
        if (module_master[3][1] == 1) { // Module disarmed
          simon_disp_step = 9; // State never checked for...
        }
      }
      simon_lights(4);
    }
  }
  //}
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
        //          lcd.setLED1Pin(LOW);
        //          lcd.setLED2Pin(LOW);
        digitalWrite(module_master[0][2], 0);
        digitalWrite(module_master[0][3], 0);
        break;
      case 1:
        //          lcd.setLED1Pin(HIGH);
        //          lcd.setLED2Pin(LOW);
        digitalWrite(module_master[0][2], 1);
        digitalWrite(module_master[0][3], 0);
        break;
      case 2:
        blinktime = blinktime + delta_t;
        if (blinktime > blinkperiod) {
          if (blinkbool) {
            //              lcd.setLED1Pin(LOW);
            //              lcd.setLED2Pin(LOW);
            digitalWrite(module_master[0][2], 0);
            digitalWrite(module_master[0][3], 0);
            blinkbool = false;
          } else {
            //              lcd.setLED1Pin(HIGH);
            //              lcd.setLED2Pin(HIGH);
            digitalWrite(module_master[0][2], 1);
            digitalWrite(module_master[0][3], 1);
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
      //      lcd.setLED1Pin(LOW);
      //      lcd.setLED2Pin(LOW);
    } else {
      game_lose(module_index);
    }
  }
  //  lcd.shift595();

}

void game_lose(byte module_index) {
  lcd.clear();
  lcd.setCursor(3, 0);
  lcd.print(F("EXPLODED!"));
  lcd.setCursor(0, 1);
  lcd.print(F("Cause: "));
  lcd.print(module_labels[module_master[module_index][0]]);
  timerdisp.print(10000);
  timerdisp.writeDisplay();
  tone(pin_buzzer, 110, 1500);
  delay(15000);
  gamemode = 0;
}

void moduledefuzed(byte module_index) {
  module_master[module_index][1] = 1;
      digitalWrite(module_master[module_index][3], HIGH);
//  bitWrite(shiftout_master[1], (module_index * 2 + 1), 1);
  Serial.print(F("Defused "));
  Serial.println(module_labels[module_master[module_index][0]]);
  Serial.println(module_master[module_index][3]);
  Serial.println(module_index);
}

void shift_out() {
  //  digitalWrite(shiftout_master[4], HIGH);
  digitalWrite(shiftout_master[5], HIGH);
  digitalWrite(shiftout_master[4], LOW);
  shiftOut(shiftout_master[3], shiftout_master[5], MSBFIRST, shiftout_master[module_master[3][6]]);
  //  shiftOut(shiftout_master[3], shiftout_master[5], MSBFIRST, shiftout_master[1]);
  digitalWrite(shiftout_master[4], HIGH);

  //  digitalWrite(34, HIGH);
  //  digitalWrite(36, LOW);
  ////  shiftOut(38,34,LSBFIRST, 0);
  ////  shiftOut(38,34,LSBFIRST, 0);
  //  shiftOut(38,34,MSBFIRST, shiftout_master[module_master[3][6]]);
  //    digitalWrite(36, HIGH);
//
//  Serial.print(" - SO:");
//  Serial.println(shiftout_master[module_master[3][6]]);
//  Serial.print("Shiftout 5:");
//  Serial.println(shiftout_master[5]);
//  Serial.print("Shiftout 4:");
//  Serial.println(shiftout_master[4]);
//  Serial.print("Shiftout 3:");
//  Serial.println(shiftout_master[3]);

}

void checkwin() {
  if ((module_master[1][0] != 0 && module_master[1][1] == 1) && (module_master[2][0] != 0 && module_master[2][1] == 1) && (module_master[3][0] != 0 && module_master[3][1] == 1)) { // && (module_master[4][0] != 0 && module_master[4][1] == 1)

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
    //timerdisp.setBrightness(0);
    timerdisp.clear();
    timerdisp.writeDisplay();
    delay(750);
    //timerdisp.setBrightness(15);
    timerdisp.writeDigitNum(0, (int)(timestr[0] - 48));
    timerdisp.writeDigitNum(1, (int)(timestr[1] - 48));
    timerdisp.writeDigitNum(3, (int)(timestr[2] - 48));
    timerdisp.writeDigitNum(4, (int)(timestr[3] - 48));
    timerdisp.writeDisplay();
    delay(750);
    //timerdisp.setBrightness(0);
    timerdisp.clear();
    timerdisp.writeDisplay();
    delay(750);
    //timerdisp.setBrightness(15);
    timerdisp.writeDigitNum(0, (int)(timestr[0] - 48));
    timerdisp.writeDigitNum(1, (int)(timestr[1] - 48));
    timerdisp.writeDigitNum(3, (int)(timestr[2] - 48));
    timerdisp.writeDigitNum(4, (int)(timestr[3] - 48));
    timerdisp.writeDisplay();
    delay(7750);
    gamemode = 0;
  }
}


//**********************************************************************
// FUNCTIONS: Input Handlers
//**********************************************************************

void checkinputs() {

  check_shiftin();
  check_button();
  check_swires();
  check_simon();
    check_keypad();
}

void check_shiftin() {
  //  digitalWrite(shiftin_master[4], HIGH);
  digitalWrite(shiftin_master[5], HIGH);
  digitalWrite(shiftin_master[4], LOW);
  shiftin_master[1] = shiftIn(shiftin_master[3], shiftin_master[5], LSBFIRST);
  digitalWrite(shiftin_master[4], HIGH);
//  Serial.print("SI:");
//  Serial.print(shiftin_master[1]);
  //  shiftin_master[2] = shiftIn(shiftin_master[3], shiftin_master[5], LSBFIRST);
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

  //      Serial.print(track_button_interaction == true);
  //      Serial.print(":");
  //      Serial.print(track_button_state == HIGH);
  //      Serial.print(":");
  //      Serial.println(thismillis - lastDebounceTime);
  //      Serial.println(isthisapress);

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
            strip_colour = random(4); // If short push needed but long hold is released, colour is random and can change every attempt
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
  register_inputs = shiftIn(module_master[2][4], module_master[2][6], MSBFIRST);
  register_inputs = register_inputs >> 2;

  byte byte_compare;

  if (register_inputs != track_wire_state) { // change in inputs
    if (register_inputs < track_wire_state || gamemode != 1) { // Omit anything where a wire is reconnected!
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
      track_wire_state = register_inputs; // Update the tracked state so long as a connection hasn't rejoined
    }
  }
}

void check_simon() { //SIMON SAYS CHECK
  byte correct_seq[4];
  byte this_colour;
  const long simon_reset_time = 4000; // Time between last user input and LEDs repeating sequence

  //  Serial.print(simon_stage);
  //  Serial.print(":");
  //  Serial.println(simon_step);

  if (simon_button_track == false) { // If a button wasn't pressed last iteration...
    if (shiftin_master[1] % 16 > 0) { // Something has now been pressed...
      simon_disp_step = 16;
      if (serial_vowel == true) {
        if (strikenumber == 0) {
          correct_seq[0] = 1;
          correct_seq[1] = 0;
          correct_seq[2] = 3;
          correct_seq[3] = 2;
        } else if (strikenumber == 1) {
          correct_seq[0] = 2;
          correct_seq[1] = 3;
          correct_seq[2] = 0;
          correct_seq[3] = 1;
        } else {
          correct_seq[0] = 3;
          correct_seq[1] = 0;
          correct_seq[2] = 1;
          correct_seq[3] = 2;
        }
      } else {
        if (strikenumber == 0) {
          correct_seq[0] = 1;
          correct_seq[1] = 2;
          correct_seq[2] = 0;
          correct_seq[3] = 3;
        } else if (strikenumber == 1) {
          correct_seq[0] = 0;
          correct_seq[1] = 1;
          correct_seq[2] = 3;
          correct_seq[3] = 2;
        } else {
          correct_seq[0] = 2;
          correct_seq[1] = 3;
          correct_seq[2] = 0;
          correct_seq[3] = 1;
        }
      }
      simon_button_track = true;
      if (shiftin_master[1] % 16 == 1) { //R
        this_colour = 0;
      } else if (shiftin_master[1] % 16 == 2) { //B
        this_colour = 1;
      } else if (shiftin_master[1] % 16 == 4) { //Y
        this_colour = 2;
      } else if (shiftin_master[1] % 16 == 8) { //G
        this_colour = 3;
      }

      if (correct_seq[simon_sequence[simon_step]] == this_colour) { //Right colour pressed
        Serial.println("Right colour!");
        simon_disp_step = 16;
        simon_led_timing = thismillis;
        if (simon_step == simon_stage) { //Last button press for this sequence
          simon_stage ++;
          simon_step = 0;
          if (simon_stage == simon_length) { //All stages complete
            moduledefuzed(3);
          }
        } else { // Not last press for this stage
          simon_step++;
        }
      } else {
        updatestrikes(3);
        simon_disp_step = 15;
        simon_led_timing = thismillis;
        simon_step = 0;
      }
    }
  } else { // Button was pressed previously
    if (shiftin_master[1] % 16 == 0) { // Now nothing pressed
      simon_button_track = false;
    }
  }
}

void check_keypad() { // KEYPAD CHECK

  byte this_button_press;
  Serial.print("Shift: ");
  Serial.println(shiftin_master[1]);
  Serial.print("Keypad: ");
  Serial.println(byte(shiftin_master[1] / 16));

  if (module_master[4][1] == 0) { // If the module isn't defused

    if (byte(shiftin_master[1] / 16) != 0  ) { // and something has been pressed now...
      if (!keypad_track_state) { // and if nothing was pressed last check...
        keypad_track_state = true;
        switch (byte(shiftin_master[1] / 16)) {
          case 1:
            this_button_press = 0;
            break;
          case 2:
            this_button_press = 1;
            break;
          case 4:
            this_button_press = 2;
            break;
          case 8:
            this_button_press = 3;
            break;
        }
        Serial.print(this_button_press);
        Serial.print(":");
        Serial.print(keypad_order[this_button_press]);
        Serial.print(":");
        Serial.println(keypad_stage);

        if (keypad_order[this_button_press] == keypad_stage) {
          keypad_stage++;
          Serial.println("Right button!");
        } else if (keypad_order[this_button_press] > keypad_stage) {
          updatestrikes(4);
          Serial.println("Wrong button...");
        }
        if (keypad_stage == 4) {
          moduledefuzed(4);
        }

      }
    } else { // and nothing is being pressed now
      keypad_track_state = false;
    }
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
  init_timer();
  init_shiftin();
  init_shiftout();
  init_button();
  init_swires();
  init_simon();
  //    init_keypad();
}

void init_timer() {
  // Manually set timer
  module_master[0][0] = 1;
  module_master[0][1] = 2;
  module_master[0][2] = 12;
  module_master[0][3] = 13;
  module_master[0][4] = 0;
  module_master[0][5] = 40;
  module_master[0][6] = 42;
  pinMode(module_master[0][1], OUTPUT);
  pinMode(module_master[0][2], OUTPUT);
  pinMode(module_master[0][3], OUTPUT);
  pinMode(module_master[0][4], OUTPUT);
  pinMode(module_master[0][5], OUTPUT);
  pinMode(module_master[0][6], OUTPUT);

  digitalWrite(module_master[0][2],LOW);
  digitalWrite(module_master[0][3],LOW);

  pinMode(pin_buzzer, OUTPUT);
  digitalWrite(pin_buzzer, LOW); //Buzzer
  timerdisp.begin(0x70); // Timer uses I2C pins @ 0x70
  timerdisp.print(10000);
  timerdisp.writeDisplay();

  pinMode(pin_lcd_rs, OUTPUT);
  pinMode(pin_lcd_e, OUTPUT);
  pinMode(pin_lcd_d4, OUTPUT);
  pinMode(pin_lcd_d5, OUTPUT);
  pinMode(pin_lcd_d6, OUTPUT);
  pinMode(pin_lcd_d7, OUTPUT);
  lcd.begin(16, 2);
}

void init_shiftin() {
  shiftin_master[0] = 0; // Reference number;
  shiftin_master[1] = 0; // Data retrieved from register 1
  shiftin_master[2] = 0; // Data retrieved from register 2
  shiftin_master[3] = 9; // Data
  shiftin_master[4] = 10; // Latch
  shiftin_master[5] = 11; // Clock
  pinMode(shiftin_master[3], INPUT); // Data pin
  pinMode(shiftin_master[4], OUTPUT); // Latch pin
  pinMode(shiftin_master[5], OUTPUT); // Clock pin
}

void init_shiftout() {
  shiftout_master[0] = 0; // Reference number;
  shiftout_master[1] = 0; // Data sent to register 1
  shiftout_master[2] = 0; // Data sent to register 2
  shiftout_master[3] = 38; // Data
  shiftout_master[4] = 36; // Latch
  shiftout_master[5] = 34; // Clock
  pinMode(shiftout_master[3], OUTPUT); // Data pin
  pinMode(shiftout_master[4], OUTPUT); // Latch pin
  pinMode(shiftout_master[5], OUTPUT); // Clock pin
}

void init_button() {
  // Manually set button
  module_master[1][0] = 2;
  module_master[1][1] = 0;
  module_master[1][2] = 23;
  module_master[1][3] = 25;
  module_master[1][4] = 33; // Button input
  module_master[1][5] = 27; // LED R
  module_master[1][6] = 29; // LED G
  module_master[1][7] = 31; // LED B
  pinMode(module_master[1][2], OUTPUT);
  pinMode(module_master[1][3], OUTPUT);

  pinMode(module_master[1][4], INPUT_PULLUP); //Button
  pinMode(module_master[1][5], OUTPUT);
  digitalWrite(module_master[1][5], LOW); //RGB LED R
  pinMode(module_master[1][6], OUTPUT);
  digitalWrite(module_master[1][6], LOW); //RGB LED G
  pinMode(module_master[1][7], OUTPUT);
  digitalWrite(module_master[1][7], LOW); //RGB LED B
}

void init_swires() {
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

  pinMode(module_master[2][4], INPUT); // Data pin
  pinMode(module_master[2][5], OUTPUT); // Latch pin
  pinMode(module_master[2][6], OUTPUT); // Clock pin
}

void init_simon() {
  // Manually set Simon
  module_master[3][0] = 5;
  module_master[3][1] = 0;
  module_master[3][2] = 46;
  module_master[3][3] = 44;
  module_master[3][4] = 1; // Shift-in register reference number
  module_master[3][5] = 0; // Bits to count (first 4 bits)
  module_master[3][6] = 2; // Shift-out register reference number
  module_master[3][7] = 0; // Bits to count (first 4 bits)

  pinMode(simon_led_r, OUTPUT);
  pinMode(simon_led_b, OUTPUT);
  pinMode(simon_led_y, OUTPUT);
  pinMode(simon_led_g, OUTPUT);
}

void init_keypad() {
  // Manually set Keypad
  module_master[4][0] = 4;
  module_master[4][1] = 0;
  module_master[4][2] = 42;
  module_master[4][3] = 40;
  module_master[4][4] = 1; // Shift-in register reference number
  module_master[4][5] = 1; // Bits to count (last 4 bits)
  module_master[4][6] = 0; // Shift-out register reference number
  module_master[4][7] = 1; // Bits to count (last 4 bits)

}

//void init_cwires() {
//  // Manually set Keypad
//  module_master[5][0] = 6;
//  module_master[5][1] = 0;
//  module_master[5][2] = 38;
//  module_master[5][3] = 36;
//  module_master[5][4] = 43; // Data
//  module_master[5][5] = 41; // Latch
//  module_master[5][6] = 39; // Clock
//  pinMode(module_master[5][2], OUTPUT);
//  pinMode(module_master[5][3], OUTPUT);
//
//  pinMode(module_master[5][4], INPUT); // Data pin
//  pinMode(module_master[5][5], OUTPUT); // Latch pin
//  pinMode(module_master[5][6], OUTPUT); // Clock pin
//}

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
