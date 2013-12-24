#include <Wire.h>
#include <RTClib1388.h>
#include <Adafruit_GFX.h>
#include <Adafruit_PCD8544.h>
//#include <Time.h>
//#include <TimeAlarms.h>

RTC_DS1388 rtc;

// time variables
DateTime time_last_update = 0;

int backlight_pin = 9;
int nightlight_pin = 10;
int ledstrip_pin = 5;

// led states
byte backlight_level = 0;
byte ledstrip_level = 0;
byte nightlight_level = 0;

// map key pins to a standard order (top, bottom, left, right)
// touch sensor key0 = pin 2 = bottom 
// touch sensor key1 = pin 3 = top
// touch sensor key2 = pin 14 = left
// touch sensor key3 = pin 15 = right
// touch sensor key4 = unused
#define NUM_KEY_PINS 4
int my_key_pins[4] = {3, 2, 14, 15};
// position of pin assignments in my_key_pins array
#define UP_KEY 0
#define DOWN_KEY 1
#define LEFT_KEY 2
#define RIGHT_KEY 3

// key states
unsigned long int old_key_time = millis();
int old_key = 0;

/////// nav tree /////////////////////////////////
// main screen items
#define ROOT_SCREEN 0
  #define ROOT_MANUAL_ITEM 1
  #define ROOT_SETTINGS_ITEM 0
  #define ROOT_NIGHTLIGHT_ITEM 2
// select settings screen items
#define SETTINGS_SCREEN 1
  #define SETTINGS_TIME_ITEM 0
  #define SETTINGS_ALARM_ITEM 1
  #define SETTINGS_BACKLIGHT_ITEM 2
// adjust settings screen items
#define ADJUST_TIME_SCREEN 2
  #define ADJUST_TIME_HOUR_ITEM 0
  #define ADJUST_TIME_MINUTES_ITEM 1
  #define ADJUST_TIME_SAVE_ITEM 2
// adjust settings screen items
#define ADJUST_ALARM_SCREEN 3
  #define ADJUST_ALARM_HOUR_ITEM 0
  #define ADJUST_ALARM_MINUTES_ITEM 1
  #define ADJUST_ALARM_SAVE_ITEM 2

//int screen_items[5] = {3, 3, 3, 3, 1};
int active_screen = ROOT_SCREEN;
int active_item = ROOT_SETTINGS_ITEM;
boolean update_screen = true;

// operating modes
// [0] = manual
// [1] = alarm
// [2] = nighlight
#define MANUAL_MODE 0
#define ALARM_MODE 1
#define NIGHTLIGHT_MODE 2
boolean active_mode[3] = {false, true, false};

// pin 8 - Serial clock out (SCLK)
// pin 7 - Serial data out (DIN)
// pin 6 - Data/Command select (D/C)
// none - LCD chip select (CS)
// pin 4 - LCD reset (RST)
Adafruit_PCD8544 display = Adafruit_PCD8544(8, 7, 6, 4);

///////////////////////////////////////////////////////////////////////

// the setup routine runs once when you press reset:
void setup() {                
  Serial.begin(1200);
  Wire.begin();
  display.begin();
  rtc.begin();  
  
  rtc.adjust(DateTime(__DATE__, __TIME__));
  time_last_update = rtc.now();
  
  for(int i=0; i < NUM_KEY_PINS; i++) {
    pinMode(my_key_pins[i], INPUT);
  }
  
  pinMode(backlight_pin, OUTPUT);  
  pinMode(ledstrip_pin, OUTPUT);
  pinMode(nightlight_pin, OUTPUT);
  
  analogWrite(backlight_pin, backlight_level);
  analogWrite(ledstrip_pin, ledstrip_level);
  analogWrite(nightlight_pin, nightlight_level);
  
  display.begin();
  display.setContrast(60);
  display.clearDisplay();   // clears the screen and buffer
  display.display();
  
  // @@turn on screen backlight
  digitalWrite(backlight_pin, 125);
}

/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////

void loop() {  
  ///////// check for new key press /////////////////////////////////
  int new_key = checkKeyPress();
  
  DateTime time_now = rtc.now();
  if ((time_now.second() - time_last_update.second()) >= 1) {
    update_screen = true;
    time_last_update = time_now;
  }
  
  ///////// Root Screen /////////////////////////////////////////////
  if (active_screen == ROOT_SCREEN) {              
    if (active_item == ROOT_SETTINGS_ITEM) {
      
      if (new_key == my_key_pins[UP_KEY]) { // top button pressed, goto select screen
        active_screen = SETTINGS_SCREEN;
      } else if (new_key == my_key_pins[LEFT_KEY]) { // left button pressed, goto manual item
        active_item = ROOT_MANUAL_ITEM;
      } else if (new_key == my_key_pins[RIGHT_KEY]) { // right button pressed, goto nightlight
        active_item = ROOT_NIGHTLIGHT_ITEM;
      }            
  
      // update the screen if first loop, minutes changed, or arrived from another screen/item
      if (update_screen) {
        display.clearDisplay();
        display.display();
        
        display.setCursor(0,0);
        display.setTextColor(BLACK);
        display.setTextSize(1);
        display.println("- Main Menu -");
        
        display.setTextSize(2);
        display.print(time_now.hour(), DEC);        
        if ((time_now.second() % 2) == 0) {
          display.print(":"); 
        } else {
          display.print(" ");
        }           
        display.println(time_now.minute());  
        
        display.setTextSize(1);  
        if (active_mode[ALARM_MODE]) {
          display.println("( ( Alarm ) )"); 
        } else {
          display.println("");
        }
        
        display.println("");
        
        display.print("ON-");
        display.setTextColor(WHITE, BLACK); // inverted text
        display.print(" SET ");
        display.setTextColor(BLACK);
        display.println("-NITE");
        display.display();
        
        update_screen = false;
      }
      
      if ((new_key == my_key_pins[UP_KEY]) || (new_key == my_key_pins[LEFT_KEY]) || (new_key == my_key_pins[RIGHT_KEY])) {
        update_screen = true;
      }
    ///////// Manual Item /////////////////////////////////////////////  
    } else if (active_item == ROOT_MANUAL_ITEM) {
     
        if (new_key == my_key_pins[UP_KEY]) {
          active_mode[0] = true; // ledstrip is in manual mode
          if ((ledstrip_level + 5) <= 255) {
             ledstrip_level = ledstrip_level + 5;
             analogWrite(ledstrip_pin, ledstrip_level);
          }
        } else if (new_key == my_key_pins[DOWN_KEY]) { 
           if ((ledstrip_level - 5) >= 0) {
             ledstrip_level = ledstrip_level - 5;
             analogWrite(ledstrip_pin, ledstrip_level);
           }
           
           if (ledstrip_level == 0) {
             active_mode[0] = false; // manual_mode is false if pwm at zero
           }
        } else if (new_key == my_key_pins[LEFT_KEY]) { // left button pressed, goto nightlight item      
          active_item = ROOT_NIGHTLIGHT_ITEM;
        } else if (new_key == my_key_pins[RIGHT_KEY]) { // right button pressed, goto setting item   
          active_item = ROOT_SETTINGS_ITEM;
        }
        
        // update the screen if first loop, minutes changed, or arrived from another screen/item
        if (update_screen) {
          display.clearDisplay();
          display.display();
          
          display.setCursor(0,0);
          display.setTextColor(BLACK);
          display.setTextSize(1);
          display.println("- Main Menu -");
          
          display.setTextSize(2);
          display.print(time_now.hour(), DEC);        
          if ((time_now.second() % 2) == 0) {
            display.print(":"); 
          } else {
            display.print(" ");
          }           
          display.println(time_now.minute());  
          
          display.setTextSize(1);  
          if (active_mode[ALARM_MODE]) {
            display.println("( ( Alarm ) )"); 
          } else {
            display.println("");
          }
          
          display.print(ledstrip_level);
          display.println("/255");
          
          display.setTextColor(WHITE, BLACK); // inverted text
          display.print(" ON "); 
          display.setTextColor(BLACK);
          display.print("-SET");
          display.println("-NITE");
          display.display();
          
          update_screen = false;
        }
        
        if (new_key != 0) {
          update_screen = true;
        }     
    
    ///////// Nightlight Item /////////////////////////////////////////////    
    } else if (active_item == ROOT_NIGHTLIGHT_ITEM) {  
      active_mode[2] = true; // nighlight is on

      if (new_key == my_key_pins[UP_KEY]) {    
         if ((nightlight_level + 5) <= 255) {
           nightlight_level = nightlight_level + 5;
           analogWrite(nightlight_pin, nightlight_level);
         }
      } else if (new_key == my_key_pins[DOWN_KEY]) {    
         if ((nightlight_level - 5) >= 0) {
           nightlight_level = nightlight_level - 5;
           analogWrite(nightlight_pin, nightlight_level);
         }
      } else if (new_key == my_key_pins[LEFT_KEY]) {         
        active_item = ROOT_SETTINGS_ITEM;
      }  else if (new_key == my_key_pins[RIGHT_KEY]) {         
        active_item = ROOT_MANUAL_ITEM;
      }
      
      // update the screen if first loop, minutes changed, or arrived from another screen/item
      if (update_screen) {
        display.clearDisplay();
        display.display();
        
        display.setCursor(0,0);
        display.setTextColor(BLACK);
        display.setTextSize(1);
        display.println("- Main Menu -");
        
        display.setTextSize(2);
        display.print(time_now.hour(), DEC);        
        if ((time_now.second() % 2) == 0) {
          display.print(":"); 
        } else {
          display.print(" ");
        }           
        display.println(time_now.minute());  
        
        display.setTextSize(1);  
        if (active_mode[ALARM_MODE]) {
          display.println("( ( Alarm ) )"); 
        } else {
          display.println("");
        }
        
        display.print(nightlight_level);
        display.println("/255");
          
        display.print("ON"); 
        display.print("-SET-");
        display.setTextColor(WHITE, BLACK); // inverted text
        display.println(" NITE ");
        display.display();
    
        update_screen = false;    
      }
      
       if (new_key != 0) {
        update_screen = true;
      }
      
    } // end of menu 0 item 2
    
  ///////// Settings Screen /////////////////////////////////////////////
  } else if (active_screen == SETTINGS_SCREEN) {

    if (active_item == SETTINGS_TIME_ITEM) {
      
     if (new_key == my_key_pins[UP_KEY]) { // top button pressed, goto select screen
        active_item = ADJUST_TIME_HOUR_ITEM;
      } else if (new_key == my_key_pins[LEFT_KEY]) { // left button pressed, goto manual item
        active_screen = ROOT_SCREEN;
      } else if (new_key == my_key_pins[RIGHT_KEY]) { // right button pressed, goto nightlight
        active_item = ADJUST_ALARM_SCREEN;
      }            
  
      // update the screen if first loop, minutes changed, or arrived from another screen/item
      if (update_screen) {
        display.clearDisplay();
        display.display();
        
        display.setCursor(0,0);
        display.setTextColor(BLACK);
        display.setTextSize(1);
        display.println("- Settings -");
        
        display.println("Adjust Time");
        display.print(time_now.hour(), DEC);        
        if ((time_now.second() % 2) == 0) {
          display.print(":"); 
        } else {
          display.print(" ");
        }           
        display.println(time_now.minute());  
        
        display.print(" << ");
        display.setTextColor(WHITE, BLACK); // inverted text
        display.print(" SET ");
        display.setTextColor(BLACK);
        display.println(" >> ");
        display.display();
        
        update_screen = false;
      }
      
      if ((new_key == my_key_pins[UP_KEY]) || (new_key == my_key_pins[LEFT_KEY]) || (new_key == my_key_pins[RIGHT_KEY])) {
        update_screen = true;
      }
      
  ////////// Adjust Time Hours Item //////////////////////////////////
  
  
  
  
  ////@@@@@@@@@ THIS NEEDS TO BE THE NEXT SCREEN NOT AN ITEM IN THE ABOVE@@@@@@@@@ 
  
  
  
    } else if (active_item == ADJUST_TIME_HOUR_ITEM) {
      
      if (new_key == my_key_pins[UP_KEY]) { // top button pressed, goto select screen

        
      } else if (new_key == my_key_pins[DOWN_KEY]) {
        
      
      } else if (new_key == my_key_pins[LEFT_KEY]) { // left button pressed, goto manual item
        active_screen = SETTINGS_SCREEN;
      } else if (new_key == my_key_pins[RIGHT_KEY]) { // right button pressed, goto nightlight
        active_item = ADJUST_TIME_MINUTES_ITEM;
      }            
  
      // update the screen if first loop, minutes changed, or arrived from another screen/item
      if (update_screen) {
        display.clearDisplay();
        display.display();
        
        display.setCursor(0,0);
        display.setTextColor(BLACK);
        display.setTextSize(1);
        display.println("- Set Time -");
        
        display.println("Adjust Hour");
        display.setTextSize(2);
        display.print(time_now.hour(), DEC);        
        if ((time_now.second() % 2) == 0) {
          display.print(":"); 
        } else {
          display.print(" ");
        }           
        display.println(time_now.minute());  
        
        display.setTextSize(1);
        display.print(" << ");
        display.setTextColor(WHITE, BLACK); // inverted text
        display.print(" ADJ ");
        display.setTextColor(BLACK);
        display.println(" >> ");
        display.display();
        
        update_screen = false;
      }
      
      if (new_key != 0) {
        update_screen = true;
      }
  } 
 } 
} // end of main loop

//////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////

int checkKeyPress() {
  int key_pressed = 0;
  int key_interval = 375; // minimum interval between registered key presses

  if ((millis() - old_key_time) > key_interval) {    
    for (int i=0; i < NUM_KEY_PINS; i++) {
      if (digitalRead(my_key_pins[i]) == LOW) {
        //key_pressed = i + 1;
        key_pressed = my_key_pins[i];
      }
    } 

    if (key_pressed) {
      old_key = key_pressed; // set global
      old_key_time =  millis(); // set global
    }
  } 
  return key_pressed;
}