#include <stdlib.h>
#include <math.h>
#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include "RTClib.h"
#include "SD.h"
#include "Adafruit_GFX.h"
#include "MCUFRIEND_kbv.h"
#include "UTFTGLUE.h"
#include "dht.h"
#include "HSConstants.h"
#include "Fonts/FreeSans9pt7b.h"
#include "Fonts/FreeMono9pt7b.h"

UTFTGLUE		myGLCD(0x9488,A2,A1,A3,A4,A0);
MCUFRIEND_kbv 	kbv;
RTC_DS1307 		rtc;
DateTime	 	now;


int8_t date_interval_counter = 1;
int8_t last_minute = 0;
int8_t curr_minute = 0;

int8_t temp_interval_counter = 1;

float last_temp_reading = 0.00;

char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

// Declare which fonts we will be using
#if !defined(BigFont)
extern uint8_t BigFont[];    //.kbv GLUE defines as GFXFont ref
#endif

void writeFileHeader(void);
void writeData(void);
void printFileContents(void);
void initializeCard(void);

File fd;
char fileName[] = "readings.txt"; // SD library only supports up to 8.3 names
char buff[BUFFER_SIZE+2] = "";  // Added two to allow a 2 char peek for EOF state
uint8_t index = 0;

enum states: uint8_t { NORMAL, E, EO };
uint8_t state = NORMAL;

bool alreadyBegan = false;  // SD.begin() misbehaves if not first call

dht DHT;

void initDisplay() {
	randomSeed(analogRead(5));   //.kbv Due does not like A0
    pinMode(A0, OUTPUT);       //.kbv mcufriend have RD on A0
    digitalWrite(A0, HIGH);

	// Setup the LCD
  	myGLCD.InitLCD();
  	myGLCD.setRotation(0);
  	myGLCD.clrScr();
}

void drawDateTimeSection(void)
{
	myGLCD.setColor(255,255,255);
	myGLCD.fillRect(0, 0, 320, 40);
	myGLCD.setColor(0, 0, 0);
	myGLCD.setFontAndSize(1, NULL);
    myGLCD.print("[ Display Date & Time Here. ]", CENTER, 20);
}

void drawThermometer(void)
{
	// We will use color codes for the thermometer display
	// Red -
	// Yellow - (0, 40, 220), room temperatureVal
	// Blue - (120, 30, 0), cool
	myGLCD.setColor(120, 30, 0);
	myGLCD.drawRoundRect(51, 70, 59, 120);
	myGLCD.drawCircle(55, 120, 10);
	myGLCD.setColor(0, 0, 0);
	myGLCD.fillCircle(55, 120, 9);
	myGLCD.fillRoundRect(52, 71, 58, 119);
	myGLCD.setColor(120, 30, 0);
	myGLCD.fillCircle(55, 120, 8);

	// Draw a sample temperature reading.
	// 100 degrees = 50px;
	// 1 degree = 0.5px
	myGLCD.fillRoundRect(54, 99, 56, 120);

}

void refreshTemperatureInfo(float humidity, float temp)
{
	// Clear the screen and draw the frame
	String temperatureVal = String(temp);
	String humidityVal = String(humidity);

  	myGLCD.setColor(0, 0, 0);
  	myGLCD.fillRect(0, 40, 320, 160);

	drawThermometer();

  	myGLCD.setColor(255,255,255);

  	uint8_t fontChoice = 2;
  	myGLCD.setFontAndSize(1, &FreeSans9pt7b);
  	myGLCD.print("Humidity: " + humidityVal + "%", 140, 80);
  	myGLCD.print("Temp.: " + temperatureVal + " deg", 140, 120 );
}

void writeFileHeader(void) {
	String logHeader = 	String("Temperature   Humidity\r\n");
	String logSep = 	String("======================\r\n");

	fd = SD.open(fileName, FILE_WRITE);

	if (fd) {

		for (unsigned int i = 0; i < logHeader.length(); i++)
		{
			byte byteRead = logHeader[i];
			//Serial.write(byteRead); // Echo
			buff[index++] = byteRead;
		}

		fd.write(buff, index);
		fd.flush();
		index = 0;

		for (unsigned int i = 0; i < logSep.length(); i++)
		{
			byte byteRead = logSep[i];
			//Serial.write(byteRead); // Echo
			buff[index++] = byteRead;
		}

		fd.write(buff, index);
		fd.flush();
		index = 0;

		fd.close();
	}
}

void writeData(float temp, float humidity = 0.00)
{
  	fd = SD.open(fileName, FILE_WRITE);

  	if (fd) {

		String temperatureVal = String(temp);
		String humidityVal = String(humidity);

		String logData = 	String(temperatureVal + "         " + humidityVal) + "\r\n";
		for (unsigned int i = 0; i < logData.length(); i++)
		{
			byte byteRead = logData[i];
		 	//Serial.write(byteRead); // Echo
		 	buff[index++] = byteRead;
		}

    	fd.write(buff, index);
    	fd.flush();
    	index = 0;
    	fd.close();
  	}
}

////////////////////////////////////////////////////////////////////////////////
// Prints the captured log so far.
////////////////////////////////////////////////////////////////////////////////
void printFileContents(void)
{
  	// Re-open the file for reading:
  	fd = SD.open(fileName);

  	if (fd)
  	{
    	Serial.println("");
    	Serial.print(fileName);
    	Serial.println(":");

    	while (fd.available())
    	{
      		Serial.write(fd.read());
    	}
  	}
  	else
  	{
    	Serial.print("Error opening ");
    	Serial.println(fileName);
  	}

	fd.close();
}

////////////////////////////////////////////////////////////////////////////////
// Do everything from detecting card through opening the log file
////////////////////////////////////////////////////////////////////////////////
void initializeCard(void)
{
 	Serial.print(F("Initializing SD card..."));

  	// Is there even a card?
  	if (!digitalRead(CARD_DETECT_PIN))
  	{
    	Serial.println(F("No card detected. Waiting for card."));
    	while (!digitalRead(CARD_DETECT_PIN));
    		delay(250); // 'Debounce insertion'
  	}

  	// Card seems to exist.  begin() returns failure
  	// even if it worked if it's not the first call.
  	if (!SD.begin(CHIP_SELECT_PIN) && !alreadyBegan)  // begin uses half-speed...
  	{
    	Serial.println(F("Initialization failed!"));
    	//initializeCard(); // Possible infinite retry loop is as valid as anything
  	}
  	else
  	{
    	alreadyBegan = true;
  	}
  	Serial.println(F("Initialization done."));

  	Serial.print(fileName);
  	if (SD.exists(fileName))
  	{
    	Serial.println(F(" exists."));
  	}
  	else
  	{
    	Serial.println(F(" doesn't exist. Creating."));
  	}

  	Serial.print("Opening file: ");
  	Serial.println(fileName);
}


void setup() {
    // put your setup code here, to run once:
	Serial.begin(57600);
	//while (!Serial);

	if (!rtc.begin()) {
		Serial.println("Couldn't find RTC");
		while (1);
	}

	if (!rtc.isrunning()) {
		rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));	// Set it to the time when this program is being executed for the first time.
	}

	// Note: To satisfy the AVR SPI gods the SD library takes care of setting
  	// SS_PIN as an output. We don't need to.
  	pinMode(CARD_DETECT_PIN, INPUT);
	initializeCard();
	writeFileHeader();

	delay(500);		// Delay to let system boot
	Serial.println("DHT11 Humidity & Temperature Sensor\n\n");
	delay(1000);	// Wait before accessing sensor
	pinMode(TEMP_OK_LED_PIN, OUTPUT);
	pinMode(TEMP_WARN_LED_PIN, OUTPUT);
	pinMode(TEMP_HOT_LED_PIN, OUTPUT);

	initDisplay();

	drawDateTimeSection();
	drawThermometer();
}

void loop() {
    // put your main code here, to run repeatedly:
	// get the date and time right now.
	now = rtc.now();
	DHT.read11(DHT_PIN);

	// Make sure the card is still present
  	/*if (!digitalRead(CARD_DETECT_PIN))
  	{
    	initializeCard();
  	}*/

	Serial.print(now.year(), DEC);
	Serial.print(".");
	Serial.print(now.month(), DEC);
	Serial.print(".");
	Serial.print(now.day(), DEC);
	Serial.print(" ");
	uint8_t hour = now.hour();
	if (hour < 10) {
		Serial.print("0");
		Serial.print(now.hour(), DEC);
	}
	else
		Serial.print(now.hour(), DEC);

	Serial.print(":");
	uint8_t minute = now.minute();
	if (minute < 10) {
		Serial.print("0");
		Serial.print(now.minute(), DEC);
	}
	else
		Serial.print(now.minute(), DEC);

	Serial.print(":");
	uint8_t seconds = now.second();
	if (seconds < 10) {
		Serial.print("0");
		Serial.print(now.second(), DEC);
	}
	else
		Serial.print(now.second(), DEC);
	Serial.print("  Current humidity = ");
	Serial.print(DHT.humidity);
	Serial.print("%\t");
	Serial.print("temperature = ");
	Serial.print(DHT.temperature);
	Serial.println("°C");

	// If the current temperature reading is different from the last recorded reading,
	// refresh the temperature info. Otherwise, do nothing.
	if (DHT.temperature != last_temp_reading)
	{
		last_temp_reading = DHT.temperature;
		refreshTemperatureInfo(DHT.humidity, DHT.temperature);
	}

	if (date_interval_counter == DATE_REFRESH_INTERVAL)
	{
		drawDateTimeSection();
		date_interval_counter = 1;	// Reset
	}

	if (DHT.temperature <= 25.00)	// Less than or Equal to 25 degrees Celsius
	{
		digitalWrite(TEMP_HOT_LED_PIN, LOW);
		digitalWrite(TEMP_WARN_LED_PIN, LOW);
		digitalWrite(TEMP_OK_LED_PIN, HIGH);
	}
	else if (DHT.temperature > 25 && DHT.temperature <= 31) {	// Between 26 and 31 degrees
		digitalWrite(TEMP_HOT_LED_PIN, LOW);
		digitalWrite(TEMP_WARN_LED_PIN, HIGH);
		digitalWrite(TEMP_OK_LED_PIN, LOW);
	}
	else	// Above 31 degrees, man that is hot!
	{
		digitalWrite(TEMP_HOT_LED_PIN, HIGH);
		digitalWrite(TEMP_OK_LED_PIN, LOW);
		digitalWrite(TEMP_WARN_LED_PIN, LOW);
	}

  	writeData(DHT.temperature, DHT.humidity);

	//printFileContents();
	//Serial.print("===== End of File Contents =====\n\n");

	temp_interval_counter++;
	date_interval_counter++;

	delay(1000);
}
