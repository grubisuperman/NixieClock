

#if ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#include "pins_arduino.h"
#endif


#include "nixieTube.h"

NixieClock::NixieClock()
{
	
	SetupNixies();
}

void NixieClock::SetupNixies() {
	pinMode(SNX4HC595_DATA_PIN, OUTPUT);
	pinMode(SNX4HC595_LATCH_PIN, OUTPUT);
	pinMode(SNX4HC595_CLOCK_PIN, OUTPUT);

	pinMode(PIN_CONFIG_PORTAL, INPUT);

	#ifdef NIXIE_TYPE_IN14
		Serial.println(F("NixieTube: Setting up Nixie IN-14"));
		pinMode(PIN_LED_NIXIE1, OUTPUT);
		pinMode(PIN_LED_NIXIE2, OUTPUT);
		pinMode(PIN_LED_NIXIE3, OUTPUT);

		analogWrite(PIN_LED_NIXIE1, 0);
		analogWrite(PIN_LED_NIXIE2, 0);
		analogWrite(PIN_LED_NIXIE3, 0);

		//Set PWM frequency 500, default is 1000
		analogWriteFreq(500);
		//Set range 0~100, default is 0~1023
		analogWriteRange(255);
	#endif

	#ifdef NIXIE_TYPE_IN12
		Serial.println(F("NixieTube: Setting up Nixie IN-12"));
		pinMode(PIN_D_ENABLE_HV, OUTPUT);
		digitalWrite(PIN_D_ENABLE_HV, false);
	#endif




	// setup nixie hardware
	ledEnabled[0] = true;
	ledEnabled[1] = true;
	ledEnabled[2] = true;

	ledPWM[0] = 50;
	ledPWM[1] = 50;
	ledPWM[2] = 50;



	groupEnabled[0] = true;
	groupEnabled[1] = true;
	groupEnabled[2] = true;

	hvSupplyEnabled = false;

	ConvertDataToShiftOutByte();
	UpdateDisplay();
	UpdateLED();

}


void NixieClock::ConvertDataToShiftOutByte() {
	for (byte i = 0; i<NUM_NIXIE_PAIRS; i++) {
		byte highDigit = 0;
		byte lowDigit = 0;
		byte tmp_highDigit = 0;
		byte tmp_lowDigit = 0;
		
		#ifdef NIXIE_TYPE_IN14
			if (groupEnabled[i]) {
				tmp_highDigit = (byte)((float)dispValue[i] * 0.1);

				if (tmp_highDigit == 0) {
					highDigit = 0;
				}
				else {
					highDigit = 10 - tmp_highDigit;
				}

				tmp_lowDigit = (byte)(dispValue[i] - tmp_highDigit * 10);

				if (tmp_lowDigit == 0) {
					lowDigit = 0;
				}
				else {
					lowDigit = 10 - (byte)(dispValue[i] - tmp_highDigit * 10);
				}

				shiftOutByte[i] = (B11110000 & (highDigit << 4)) | (B00001111 & lowDigit);
			}
			else {
				shiftOutByte[i] = B11111111;
			}
		#endif

		#ifdef NIXIE_TYPE_IN12
			if (groupEnabled[i]) {

				tmp_highDigit = (byte)((float)dispValue[i] * 0.1);
				
				if (tmp_highDigit == 0) {
					highDigit = B00001001;
				}
				else {
					highDigit = tmp_highDigit - 1;
				}


				tmp_lowDigit = (byte)(dispValue[i] - tmp_highDigit * 10);
				if (tmp_lowDigit == 0) {
					lowDigit = B00001001;
				}
				else {
					lowDigit = tmp_lowDigit - 1;
				}



				shiftOutByte[i] =  (byte) ((lowDigit << 4) | highDigit);
			}
			else {
				shiftOutByte[i] = B11111111;
			}
		#endif

	}

}


void NixieClock::UpdateDisplay() {

	millisLastDispUpdate = millis();
	byte shiftOutMainboard = 0;

	// prepare display data
	ConvertDataToShiftOutByte();
  
	// set bit to turn HV supply on or off
	#ifdef NIXIE_TYPE_IN14
		shiftOutMainboard |= (!hvSupplyEnabled) << 7;
	#else
		shiftOutMainboard |= 0 << 7;
	#endif
	
	shiftOutMainboard |= topColonEnabled[1] << 6;   //top right
	shiftOutMainboard |= 0 << 5; //nc
	shiftOutMainboard |= bottomColonEnabled[1] << 4; //bottom right
	shiftOutMainboard |= 0 << 3;
	shiftOutMainboard |= topColonEnabled[0] << 2;
	shiftOutMainboard |= 1 << 1;
	shiftOutMainboard |= bottomColonEnabled[0] << 0; //Q_H
	

	#ifdef NIXIE_TYPE_IN12
		// turn HV supply on or off
		digitalWrite(PIN_D_ENABLE_HV, hvSupplyEnabled);
	#endif
	
	// activate SNX4HC595
	digitalWrite(SNX4HC595_LATCH_PIN, LOW);

	// write nixie digits
	for (int index = NUM_NIXIE_PAIRS - 1; index >= 0; index--) {
		shiftOut(SNX4HC595_DATA_PIN, SNX4HC595_CLOCK_PIN, MSBFIRST, shiftOutByte[index]);
	}

	// write colons and HV supply state
	shiftOut(SNX4HC595_DATA_PIN, SNX4HC595_CLOCK_PIN, LSBFIRST, shiftOutMainboard);

	// finish transmission to SNX4HC595
	digitalWrite(SNX4HC595_LATCH_PIN, HIGH);
}

void NixieClock::UpdateLED() {
	#ifdef NIXIE_TYPE_IN14
		if (ledEnabled[0]) { analogWrite(PIN_LED_NIXIE1, 255 - ledPWM[0]); }
		else { analogWrite(PIN_LED_NIXIE1, 255); };
		if (ledEnabled[1]) { analogWrite(PIN_LED_NIXIE2, 255 - ledPWM[1]); }
		else { analogWrite(PIN_LED_NIXIE2, 255); };
		if (ledEnabled[2]) { analogWrite(PIN_LED_NIXIE3, 255 - ledPWM[2]); }
		else { analogWrite(PIN_LED_NIXIE3, 255); };
	#endif
}



void NixieClock::update()
{
	// update function is called directly from main loop
	// long currentMillis = millis();
	

	if ((millis() - millisLastDispUpdate > 80) && (currentDisplayState == showScreensaver)) {
		dispValue[0] = random(0,99);
		dispValue[1] = random(0,99);
		dispValue[2] = random(0,99);
		UpdateDisplay();

	}
	
	//UpdateLED();
}

void NixieClock::storeTimeChange()
{
	millisLastTimeChange = millis();
}

void NixieClock::evaluateStatusOfTimeProvider()
{
	static boolean _lastflagRTCavailable = false;
	static boolean _lastflagNTPavailable = false;
	static byte ctrSeconds = 0;

	if (_lastflagNTPavailable != flagNTPavailable) {
		if (flagNTPavailable) {
			Serial.println(F("StatusIndicator: NTP available"));
		}
		else
		{
			Serial.println(F("StatusIndicator: NTP not available"));
		}
	}

	if (_lastflagRTCavailable != flagRTCavailable) {
		if (flagRTCavailable) {
			Serial.println(F("StatusIndicator: RTC available"));
		}
		else
		{
			Serial.println(F("StatusIndicator: RTC not available"));
		}
	}

	_lastflagRTCavailable = flagRTCavailable;
	_lastflagNTPavailable = flagNTPavailable;
	
	ctrSeconds++;
	if (ctrSeconds >= 5) {
		ctrSeconds = 0;
	}
	else if (ctrSeconds == 1)
	{
		bottomColonEnabled[0] = flagRTCavailable;
		bottomColonEnabled[1] = flagNTPavailable;
	}
	else
	{
		bottomColonEnabled[0] = true;
		bottomColonEnabled[1] = true;
	}

}

void NixieClock::setLedEffekt(LedEffect effect) {
	switch (effect)
	{
	case NixieClock::fader:
		break;
	case NixieClock::sinus:
		break;
	default:
		break;
	}
}


void NixieClock::setCurrentDisplayState(displayState state) {
	currentDisplayState = state;
	return;
}
