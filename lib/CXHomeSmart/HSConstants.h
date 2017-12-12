#ifndef HSCONSTANTS_H
#define HSCONSTANTS_H

#include <Arduino.h>

#define DHT_PIN             A6
#define TEMP_OK_LED_PIN		22
#define TEMP_WARN_LED_PIN	23
#define TEMP_HOT_LED_PIN	24

#define CHIP_SELECT_PIN     48
#define CARD_DETECT_PIN     49


const int8_t    DATE_REFRESH_INTERVAL   = 60;	// Refresh the date every minute.
const int8_t    TEMP_REFRESH_RATE       = 5;	// Set the temperature refreshing rate (by seconds)
const uint8_t   BUFFER_SIZE             = 20;

#endif
