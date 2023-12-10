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
Stepper vent = Stepper(stepsPerRevolution, 2, 3, 4, 5);
#define ventPower 8
#define ventPin A0
#define TBE 0x20  
String waterError = "Water level is too low";
// my_delay
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
// Pin 43 setup 
volatile unsigned char* port_l = (unsigned char*) 0x10B;
volatile unsigned char* ddr_l = (unsigned char*) 0x10A;
volatile unsigned char* pin_l = (unsigned char*) 0x109;

// Pin 19 setup 
volatile unsigned char* port_d = (unsigned char*) 0x2B;
volatile unsigned char* ddr_d = (unsigned char*) 0x2A;
volatile unsigned char* pin_d = (unsigned char*) 0x29;

bool start = false; 
const int RS = 22, EN = 23, D4 = 24, D5 = 25, D6 = 26, D7 = 27;
LiquidCrystal lcd(RS, EN, D4, D5, D6, D7);

dht DHT;
DS3231 clock;

void setup() {
  U0init(9600);
  adc_init(); 
  *ddr_l |= (0x01 << 6);
  *ddr_d &= ~(0x01 << 2);
}

void loop() {
  attachInterrupt(digitalPinToInterrupt(19), startInterrupt, CHANGE);
  if(start){
    my_delay(1000);
    unsigned int analog = adc_read(0);
    if(analog <= 10){
      for(int i = 0; i < waterError.length(); i++){
        U0putchar(waterError[i]);
      }
      U0putchar('\n');
      *port_l |= (0x01 << 6); 
      my_delay(1000);
    } else {
      *port_l &= ~(0x01 << 6); 
      my_delay(1000);
    }
  } else {
    Serial.println("OFF");
    my_delay(1000);
  }

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
  delay(1000);
  start = !start; 
}