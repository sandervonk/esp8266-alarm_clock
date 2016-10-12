#pragma once

#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <WiFiClientSecureRedirect.h>

class GoogleCalEvent {

  public:
	GoogleCalEvent();
	~GoogleCalEvent();

	typedef enum {
		alarmNotSet, alarmNeedsSync, alarmSet
	}  status_t;
	typedef struct {
		status_t status;
		int alarmTime;
		int startTime;
		char title[30];
	} alarm_t;

	void tick(); // housekeeping, should be called approx every 100 msec

	alarm_t const getAlarm();

  private:
	WiFiClientSecureRedirect client;
	status_t status = alarmNotSet;
	alarm_t alarm;
	typedef enum {
		ALARMUNKNOWN = 0,
		WAIT4SYNCINTERVAL,
		WAIT4CONNECT,
		WAIT4RESPONSE,
		COUNT
	} state_t;
	state_t state = ALARMUNKNOWN;
	uint32_t eventTimeouts[COUNT] = {
		0,     // ALARMUNKNOWN (0)
		0,     // WAIT4SYNCINTERVAL (1)
		6000,  // WAIT4CONNECT (2)
		10000   // WAIT4RESPONSE (3)
	};
	uint32_t beginWait;
	uint8_t const connect();
	uint8_t const sendAlarmRequest();
	uint8_t const receiveAlarmResponse();
	uint32_t syncInterval = 0;  // [msec]
	uint32_t nextSyncTime = 0;  // [msec]
	void setSyncInterval(uint32_t const interval_); // [msec]
};