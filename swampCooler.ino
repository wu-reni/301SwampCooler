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

dht DHT;
DS3231 clock;

void setup() {
  Serial.begin(9600);
}

void loop() {

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