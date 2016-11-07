#pragma once
#include <ESP8266WiFi.h>        // http://arduino.esp8266.com/stable/package_esp8266com_index.json (board manager)
#include <WiFiUdp.h>
#include <time.h>

#define TIME_INCLUDEDATE (1)

class NtpTime : WiFiUDP {
public:
	NtpTime(char const * const host,
		int const timeZone, // -5=Eastern US, -4=Eastern US DST, -8=Pacific US, -7=Pacific US DST
		unsigned int const localPort);
	~NtpTime();
	typedef enum {
		timeNotSet, timeNeedsSync, timeSet
	}  status_t;
	typedef struct {
		status_t status;
		uint8_t second;
		uint8_t minute;
		uint8_t hour24;
		uint8_t hour12;
		bool hour12pm;
#if TIME_INCLUDEDATE
		uint8_t wday;   // day of week, Sunday is day 1
		uint8_t day;
		uint8_t month; // January = 1
		uint16_t year;
#endif
	} 	ntptime_t;
	void begin(void);
	ntptime_t const getTime();
	void tick(void);  // maintenance, should be called every 100 msec or so from loop()
#if TIME_INCLUDEDATE
	char const * const wdayString(uint8_t const wday);
	char const * const monthString(uint8_t const month);
	boolean isLeapYear(int year);
#endif

private:
	char const * host;
	int timeZone;
	unsigned int localPort;
	WiFiUDP udp;
	static const int NTP_TIMEOUT = 1000; // [msec]
	static const int NTP_PORT_NUMBER = 123;
	static const int NTP_PACKET_SIZE = 48; // NTP time stamp is in the first 48 bytes of the message
	byte packetBuffer[NTP_PACKET_SIZE]; // tx and rx buffer
	uint32_t syncInterval = 0;  // [sec]
	uint32_t sysTime = 0;
	uint32_t prevMillis = 0;
	uint32_t nextSyncTime = 0;
	status_t status = timeNotSet;
	uint32_t beginWait;
	enum { TIMEUNKNOWN, WAIT4SYNCINTERVAL, WAIT4RESPONSE, } state = TIMEUNKNOWN;
	void setTime(time_t t);
	void setSyncInterval(uint32_t const interval);
	void sendTimeRequest(void);
	time_t receiveTimeResponse(void);
	time_t now(void);
};