/*
 * Read the time using the Network Time Protocol (NTP), non-blocking
 *
 * Platform: ESP8266 using Arduino IDE
 * Documentation: http://www.coertvonk.com/technology/embedded/connected-alarm-using-esp8266-15838
 * Tested with: Arduino IDE 1.6.11, board package esp8266 2.3.0, Adafruit huzzah feather esp8266
 * Inspired by Arduino Timer library and ESP8266 NTP Client example (but my iteration doesn't block)
 *
 * GNU GENERAL PUBLIC LICENSE Version 3, check the file LICENSE.md for more information
 * (c) Copyright 2016, Coert Vonk
 * All text above must be included in any redistribution
 */

#include "NtpTime.h"

#define DEBUG
#ifdef DEBUG
#define DPRINT(...)    Serial.print(__VA_ARGS__)
#define DPRINTLN(...)  Serial.println(__VA_ARGS__)
#else
#define DPRINT(...)     //now defines a blank line
#define DPRINTLN(...)   //now defines a blank line
#endif

NtpTime::NtpTime( char const * const host_, // must be static alloc'ed
	              int const timeZone_, // -5=Eastern US, -4=Eastern US DST, -8=Pacific US, -7=Pacific US DST
	              unsigned int const localPort_ )
{
	host = host_;
	timeZone = timeZone_;
	localPort = localPort_;
}

NtpTime::~NtpTime() {}

void
NtpTime::setTime(time_t time_) {
	sysTime = (uint32_t)time_;
	nextSyncTime = (uint32_t)time_ + syncInterval;
	prevMillis = millis();
}

void
NtpTime::sendTimeRequest(void)
{
	while (udp.parsePacket() > 0) {
		// discard any previously received packets
	}

	IPAddress ntpServerIP;
	WiFi.hostByName(host, ntpServerIP);
	memset(packetBuffer, 0, NTP_PACKET_SIZE);
	packetBuffer[0] = 0b11100011;  // LI, version, mode
	packetBuffer[1] = 0;           // stratum, or type of clock
	packetBuffer[2] = 6;           // polling Interval
	packetBuffer[3] = 0xEC;        // peer clock precision
	packetBuffer[12] = 49;
	packetBuffer[13] = 0x4E;
	packetBuffer[14] = 49;
	packetBuffer[15] = 52;

	udp.beginPacket(ntpServerIP, NTP_PORT_NUMBER);
	udp.write(packetBuffer, NTP_PACKET_SIZE);
	udp.endPacket();  // buffer should remain alloc'ed
}

time_t
NtpTime::receiveTimeResponse(void)
{
	int const size = udp.parsePacket();

	if (size >= NTP_PACKET_SIZE) {
		udp.read(packetBuffer, NTP_PACKET_SIZE);  // read packet into the buffer
		unsigned long secsSince1900;
		secsSince1900 = (unsigned long)packetBuffer[40] << 24;
		secsSince1900 |= (unsigned long)packetBuffer[41] << 16;
		secsSince1900 |= (unsigned long)packetBuffer[42] << 8;
		secsSince1900 |= (unsigned long)packetBuffer[43];
		unsigned const long seventyYears = 2208988800UL; // 1970 - 1900
		return secsSince1900 - seventyYears + timeZone * 3600;
	}
	return 0;
}

void
NtpTime::setSyncInterval(uint32_t const interval_) { // set the number of seconds between re-sync
	syncInterval = (uint32_t)interval_;
	nextSyncTime = sysTime + syncInterval;
}

void
NtpTime::begin(void)
{
	udp.begin(localPort);
	setSyncInterval(1);
}

time_t
NtpTime::now(void)
{
	while (millis() - prevMillis >= 1000) {  // will always be the absolute value of the difference
		sysTime++;
		prevMillis += 1000;
	}
	return sysTime;
}

void
NtpTime::tick(void) 
{
	if (state == WAIT4RESPONSE) {
		time_t const time = receiveTimeResponse();
		if (time) {
			setSyncInterval(60);
			setTime(time);
			DPRINTLN("done");
			status = timeSet;
			state = WAIT4SYNCINTERVAL;
		}
		else if (millis() - beginWait >= NTP_TIMEOUT) {
			setSyncInterval(0);
			DPRINTLN("!to");
			status = timeNeedsSync;
			state = WAIT4SYNCINTERVAL;
		}
	}

	if (state == TIMEUNKNOWN ||
		(state == WAIT4SYNCINTERVAL && (uint32_t)now() > nextSyncTime)) {

		DPRINT("Time sync .. ");
		sendTimeRequest();
		beginWait = millis();
		state = WAIT4RESPONSE;
	}
}

NtpTime::ntptime_t const
NtpTime::getTime()
{
	ntptime_t ntptime;
	uint32_t time = (state == TIMEUNKNOWN) ? 0 : now();
	ntptime.status = status;
	ntptime.second = time % 60; time /= 60;
	ntptime.minute = time % 60; time /= 60;
	uint8_t const hour24 = time % 24;
	ntptime.hour24 = hour24;
	ntptime.hour12 = (hour24 == 0) ? 12 : (hour24 > 12) ? hour24 - 12 : hour24;
	ntptime.hour12pm = (hour24 >= 12);

#if TIME_INCLUDEDATE
	static const uint8_t monthDays[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31}; // API starts months from 1, this array starts from 0

    time /= 24;
	ntptime.wday = ((time + 4) % 7) + 1;  // Sunday is day 1

	uint8_t year = 0;
	unsigned long days = 0;
	while((unsigned)(days += (isLeapYear(year) ? 366 : 365)) <= time) {
		year++;
	}
	ntptime.year = 1970 + year;

	days -= isLeapYear(year) ? 366 : 365;
	time -= days; // now it is days in this year, starting at 0

	days = 0;
	uint8_t month = 0;
	for (month = 0; month < 12; month++) {
		uint8_t monthLength = monthDays[month];
		if (month == 1 && isLeapYear(year)) {
			monthLength++;
		}
		if (time < monthLength) {
			break;
		}
		time -= monthLength;
	}
	ntptime.month = month + 1;  // January=1
	ntptime.day = time + 1;     // day of month
#endif
	return ntptime;
}

