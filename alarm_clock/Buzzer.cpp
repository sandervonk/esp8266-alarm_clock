/*
 * Driver for piezo transducer and haptic motor, non-blocking (asynchronous)
 *
 * Platform: ESP8266 using Arduino IDE
 * Documentation: http://www.coertvonk.com/technology/embedded/connected-alarm-using-esp8266-15838
 * Tested with: Arduino IDE 1.6.11, board package esp8266 2.3.0, Adafruit huzzah feather esp8266
 *
 * GNU GENERAL PUBLIC LICENSE Version 3, check the file LICENSE.md for more information
 * (c) Copyright 2016, Coert Vonk
 * All text above must be included in any redistribution
 */

#include "Buzzer.h"

Buzzer::Buzzer(uint8_t const buzzerGpio_, uint8_t const hapticGpio_, uint32_t const onDuration_, uint32_t const offDuration_, uint16_t const frequency_)
	: buzzerGpio(buzzerGpio_), hapticGpio(hapticGpio_), onDuration(onDuration_), offDuration(offDuration_), frequency(frequency_ / 2)
{}

void
Buzzer::begin(void) 
{}

void
Buzzer::start(void) {
	if (state == OFF) {
		nextTime = 0;  // now
		state = PAUSE; // will immediate transition to BUZZING state
	}
}

void
Buzzer::stop(void) {
	state = STOP;
}

bool
Buzzer::isOff(void) {
	return state == OFF;
}

void
Buzzer::tick(void) 
{
	state_t prevState;
	do {
		prevState = state;

		switch (state) {
			case OFF:
				break;

			case BUZZING:
				if (millis() > nextTime) {
					nextTime = millis() + offDuration;
					noTone(buzzerGpio);
					digitalWrite(hapticGpio, 0);
					state = PAUSE;
				}
				break;

			case PAUSE:
				if (millis() > nextTime) {
					nextTime = millis() + onDuration;
					tone(buzzerGpio, frequency);
					digitalWrite(hapticGpio, 1);
					state = BUZZING;
				}
				break;

			case STOP:
				noTone(buzzerGpio);
				digitalWrite(hapticGpio, 0);
				state = OFF;
				break;

		}
	} while (state != prevState);
}
