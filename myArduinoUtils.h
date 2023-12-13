#ifndef MY_ARDUINO_UTILS_H
#define MY_ARDUINO_UTILS_H

void my_delay(unsigned int ms);

void U0init(int U0baud);

void adc_init();

unsigned int adc_read(unsigned char adc_channel_num);


void U0putchar(unsigned char U0pdata);

#endif
