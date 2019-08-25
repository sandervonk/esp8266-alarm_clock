/*
 * ESP8266 based alarm clock, gets time and alarm from Internet
 *
 * Platform: ESP8266 using Arduino IDE
 * Documentation: http://www.coertvonk.com/technology/embedded/connected-alarm-using-esp8266-15838
 * Tested with: Arduino IDE 1.6.11, board package esp8266 2.3.0, Adafruit huzzah feather esp8266
 *
 * GNU GENERAL PUBLIC LICENSE Version 3, check the file LICENSE.md for more information
 * (c) Copyright 2016, Sander Vonk
 * All text above must be included in any redistribution
 */

#define USING_AXTLS
#include <ESP8266WiFi.h>
//#include <WiFiClientSecure.h>
#include "WiFiClientSecureAxTLS.h"
using namespace axTLS;


#include <ESP8266WiFi.h>
#include <WiFiClientSecureRedirect.h>  // https://github.com/cvonk/esp8266-WiFiClientSecureRedirect
#include <WiFiUdp.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>      // https://github.com/adafruit/Adafruit-GFX-Library
#include <Adafruit_SSD1306.h>  // https://github.com/adafruit/Adafruit_SSD1306
#if (SSD1306_LCDHEIGHT != 32)
# error("Height incorrect, please fix Adafruit_SSD1306.h!");
#endif
#include "NtpTime.h"
#include "GoogleCalEvent.h"
#include "Buzzer.h"

// UPDATE THIS BASED ON YOUR CONFIG
//   also update dstPath in GoogleCalEvent.cpp

struct {
	char const * const ssid;
	char const * const passwd;
} _wifi = { "your_wifi_SSID", "your_wifi_key" };

#define DPRINT(...)    Serial.print(__VA_ARGS__)
#define DPRINTLN(...)  Serial.println(__VA_ARGS__)

int8_t const BUZZER_GPIO = 12;  // needs transistor to limit current below 12 mA
int8_t const HAPTIC_GPIO = 13;  // needs transistor to limit current below 12 mA
int8_t const BUTTON_A_GPIO = 0;
int8_t const BUTTON_B_GPIO = 16;
int8_t const BUTTON_C_GPIO = 2;
int8_t const OLED_RESET_GPIO = 3;

static uint8_t timeSyncBitmap[] = {
	0x00, 0x00, 0x07, 0xe0, 0x1f, 0xf0, 0x3c, 0x38, 0x30, 0x1e, 0x70, 0x0e, 0x00, 0x1e, 0x00, 0x00
};
static uint8_t alarmSyncBitmap[] = {
	0x00, 0x00, 0x78, 0x00, 0x70, 0x0e, 0x78, 0x0c, 0x1c, 0x3c, 0x0f, 0xf8, 0x07, 0xe0, 0x00, 0x00
};

Adafruit_SSD1306 oled(OLED_RESET_GPIO);
NtpTime ntpTime("0.north-america.pool.ntp.org", -7, 8888);  // -7 Pacific Daylight Savings, -8 Pacific Standard Time
GoogleCalEvent googleCalEvent;
Buzzer buzzer(BUZZER_GPIO, HAPTIC_GPIO, 1000 /*ms*/, 500 /*ms*/, 1000 /*Hz*/);

void 
setup()
{
	Serial.begin(115200); delay(500);

	ntpTime.begin();
	oled.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // I2C addr 0x3C (for the 128x32)
	oled.display();  // displays the splash screen
	oled.setTextSize(1);
	oled.setTextColor(WHITE);

	DPRINT("\n\nWiFi .");
	char progress[] = "-/|\\";
	uint8_t ii = 0;
	WiFi.begin(_wifi.ssid, _wifi.passwd);
	while (WiFi.status() != WL_CONNECTED) {
		delay(500);
		oled.setCursor(0, 0);
		oled.clearDisplay();
		oled.print("Connecting to WiFi ");
		oled.print(progress[ii++ % sizeof(progress)]);
		oled.display();
		Serial.print(".");
	}
	DPRINTLN(" done");

	buzzer.begin();

	pinMode(BUTTON_B_GPIO, INPUT_PULLUP);
	pinMode(BUTTON_C_GPIO, INPUT_PULLUP);
	pinMode(HAPTIC_GPIO, OUTPUT);
}

void 
loop()
{
	ntpTime.tick();
	googleCalEvent.tick();
	buzzer.tick();

	if (digitalRead(BUTTON_C_GPIO) == 0) {  // button C on OLED pressed
		buzzer.start();  // test only
	}
	if (digitalRead(BUTTON_B_GPIO) == 0) {  // button B on OLED pressed
		buzzer.stop();
	}

	static unsigned long last = 0;
	unsigned long const now = millis();

	if (now - last > 200) {  // every 200 msec

		NtpTime::ntptime_t const time = ntpTime.getTime();
		GoogleCalEvent::alarm_t const alarm = googleCalEvent.getAlarm();

		oled.clearDisplay();
		oled.setCursor(0, 0);

		if (time.status != NtpTime::timeNeedsSync)
		{
			oled.drawBitmap(SSD1306_LCDWIDTH - 16, 0, timeSyncBitmap, 16, 8, WHITE);
		}

		if (alarm.status != GoogleCalEvent::alarmNeedsSync)
		{
			oled.drawBitmap(SSD1306_LCDWIDTH - 16, 8, alarmSyncBitmap, 16, 8, WHITE);
		}

		if (time.status == NtpTime::timeNotSet)
		{
			oled.print("Waiting for time");
		}
		else
		{
			oled.setTextSize(3);
			if (time.hour12 < 10) {
				oled.print(" ");
			}
			oled.print(time.hour12); oled.print(":");
			if (time.minute < 10) {
				oled.print("0");
			}
			oled.print(time.minute);
			oled.setTextSize(1);
			oled.print(time.hour12pm ? "PM" : "AM");
		}

		int const alarmHour = alarm.alarmTime / 60;
		int const alarmMinute = alarm.alarmTime % 60;

		oled.setCursor(0, SSD1306_LCDHEIGHT - 8);
		if (alarm.status == GoogleCalEvent::alarmNotSet) {
			oled.printf("%s %s %u, %u", ntpTime.wdayString(time.wday), ntpTime.monthString(time.month),
				time.day, time.year);
			//oled.print("No alarm set");
		} else {
			oled.print(alarmHour); oled.print(":");
			if (alarmMinute < 10) {
				oled.print("0");
			}
			oled.print(alarmMinute);
			oled.print(" ");
			oled.print(alarm.title);
		}

		int const light = analogRead(0);
		oled.dim(light < 50);

		if (time.status != NtpTime::timeNotSet &&
			alarm.status != GoogleCalEvent::alarmNotSet &&
			alarmHour == time.hour24 && alarmMinute == time.minute && time.second == 0 && buzzer.isOff())
		{
			buzzer.start();
		}

		oled.display();  // write the cached commands to display
		last = now;
	}
}