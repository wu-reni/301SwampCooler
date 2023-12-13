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
volatile unsigned char* port_b = (unsigned char*) 0x25;
volatile unsigned char* ddr_b = (unsigned char*) 0x24;
volatile unsigned char* pin_b = (unsigned char*) 0x23;
#define DISABLE_MOTOR *port_b &= 0x7f
#define ENABLE_MOTOR *port_b |= 0x80


const int RS = 22, EN = 23, D4 = 24, D5 = 25, D6 = 26, D7 = 27;
LiquidCrystal lcd(RS, EN, D4, D5, D6, D7);

dht DHT;
DS3231 timer;

unsigned long begin = 0;
const long interval = 5000;

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
  timer.setHour(0);
  timer.setMinute(0);
  timer.setSecond(0);
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
    //TODO: implement vent position logic
    lcd.clear();
    
    if(state != DISABLED)
      displayTime();
      break;
  }
}

void execIdleState() {
  CLEAR_LEDS;
  SET_LED_GREEN;

  while(true) {
    DHT.read11(THSensor);
    displayDHT();

    //TODO: implement vent position logic
    
    if(DHT.temperature > TEMP_THRESHOLD_HIGH){
      state = RUNNING;
      displayTime();
    }

    if(STOP_PRESSED){
      state = DISABLED;
      displayTime();
    }

    unsigned int waterLevel = adc_read(0);
    if(waterLevel <= WATER_THRESHOLD){
      state = ERROR;
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
  Serial.println("Motor enabled at time ");
  // TODO: print current time

  while(true) {
    DHT.read11(THSensor);
    displayDHT();
    
    
    //TODO: implement vent position logic

    if(DHT.temperature <= TEMP_THRESHOLD_LOW){
      state = IDLE;
      displayTime();
    }

    if(STOP_PRESSED){
      state = DISABLED;
      displayTime();
    }

    unsigned int waterLevel = adc_read(0);
    if(waterLevel <= WATER_THRESHOLD){
      state = ERROR;
      displayTime();
    }
    
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
  DHT.read11(THSensor);
  displayDHT();

  while(true) {
    if(RESET_PRESSED){
      state = IDLE;
      displayTime();
    }

    if(STOP_PRESSED){
      state = DISABLED;
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

void displayDHT() {
  // TOOD: implement temperature and humidity LCD display
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