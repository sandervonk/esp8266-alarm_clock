#pragma once
#include <ESP8266WiFi.h>        // http://arduino.esp8266.com/stable/package_esp8266com_index.json (board manager)

#define TIME_INCLUDEDATE (0)

class Buzzer {
public:
	Buzzer(uint8_t const buzzerGpio, uint8_t const hapticGpio, uint32_t const onDuration, uint32_t const offDuration, uint16_t const frequency);
	void begin(void);
	void tick(void);
	void start(void);
	void stop(void);
	bool isOff(void);

private:
	typedef enum {OFF, STOP, BUZZING, PAUSE} state_t;
	state_t state;
	uint32_t nextTime;
	uint8_t const buzzerGpio;
	uint8_t const hapticGpio;
	uint32_t const onDuration;
	uint32_t const offDuration;
	uint16_t const frequency;
};