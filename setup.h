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

#endif //SETUP_H
