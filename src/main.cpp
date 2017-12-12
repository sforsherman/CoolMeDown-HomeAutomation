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
#include "Fonts/FreeSans9pt7b.h"
#include "Fonts/FreeMono9pt7b.h"
#define DHT_PIN		A6

UTFTGLUE myGLCD(0x9488,A2,A1,A3,A4,A0);
MCUFRIEND_kbv kbv;
RTC_DS1307 rtc;
DateTime now;

const int8_t DATE_REFRESH_INTERVAL = 60;	// Refresh the date every minute.
int8_t date_interval_counter = 1;
int8_t last_minute = 0;
int8_t curr_minute = 0;

const int8_t TEMP_REFRESH_RATE = 5;	// Set the temperature refreshing rate (by seconds)
int8_t temp_interval_counter = 1;

float last_temp_reading = 0.00;

char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

#define TEMP_OK_LED_PIN		22
#define TEMP_WARN_LED_PIN	23
#define TEMP_HOT_LED_PIN	24

// Declare which fonts we will be using
#if !defined(BigFont)
extern uint8_t BigFont[];    //.kbv GLUE defines as GFXFont ref
#endif

void writeFileHeader(void);
void writeData(void);
void printFileContents(void);
void initializeCard(void);

File fd;
const uint8_t BUFFER_SIZE = 20;
char fileName[] = "readings.txt"; // SD library only supports up to 8.3 names
char buff[BUFFER_SIZE+2] = "";  // Added two to allow a 2 char peek for EOF state
uint8_t index = 0;

const uint8_t chipSelect = 48;
const uint8_t cardDetect = 49;

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
	/*int buf[478];
  int x, x2;
  int y, y2;
  int r;*/

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
/*
  myGLCD.setColor(255, 0, 0);
  myGLCD.fillRect(0, 0, 479, 13);
  myGLCD.setColor(64, 64, 64);
  myGLCD.fillRect(0, 306, 479, 319);
  myGLCD.setColor(255, 255, 255);
  myGLCD.setBackColor(255, 0, 0);
  myGLCD.print("* Universal Color TFT Display Library *", CENTER, 1);
  myGLCD.setBackColor(64, 64, 64);
  myGLCD.setColor(255,255,0);
  myGLCD.print("<http://electronics.henningkarlsen.com>", CENTER, 307);

  myGLCD.setColor(0, 0, 255);
  myGLCD.drawRect(0, 14, 479, 305);

// Draw crosshairs
  myGLCD.setColor(0, 0, 255);
  myGLCD.setBackColor(0, 0, 0);
  myGLCD.drawLine(239, 15, 239, 304);
  myGLCD.drawLine(1, 159, 478, 159);
  for (int i=9; i<470; i+=10)
    myGLCD.drawLine(i, 157, i, 161);
  for (int i=19; i<220; i+=10)
    myGLCD.drawLine(237, i, 241, i);

// Draw sin-, cos- and tan-lines
  myGLCD.setColor(0,255,255);
  myGLCD.print("Sin", 5, 15);
  for (int i=1; i<478; i++)
  {
    myGLCD.drawPixel(i,159+(sin(((i*1.13)*3.14)/180)*95));
  }

  myGLCD.setColor(255,0,0);
  myGLCD.print("Cos", 5, 27);
  for (int i=1; i<478; i++)
  {
    myGLCD.drawPixel(i,159+(cos(((i*1.13)*3.14)/180)*95));
  }

  myGLCD.setColor(255,255,0);
  myGLCD.print("Tan", 5, 39);
  for (int i=1; i<478; i++)
  {
    myGLCD.drawPixel(i,159+(tan(((i*1.13)*3.14)/180)));
  }

  delay(2000);

  myGLCD.setColor(0,0,0);
  myGLCD.fillRect(1,15,478,304);
  myGLCD.setColor(0, 0, 255);
  myGLCD.setBackColor(0, 0, 0);
  myGLCD.drawLine(239, 15, 239, 304);
  myGLCD.drawLine(1, 159, 478, 159);

// Draw a moving sinewave
  x=1;
  for (int i=1; i<(478*15); i++)
  {
    x++;
    if (x==479)
      x=1;
    if (i>479)
    {
      if ((x==239)||(buf[x-1]==159))
        myGLCD.setColor(0,0,255);
      else
        myGLCD.setColor(0,0,0);
      myGLCD.drawPixel(x,buf[x-1]);
    }
    myGLCD.setColor(0,255,255);
    y=159+(sin(((i*0.7)*3.14)/180)*(90-(i / 100)));
    myGLCD.drawPixel(x,y);
    buf[x-1]=y;
  }

  delay(2000);

  myGLCD.setColor(0,0,0);
  myGLCD.fillRect(1,15,478,304);

// Draw some filled rectangles
  for (int i=1; i<6; i++)
  {
    switch (i)
    {
      case 1:
        myGLCD.setColor(255,0,255);
        break;
      case 2:
        myGLCD.setColor(255,0,0);
        break;
      case 3:
        myGLCD.setColor(0,255,0);
        break;
      case 4:
        myGLCD.setColor(0,0,255);
        break;
      case 5:
        myGLCD.setColor(255,255,0);
        break;
    }
    myGLCD.fillRect(150+(i*20), 70+(i*20), 210+(i*20), 130+(i*20));
  }

  delay(2000);

  myGLCD.setColor(0,0,0);
  myGLCD.fillRect(1,15,478,304);

// Draw some filled, rounded rectangles
  for (int i=1; i<6; i++)
  {
    switch (i)
    {
      case 1:
        myGLCD.setColor(255,0,255);
        break;
      case 2:
        myGLCD.setColor(255,0,0);
        break;
      case 3:
        myGLCD.setColor(0,255,0);
        break;
      case 4:
        myGLCD.setColor(0,0,255);
        break;
      case 5:
        myGLCD.setColor(255,255,0);
        break;
    }
    myGLCD.fillRoundRect(270-(i*20), 70+(i*20), 330-(i*20), 130+(i*20));
  }

  delay(2000);

  myGLCD.setColor(0,0,0);
  myGLCD.fillRect(1,15,478,304);

// Draw some filled circles
  for (int i=1; i<6; i++)
  {
    switch (i)
    {
      case 1:
        myGLCD.setColor(255,0,255);
        break;
      case 2:
        myGLCD.setColor(255,0,0);
        break;
      case 3:
        myGLCD.setColor(0,255,0);
        break;
      case 4:
        myGLCD.setColor(0,0,255);
        break;
      case 5:
        myGLCD.setColor(255,255,0);
        break;
    }
    myGLCD.fillCircle(180+(i*20),100+(i*20), 30);
  }

  delay(2000);

  myGLCD.setColor(0,0,0);
  myGLCD.fillRect(1,15,478,304);

// Draw some lines in a pattern
  myGLCD.setColor (255,0,0);
  for (int i=15; i<304; i+=5)
  {
    myGLCD.drawLine(1, i, (i*1.6)-10, 304);
  }
  myGLCD.setColor (255,0,0);
  for (int i=304; i>15; i-=5)
  {
    myGLCD.drawLine(478, i, (i*1.6)-11, 15);
  }
  myGLCD.setColor (0,255,255);
  for (int i=304; i>15; i-=5)
  {
    myGLCD.drawLine(1, i, 491-(i*1.6), 15);
  }
  myGLCD.setColor (0,255,255);
  for (int i=15; i<304; i+=5)
  {
    myGLCD.drawLine(478, i, 490-(i*1.6), 304);
  }

  delay(2000);

  myGLCD.setColor(0,0,0);
  myGLCD.fillRect(1,15,478,304);

// Draw some random circles
  for (int i=0; i<100; i++)
  {
    myGLCD.setColor(random(255), random(255), random(255));
    x=32+random(416);
    y=45+random(226);
    r=random(30);
    myGLCD.drawCircle(x, y, r);
  }

  delay(2000);

  myGLCD.setColor(0,0,0);
  myGLCD.fillRect(1,15,478,304);

// Draw some random rectangles
  for (int i=0; i<100; i++)
  {
    myGLCD.setColor(random(255), random(255), random(255));
    x=2+random(476);
    y=16+random(289);
    x2=2+random(476);
    y2=16+random(289);
    myGLCD.drawRect(x, y, x2, y2);
  }

  delay(2000);

  myGLCD.setColor(0,0,0);
  myGLCD.fillRect(1,15,478,304);

// Draw some random rounded rectangles
  for (int i=0; i<100; i++)
  {
    myGLCD.setColor(random(255), random(255), random(255));
    x=2+random(476);
    y=16+random(289);
    x2=2+random(476);
    y2=16+random(289);
    myGLCD.drawRoundRect(x, y, x2, y2);
  }

  delay(2000);

  myGLCD.setColor(0,0,0);
  myGLCD.fillRect(1,15,478,304);

  for (int i=0; i<100; i++)
  {
    myGLCD.setColor(random(255), random(255), random(255));
    x=2+random(476);
    y=16+random(289);
    x2=2+random(476);
    y2=16+random(289);
    myGLCD.drawLine(x, y, x2, y2);
  }

  delay(2000);

  myGLCD.setColor(0,0,0);
  myGLCD.fillRect(1,15,478,304);

  for (int i=0; i<10000; i++)
  {
    myGLCD.setColor(random(255), random(255), random(255));
    myGLCD.drawPixel(2+random(476), 16+random(289));
  }

  delay(2000);

  myGLCD.fillScr(0, 0, 255);
  myGLCD.setColor(255, 0, 0);
  myGLCD.fillRoundRect(160, 70, 319, 169);

  myGLCD.setColor(255, 255, 255);
  myGLCD.setBackColor(255, 0, 0);
  myGLCD.print("That's it!", CENTER, 93);
  myGLCD.print("Restarting in a", CENTER, 119);
  myGLCD.print("few seconds...", CENTER, 132);

  myGLCD.setColor(0, 255, 0);
  myGLCD.setBackColor(0, 0, 255);
  myGLCD.print("Runtime: (msecs)", CENTER, 290);
  myGLCD.printNumI(millis(), CENTER, 305);
  */
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
// Do everything from detecting card through opening the demo file
////////////////////////////////////////////////////////////////////////////////
void initializeCard(void)
{
 	Serial.print(F("Initializing SD card..."));

  	// Is there even a card?
  	if (!digitalRead(cardDetect))
  	{
    	Serial.println(F("No card detected. Waiting for card."));
    	while (!digitalRead(cardDetect));
    		delay(250); // 'Debounce insertion'
  	}

  	// Card seems to exist.  begin() returns failure
  	// even if it worked if it's not the first call.
  	if (!SD.begin(chipSelect) && !alreadyBegan)  // begin uses half-speed...
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
  	pinMode(cardDetect, INPUT);
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
  	/*if (!digitalRead(cardDetect))
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
