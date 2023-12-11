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

const int stepsPerRevolution = 2038;
Stepper vent = Stepper(stepsPerRevolution, 25, 24, 23, 22);
#define ventPower 8
#define ventPin A0
#define THSensor 50
#define TBE 0x20  

volatile unsigned char *myTCCR1A = (unsigned char *) 0x80;
volatile unsigned char *myTCCR1B = (unsigned char *) 0x81;
volatile unsigned char *myTCCR1C = (unsigned char *) 0x82;
volatile unsigned char *myTIMSK1 = (unsigned char *) 0x6F;
volatile unsigned int  *myTCNT1  = (unsigned  int *) 0x84;
volatile unsigned char *myTIFR1 =  (unsigned char *) 0x36;

volatile unsigned char *myUCSR0A = (unsigned char *)0x00C0;
volatile unsigned char *myUCSR0B = (unsigned char *)0x00C1;
volatile unsigned char *myUCSR0C = (unsigned char *)0x00C2;
volatile unsigned int  *myUBRR0  = (unsigned int *) 0x00C4;
volatile unsigned char *myUDR0   = (unsigned char *)0x00C6;
 
volatile unsigned char* my_ADMUX = (unsigned char*) 0x7C;
volatile unsigned char* my_ADCSRB = (unsigned char*) 0x7B;
volatile unsigned char* my_ADCSRA = (unsigned char*) 0x7A;
volatile unsigned int* my_ADC_DATA = (unsigned int*) 0x78;

// Pin 43 (PL6, Red LED)
// Pin 42 (PL7, Yellow LED)
// Pin 44 (PL5, Green LED)
// Pin 45 (PL4, Blue LED)
volatile unsigned char* port_l = (unsigned char*) 0x10B;
volatile unsigned char* ddr_l = (unsigned char*) 0x10A;
volatile unsigned char* pin_l = (unsigned char*) 0x109;

// Pin 3 (PE5, start button) 
// Pin 5 (PE3, reset button)
volatile unsigned char* port_e = (unsigned char) 0x2E;
volatile unsigned char* ddr_e = (unsigned char) 0x2D;
volatile unsigned char* pin_e = (unsigned char) 0x2C;
// Pin 4 (PG5, stop button)
volatile unsigned char* port_g = (unsigned char) 0x34;
volatile unsigned char* ddr_g = (unsigned char) 0x33;
volatile unsigned char* pin_g = (unsigned char*) 0x32;

// Pin 13 (PB7, fan motor)
volatile unsigned char* port_b = (unsigned char*) 0x25;
volatile unsigned char* ddr_b = (unsigned char*) 0x24;
volatile unsigned char* pin_b = (unsigned char*) 0x23;

bool start = false; 
bool error = false;
const int RS = 52, EN = 53, D4 = 50, D5 = 51, D6 = 48, D7 = 49;
LiquidCrystal lcd(RS, EN, D4, D5, D6, D7);

dht DHT;
DS3231 timer;

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
}

void loop() {
  DHT.read11(THSensor);
  //setting up ISR
  attachInterrupt(digitalPinToInterrupt(3), startInterrupt, RISING);
  //state transition times reported via serial port
  //stepper motor change reported via serial port
  if(*pin_g & (0x01 << 5)/*stop button pressed*/){
    start = false;
    Serial.println("stop pressed");
  }
  if(start){
    unsigned int analog = adc_read(0);
    Serial.println(analog);
    if(analog <= 10/*water level low*/){
      error = true;
    }
    if(error){
      Serial.println("in error");
      //ERROR
      //motor off 
      *port_b &= 0x7f;
      //display error message
      //red LED ON (all others OFF)
      *port_l |= (0x01 << 6);
      //other LED OFF
      *port_l &= 0x40;
      if((analog > 10) && (*pin_e & 0x08)/*water level high && reset pressed*/){
        error = false;
      }
    } else {
      if(DHT.temperature < 24/*temp low*/){
        //IDLE
        Serial.println("in idle");
        //motor off
        *port_b &= 0x7f;
        //green LED ON
        *port_l |= (0x01 << 5);
        //other LED OFF
        *port_l &= 0x20;
      } else {
        //RUNNING
        Serial.println("in running");
        //blue LED ON
        *port_l |= (0x01 << 4);
        //other LED OFF
        *port_l &= 0x10;
        //motor on 
        *port_b |= 0x80;
      }
    }
  } else {
    Serial.println("in disabled");
    //DISABLED
    //yellow LED ON
    *port_l |= (0x01 << 7);
    //other LED OFF
    *port_l &= 0x80;
    //motor off 
    *port_b &= 0x7f;
  }
  my_delay(1000);
}

void my_delay(unsigned int ms)
{
  // calculate ticks from ms (1024 prescaler)
  double ticks = (ms/1000)/.000064;
  // stop the timer
  *myTCCR1B &= 0xF8;
  // set the counts
  *myTCNT1 = (unsigned int) (65536 - ticks);
  // start the timer with 1024 prescaler
  * myTCCR1A = 0x0;
  * myTCCR1B |= 0b00000101;
  // wait for overflow
  while((*myTIFR1 & 0x01)==0); // 0b 0000 0000
  // stop the timer
  *myTCCR1B &= 0xF8;   // 0b 0000 0000
  // reset TOV           
  *myTIFR1 |= 0x01;
}

void U0init(int U0baud){
 unsigned long FCPU = 16000000;
 unsigned int tbaud;
 tbaud = (FCPU / 16 / U0baud - 1);
 *myUCSR0A = 0x20;
 *myUCSR0B = 0x18;
 *myUCSR0C = 0x06;
 *myUBRR0  = tbaud;
}

void adc_init(){
  *my_ADCSRA |= 0b10000000;
  *my_ADCSRA &= 0b11011111;
  *my_ADCSRA &= 0b11110111;
  *my_ADCSRA &= 0b11111000;
  *my_ADCSRB &= 0b11110111;
  *my_ADCSRB &= 0b11111000;
  *my_ADMUX  &= 0b01111111;
  *my_ADMUX  |= 0b01000000;
  *my_ADMUX  &= 0b11011111;
  *my_ADMUX  &= 0b11100000;
}

unsigned int adc_read(unsigned char adc_channel_num){
  *my_ADMUX  &= 0b11100000;
  *my_ADCSRB &= 0b11110111;
  if(adc_channel_num > 7){
    adc_channel_num -= 8;
    *my_ADCSRB |= 0b00001000;
  }
  *my_ADMUX  += adc_channel_num;
  *my_ADCSRA |= 0x40;
  while((*my_ADCSRA & 0x40) != 0);
  return *my_ADC_DATA;
}

void U0putchar(unsigned char U0pdata){
  while((*myUCSR0A & TBE)==0);
  *myUDR0 = U0pdata;
}

void startInterrupt(){
  start = true;
}