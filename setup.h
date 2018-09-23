/*
 * Setup/initialisation functions
 * Set filesystem, wifi, NTP server to get time
 * 
 */
#ifndef SETUP_H
#define SETUP_H

#include <Arduino.h>


void SPIFFSSetup(void);
void setupWIFI(void);
void setupNTP(void);
uint32_t NTPGetTime(void);


#endif //SETUP_H
