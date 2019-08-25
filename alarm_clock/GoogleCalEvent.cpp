/*
 * Asynchronously fetch first event of the day to set alarm clock from Google Calendar
 *
 * Platform: ESP8266 using Arduino IDE
 * Documentation: http://www.coertvonk.com/technology/embedded/connected-alarm-using-esp8266-15838
 *           and: http://www.coertvonk.com/technology/embedded/esp8266-clock-import-events-from-google-calendar-15809
 * Tested with: Arduino IDE 1.6.11, board package esp8266 2.3.0, Adafruit huzzah feather esp8266
 *
 * GNU GENERAL PUBLIC LICENSE Version 3, check the file LICENSE.md for more information
 * (c) Copyright 2016, Coert Vonk
 * All text above must be included in any redistribution
 */

#define USING_AXTLS
#include <ESP8266WiFi.h>
//#include <WiFiClientSecure.h>
#include "WiFiClientSecureAxTLS.h"
using namespace axTLS;

#include <ESP8266WiFi.h>
#include <WiFiClientSecureRedirect.h>
#include "GoogleCalEvent.h"

 // UPDATE THIS BASED ON YOUR PUBLIC GOOGLE SCRIPT
char const * const dstPath = "/macros/s/your_unique_pseudo_random_key_to_your_google_script12345/exec";  // UPDATE !!

//
char const * const dstHost = "script.google.com";
int const dstPort = 443;
int32_t const readTimeout = 2000; // [msec] 
char const * const fName = "GoogleCalEvent::";

// On a Linux system with OpenSSL installed, get the SHA1 fingerprint for the destination and redirect hosts:
//   echo -n | openssl s_client -connect script.google.com:443 2>/dev/null | openssl x509  -noout -fingerprint | cut -f2 -d'='
//   echo -n | openssl s_client -connect script.googleusercontent.com:443 2>/dev/null | openssl x509  -noout -fingerprint | cut -f2 -d'='
char const * const dstFingerprint = "C7:4A:32:BC:A0:30:E6:A5:63:D1:8B:F4:2E:AC:19:89:81:20:96:BB";
char const * const redirFingerprint = "E6:88:19:5A:3B:53:09:43:DB:15:56:81:7C:43:30:6D:3E:9D:2F:DE";

#define DEBUG
#ifdef DEBUG
#define DPRINT(...)    Serial.print(__VA_ARGS__)
#define DPRINTLN(...)  Serial.println(__VA_ARGS__)
#else
#define DPRINT(...)     //now defines a blank line
#define DPRINTLN(...)   //now defines a blank line
#endif

GoogleCalEvent::GoogleCalEvent() {}
GoogleCalEvent::~GoogleCalEvent() {}

static size_t
_readln(Stream &stream, 
	    char * const buf, 
	    size_t const bufSize) 
{
	size_t const len = stream.readBytesUntil('\n', buf, bufSize - 1);
	if (len == bufSize - 1) {
		while (stream.readBytesUntil('\n', &buf[len], 1)) {
			// ignore remainder of the line
		}
	}
	buf[len] = '\0';
	return len;
}

GoogleCalEvent::alarm_t const
GoogleCalEvent::getAlarm()
{
	alarm_t alarm;
	alarm.status = status;
	alarm.alarmTime = this->alarm.alarmTime;
	alarm.startTime = this->alarm.startTime;
	strcpy(alarm.title, this->alarm.title);
	return alarm;
}

uint8_t  // returns 0 on success
GoogleCalEvent::connect()
{
	return client.connect(dstHost, dstPort, dstFingerprint) != 1;
}

uint8_t // returns 0 on success
GoogleCalEvent::sendAlarmRequest()
{
	return client.request(dstPath, dstHost, readTimeout, redirFingerprint);
}

uint8_t // returns 0 on success
GoogleCalEvent::receiveAlarmResponse()
{
	char * const line = this->alarm.title;  // reuse eventTitle buffer
	size_t const lineSize = sizeof(this->alarm.title);

	size_t const len1 = _readln(client, line, lineSize); this->alarm.alarmTime = atoi(line);
	size_t const len2 = _readln(client, line, lineSize); this->alarm.startTime = atoi(line);
	size_t const len3 = _readln(client, line, lineSize);
	bool const success = len1 && len2 && len3;

	client.stop();
	return (!success);
}

void
GoogleCalEvent::setSyncInterval(uint32_t const interval_) { // [sec] set the number of seconds between re-sync
	syncInterval = interval_;
	nextSyncTime = millis() + interval_ * 1000UL;
}

void
GoogleCalEvent::tick()
{
	client.tick();

	state_t prevState;
	do {
		prevState = state;
		// timeout
		uint32_t const timeout = eventTimeouts[state];
		if (timeout && millis() - beginWait >= timeout) {
			DPRINT(fName); DPRINT(__func__); DPRINT("() timeout in state "); DPRINTLN(state);
			Serial.flush();
			setSyncInterval(0);
			client.stop();
			state = ALARMUNKNOWN;
		}

		uint8_t error = 0;
		switch(state) {
			case WAIT4SYNCINTERVAL:
				if (millis() <= nextSyncTime) {
					break;
				}
				// fall through
			case ALARMUNKNOWN:
				DPRINT("Alarm sync .. ");
				if (!(error = connect())) {
					state = WAIT4CONNECT;
				}
				break;
			case WAIT4CONNECT:
				if (client.connected()) {
					if (!(error = sendAlarmRequest())) {
						state = WAIT4RESPONSE;
					}
				}
				break;
			case WAIT4RESPONSE:
				if (client.response()) {
					if (!(error = receiveAlarmResponse())) {
						setSyncInterval(15 * 60);  // [s]
						DPRINTLN("done");
						status = alarmSet;
						state = WAIT4SYNCINTERVAL;
					}
				}
				break;
			case COUNT:  // to please the compiler
				break;
		}
		if (error) {
			DPRINT(fName); DPRINT(__func__); DPRINT("() error("); DPRINT(error); DPRINT(") in state "); DPRINTLN(state);
			client.stop();
			state = ALARMUNKNOWN;
		}
		if (state != prevState) {
			beginWait = millis();
		}
	} while (state != prevState && eventTimeouts[state]);
}
