/*
 * Assignment: Swamp Cooler
 * Authors: Diego Borne, Nicholas Langley, Reni Wu
 * Class: CPE301 Spring 2023
 * Date: 12/15/2023
 */

#include <DS3231.h>
#include <dht.h>
#include <Stepper.h>
#include <LiquidCrystal.h>

// User created library for clarity
#include "myArduinoUtils.h"

const int stepsPerRevolution = 2048;
Stepper vent = Stepper(stepsPerRevolution, 34, 36, 35, 37);
#define THSensor 50

#define TEMP_THRESHOLD_HIGH 25
#define TEMP_THRESHOLD_LOW 24

#define WATER_THRESHOLD 10

// Pin 42 (PL7, Yellow LED)
// Pin 43 (PL6, Red LED)
// Pin 44 (PL5, Green LED)
// Pin 45 (PL4, Blue LED)
volatile unsigned char* port_l = (unsigned char*) 0x10B;
volatile unsigned char* ddr_l = (unsigned char*) 0x10A;
volatile unsigned char* pin_l = (unsigned char*) 0x109;
#define CLEAR_LEDS *port_l &= 0x0F;
#define SET_LED_YELLOW *port_l |= (0x01 << 7)
#define SET_LED_RED *port_l |= (0x01 << 6)
#define SET_LED_GREEN *port_l |= (0x01 << 5)
#define SET_LED_BLUE *port_l |= (0x01 << 4)

// Pin 3 (PE5, start button) 
// Pin 5 (PE3, reset button)
volatile unsigned char* port_e = (unsigned char) 0x2E;
volatile unsigned char* ddr_e = (unsigned char) 0x2D;
volatile unsigned char* pin_e = (unsigned char) 0x2C;
#define START_PRESSED *pin_e & (0x01 << 5)
#define RESET_PRESSED *pin_e & (0x01 << 3)

// Pin 4 (PG5, stop button)
volatile unsigned char* port_g = (unsigned char) 0x34;
volatile unsigned char* ddr_g = (unsigned char) 0x33;
volatile unsigned char* pin_g = (unsigned char*) 0x32;
#define STOP_PRESSED *pin_g & (0x01 << 5)

// Pin 13 (PB7, fan motor)
// Pin 12 (PB6 vent left)
// Pin 11 (PB5 vent right)
// Pin 10 (PB4 water level enable)
volatile unsigned char* port_b = (unsigned char*) 0x25;
volatile unsigned char* ddr_b = (unsigned char*) 0x24;
volatile unsigned char* pin_b = (unsigned char*) 0x23;
#define WATER_LEVEL_ON *port_b |=(0x01 << 4)
#define WATER_LEVEL_OFF *port_b &= 0xEF
#define VENT_LEFT *pin_b & (0x01 << 6)
#define VENT_RIGHT *pin_b & (0x01 << 5)
#define DISABLE_MOTOR *port_b &= 0x7f
#define ENABLE_MOTOR *port_b |= 0x80

const int RS = 22, EN = 23, D4 = 24, D5 = 25, D6 = 26, D7 = 27;
LiquidCrystal lcd(RS, EN, D4, D5, D6, D7);

dht DHT;

DS3231 timer;

unsigned long begin = 0;
const long interval = 5000;

String motorOff = "motor off";
String motorOn = "motor on";
String ventLeft = "vent left";
String ventRight = "vent right";
String stateTransition = "state changed";

enum {
  DISABLED,
  IDLE,
  RUNNING,
  ERROR
} state;

void setup() {
  U0init(9600);
  lcd.begin(16, 2);
  adc_init(); 
  //Set LEDs and motor fan as output
  *ddr_l |= (0x01 << 6);
  *ddr_l |= (0x01 << 7);
  *ddr_l |= (0x01 << 5);
  *ddr_l |= (0x01 << 4);
  *ddr_b |= (0x01 << 7);
  // Set start, stop, and reset buttons as input
  *ddr_e &= ~(0x01 << 3);
  *ddr_e &= ~(0x01 << 5);
  *ddr_g &= ~(0x01 << 5);
  //Set vent buttons as input and 
  *ddr_b &= ~(0x01 << 6);
  *ddr_b &= ~(0x01 << 5);
  //Set water level sensor as output
  *ddr_b |= (0x01 << 4);

  state = DISABLED;

  timer.setHour(0);
  timer.setMinute(0);
  timer.setSecond(0);

  vent.setSpeed(10);
}

void loop() {
  attachInterrupt(digitalPinToInterrupt(3), startInterrupt, RISING);
  switch (state) {
    case DISABLED:
      execDisabledState();
      break;
    case IDLE:
      execIdleState();
      break;
    case RUNNING:
      execRunningState();
      break;
    case ERROR:
      execErrorState();
      break;
  }
}

void execDisabledState() {
  CLEAR_LEDS;
  SET_LED_YELLOW;

  while(true) {
    lcd.clear();
    
    if(state != DISABLED){
      printMessage(stateTransition);
      displayTime();
      break;
    }
  }
}

void execIdleState() {
  CLEAR_LEDS;
  SET_LED_GREEN;

  while(true) {
    DHT.read11(THSensor);
    displayDHT();

    ventControl();
    
    if(DHT.temperature > TEMP_THRESHOLD_HIGH){
      state = RUNNING;
      printMessage(stateTransition);
      displayTime();
    }

    if(STOP_PRESSED){
      state = DISABLED;
      printMessage(stateTransition);
      displayTime();
    }

    WATER_LEVEL_ON;
    unsigned int waterLevel = adc_read(0);
    WATER_LEVEL_OFF;

    if(waterLevel <= WATER_THRESHOLD){
      state = ERROR;
      printMessage(stateTransition);
      displayTime();
    }
    
    if(state != IDLE)
      break;
  }
}

void execRunningState() {
  CLEAR_LEDS;
  SET_LED_BLUE;
  ENABLE_MOTOR;

  printMessage(motorOn);
  displayTime();

  while(true) {
    DHT.read11(THSensor);
    displayDHT();

    ventControl();

    if(DHT.temperature <= TEMP_THRESHOLD_LOW){
      state = IDLE;
      printMessage(stateTransition);
      displayTime();
    }

    if(STOP_PRESSED){
      state = DISABLED;
      printMessage(stateTransition);
      displayTime();
    }

    WATER_LEVEL_ON;
    unsigned int waterLevel = adc_read(0);
    WATER_LEVEL_OFF;
    
    if(waterLevel <= WATER_THRESHOLD){
      state = ERROR;
      printMessage(stateTransition);
      displayTime();
    }
    
    if(state != RUNNING)
      break;
  }

  DISABLE_MOTOR;
  printMessage(motorOff);
  displayTime();
}

void execErrorState() {
  CLEAR_LEDS;
  SET_LED_RED;

  while(true) {
    DHT.read11(THSensor);
    displayDHT();

    ventControl();
    
    WATER_LEVEL_ON;
    unsigned int waterLevel = adc_read(0);
    WATER_LEVEL_OFF;

    if(RESET_PRESSED && (waterLevel > WATER_THRESHOLD)){
      state = IDLE;
      printMessage(stateTransition);
      displayTime();
    }

    if(STOP_PRESSED){
      state = DISABLED;
      printMessage(stateTransition);
      displayTime();
    }
  
    if(state != ERROR)
      break;
  }
}

void displayTime(){
  bool temp;
  byte time = timer.getHour(temp, temp);
  int a = (time / 10) + 48;
  int b = (time % 10) + 48;
  U0putchar(a);
  U0putchar(b);
  U0putchar(':');
  time = timer.getMinute();
  a = (time / 10) + 48;
  b = (time % 10) + 48;
  U0putchar(a);
  U0putchar(b);
  U0putchar(':');
  time = timer.getSecond();
  a = (time / 10) + 48;
  b = (time % 10) + 48;
  U0putchar(a);
  U0putchar(b);
  U0putchar('\n');
}

void startInterrupt(){
  state = IDLE;
}

void printMessage(String message){
  for(int i = 0; i < message.length(); i++){
    U0putchar(message[i]);
  }
  U0putchar(':');
  U0putchar(' ');
}

void ventControl(){
  if(VENT_RIGHT){
    vent.step((stepsPerRevolution/4));
    printMessage(ventRight);
    displayTime();
  } else if (VENT_LEFT){
    vent.step(-(stepsPerRevolution/4));
    printMessage(ventLeft);
    displayTime();
  }
}

void displayDHT() {
  unsigned long current = millis();
  if(current - begin >= interval){
    int temperature = DHT.temperature;
    int humidity = DHT.humidity;
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.write("Temperature: ");
    lcd.print(temperature);
    lcd.write("C");
    lcd.setCursor(0, 2);
    lcd.write("Humidity: ");
    lcd.print(humidity);
    lcd.write("%");
    begin = current; 
  }
}