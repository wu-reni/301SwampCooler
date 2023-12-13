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

#include "myArduinoUtils.h"

const int stepsPerRevolution = 2038;
Stepper vent = Stepper(stepsPerRevolution, 25, 24, 23, 22);
#define ventPower 8
#define ventPin A0
#define THSensor 50

#define TEMP_THRESHOLD_HIGH 24
#define TEMP_THRESHOLD_LOW 22

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
volatile unsigned char* port_b = (unsigned char*) 0x25;
volatile unsigned char* ddr_b = (unsigned char*) 0x24;
volatile unsigned char* pin_b = (unsigned char*) 0x23;
#define DISABLE_MOTOR *port_b &= 0x7f
#define ENABLE_MOTOR *port_b |= 0x80


const int RS = 52, EN = 53, D4 = 50, D5 = 51, D6 = 48, D7 = 49;
LiquidCrystal lcd(RS, EN, D4, D5, D6, D7);

dht DHT;
DS3231 timer;

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
  // Set LEDs and motor fan as output
  *ddr_l |= (0x01 << 6);
  *ddr_l |= (0x01 << 7);
  *ddr_l |= (0x01 << 5);
  *ddr_l |= (0x01 << 4);
  *ddr_b |= (0x01 << 7);
  // Set start, stop, and reset buttons as input
  *ddr_e &= ~(0x01 << 3);
  *ddr_e &= ~(0x01 << 5);
  *ddr_g &= ~(0x01 << 5);

  state = DISABLED;
}

void loop() {
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
    //TODO: implement vent position logic

    if(START_PRESSED)
      state = IDLE;
    
    if(state != DISABLED)
      break;
  }
}

void execIdleState() {
  CLEAR_LEDS;
  SET_LED_GREEN;

  while(true) {
    displayDHT();

    //TODO: implement vent position logic
    
    if(DHT.temperature > TEMP_THRESHOLD_HIGH)
      state = RUNNING;

    if(STOP_PRESSED)
      state = DISABLED;

    unsigned int waterLevel = adc_read(0);
    if(waterLevel <= WATER_THRESHOLD)
      state = ERROR;
    
    if(state != IDLE)
      break;
  }
}

void execRunningState() {
  CLEAR_LEDS;
  SET_LED_BLUE;
  ENABLE_MOTOR;
  Serial.println("Motor enabled at time ");
  // TODO: print current time

  while(true) {
    displayDHT();

    //TODO: implement vent position logic

    if(DHT.temperature <= TEMP_THRESHOLD_LOW)
      state = IDLE;

    if(STOP_PRESSED)
      state = DISABLED;

    unsigned int waterLevel = adc_read(0);
    if(waterLevel <= WATER_THRESHOLD)
      state = ERROR;
    
    if(state != RUNNING)
      break;
  }

  DISABLE_MOTOR;
  Serial.println("Motor disabled at time ");
  // TODO: print current time
}

void execErrorState() {
  CLEAR_LEDS;
  SET_LED_RED;
  Serial.println("Water level is too low");

  while(true) {
    if(RESET_PRESSED)
      state = IDLE;

    if(STOP_PRESSED)
      state = DISABLED;

    if(state != ERROR)
      break;
  }
}


void displayDHT() {
  // TOOD: implement temperature and humidity LCD display
}