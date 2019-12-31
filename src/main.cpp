//##################
//  DEFINES
//##################

#define HOSTNAME_ESP8266 "NixieClockTwo"

//##################
//  INCLUDES
//##################

// LIBRARIES
#include <Time.h>
#include <FS.h>
#include <ArduinoJson.h> 		 //https://github.com/bblanchon/ArduinoJson
#include <TimeLib.h>
#include <ESP8266WiFi.h>		 //https://github.com/esp8266/Arduino
#include <ESPAsyncWebServer.h>   //Local WebServer used to serve the configuration portal
#include <ESPAsyncWiFiManager.h> //https://github.com/tzapu/WiFiManager WiFi Configuration Magic
#include <ESP8266Ping.h>
#include <NtpClientLib.h>
#include <SPI.h>
#include <DS1307RTC.h>			 //https://github.com/PaulStoffregen/DS1307RTC
#include <Adafruit_Sensor.h>
#include "tr064Wrapper.h"
#include <ArduinoOTA.h>

// INTERNAL
#include "nixieTube.h"
#include "config.h"
#include "eventHandler.h"
#include "webServerWrapper.h"

#ifdef NIXIE_TYPE_IN12
	#include "NeoPixel.h"
	using namespace NeoPixel;
#endif

// ESP8266 interface
extern "C"
{
#include "user_interface.h"
}

//##################
//  OBJECTS
//##################

Config config; // global configuration struct

NixieClock nixie; // nixie hardware object
WiFiUDP ntpUDP;   // ntp network connection
TR064Wrapper tr064Wrapper;

AsyncWebServer server(80);
DNSServer dns;
WebServerWrapper wServer(&server, &config);

/* #ifdef NIXIE_TYPE_IN12
NeoPixelBusWrapper neoPixel;
#endif */

// AM2320 temperature and humidity sensor
//Adafruit_AM2320 am2320 = Adafruit_AM2320();
// assume there is a sensor connected
bool flagAM2023available = true;

// local time cache to sync with RTC and/or NTP --> display on nixie
bool setTimeOnce = false;
int8_t timeZone = 1;
int8_t minutesTimeZone = 0;

//##################
//  SETUP
//##################

void setup()
{

	Serial.begin(115200);
	while (!Serial); // wait for serial attach
	Serial.println();
	Serial.println("Initializing...");
	Serial.flush();
	Serial.println();
	AsyncWiFiManager wifiManager(&server, &dns);
	setupEventHandler();

	/*
	######## LOAD CONFIG FROM FS ########
	*/
	// Initialize SPIFFS library
	while (!SPIFFS.begin())
	{
		Serial.println(F("Failed to initialize SPIFFS library"));
		delay(1000);
	}
	Serial.println(F("SPIFFS library initializied"));
	loadConfiguration(config);
	Serial.println(F("Config loaded"));

	/*
	######## SETUP HARDWARE ########
	*/

	// activate HV supply
	nixie.hvSupplyEnabled = true;

	// activate AM2320 sensor
	// am2320.begin();
	
	// set hostname
	wifi_station_set_hostname(HOSTNAME_ESP8266);

	//check for wifi
	if (digitalRead(PIN_CONFIG_PORTAL) == false && (WiFi.status() != WL_CONNECTED) && !config.bootToConfig )
	{
		WiFi.begin();
		Serial.println(F("Connecting Wifi ..."));

		int ctrWifiConnectTry = 0;
		while (WiFi.status() != WL_CONNECTED)
		{
			delay(100);
			ctrWifiConnectTry++;
			if (ctrWifiConnectTry > 50)
			{
				Serial.println(F("Wifi is not available"));
				break;
			}
		}
	}
	
	// check for RTC
	if (RTC.isRunning())
	{
		nixie.flagRTCavailable = true;
		// use RTC clock as sync provider in case Wifi is not available
		if(WiFi.status() != WL_CONNECTED){
			Serial.println(F("Using RTC as time server"));
			setSyncProvider(RTC.get);
		}
	}
	else
	{
		Serial.println(F("RTC is not available"));
		nixie.flagRTCavailable = false;
	}



	//  start WIFI config portal if required
	if (digitalRead(PIN_CONFIG_PORTAL) == true || (!nixie.flagWifiavailable && !nixie.flagRTCavailable) || config.bootToConfig)
	{
		Serial.println(F("Starting config portal. User: NixieClock Password: PasswordNixie"));
		nixie.hvSupplyEnabled = true;
		nixie.dispValue[0]=0;
		nixie.dispValue[1]=0;
		nixie.dispValue[2]=0;
		nixie.UpdateDisplay();
		wifiManager.setConfigPortalTimeout(config.timeoutWifimgr);
		wifiManager.setSaveConfigCallback([]{
			Serial.println("WifiManager: configuration done. Deactivating boot to config ...");
			config.bootToConfig = false;
			config.shouldSaveConfig = true;
		});

		if (!wifiManager.startConfigPortal(HOSTNAME_ESP8266, "PasswordNixie"))
		{
			Serial.println(F("WIFI: failed to connect and hit timeout"));
			delay(3000);
		}
	}

	//  setup ArduinoOTA callbacks

	// ArduinoOTA onStart
	ArduinoOTA.onStart([]() {
		String type;
		if (ArduinoOTA.getCommand() == U_FLASH)
		{
			type = "sketch";
		}
		else
		{ // U_SPIFFS
			type = "filesystem";
		}

		// NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
		SPIFFS.end();
		Serial.println("Start updating " + type);
	});

	// ArduinoOTA onError
	ArduinoOTA.onError([](ota_error_t error) {
		Serial.printf("Error[%u]: ", error);
		if (error == OTA_AUTH_ERROR)
		{
			Serial.println("Auth Failed");
		}
		else if (error == OTA_BEGIN_ERROR)
		{
			Serial.println("Begin Failed");
		}
		else if (error == OTA_CONNECT_ERROR)
		{
			Serial.println("Connect Failed");
		}
		else if (error == OTA_RECEIVE_ERROR)
		{
			Serial.println("Receive Failed");
		}
		else if (error == OTA_END_ERROR)
		{
			Serial.println("End Failed");
		}
	});

	if (flag_wifiConnected)
	{
		wServer.init();
	}
#ifdef NIXIE_TYPE_IN12
	NeoPixel::rgbLEDColor = RgbColor(config.rgbLED_r, config.rgbLED_g, config.rgbLED_b);
	NeoPixel::setup();
#endif

	Serial.println(F("Setup finished"));
}

//##################
//  MAIN
//##################

void loop()
{

	// time displayed by clock
	static volatile time_t curTime = 0;
	static volatile bool flagInStandby = false;

	// call processSyncEvent if ntp events occured
	if (syncEventTriggered)
	{
		processSyncEvent(ntpEvent);
		syncEventTriggered = false;
	}

	if (timeStatus() != timeNotSet)
	{
		if (now() != curTime)
		{

			static byte ctrSecond = 0;

			//###########################
			//!!!    time critical    !!!
			//###########################

			curTime = now();
			nixie.storeTimeChange();

			// show time
			if (ctrSecond < 60)
			{
				nixie.dispValue[0] = hour(curTime);
				nixie.dispValue[1] = minute(curTime);
				nixie.dispValue[2] = second(curTime);
				nixie.topColonEnabled[0] = true;
				nixie.topColonEnabled[1] = true;
				nixie.setCurrentDisplayState(nixie.showTime);
			}

			// switch from time to date and back again
			else if (ctrSecond == 60 || ctrSecond == 71)
			{
				nixie.setCurrentDisplayState(nixie.showScreensaver);
			}

			// show date
			else if (ctrSecond < 71)
			{
				nixie.dispValue[0] = day(curTime);
				nixie.dispValue[1] = month(curTime);
				nixie.dispValue[2] = tmYearToY2k(CalendarYrToTm(year(curTime)));
				nixie.topColonEnabled[0] = false;
				nixie.topColonEnabled[1] = false;
				nixie.setCurrentDisplayState(nixie.showDate);
			}

			else
			{
				//Rst seconds counter if no other action is applicable
				ctrSecond = 0;
			}

			nixie.UpdateDisplay();
			nixie.UpdateLED();

			//###########################
			//    not time critical
			//###########################

			// Place not time critical actions below. The correct time is already on display.
			// Duration until next time critical action (update of seconds) will be approximatly 1 s from now,
			// so there is plenty of time to do other stuff

			//update status flags of time provider
			nixie.evaluateStatusOfTimeProvider();

			// AM2023 sensor
			if ((ctrSecond == 0 || ctrSecond == 36) && flagAM2023available)
			{
				//Serial.print("AM2023: Temperature = "); Serial.println(am2320.readTemperature());
				//Serial.print("AM2023: RelHumidity = "); Serial.println(am2320.readHumidity());
				// check if AM2023 provides valid temperature, otherwise stop futher requests
				//if (isnan(am2320.readTemperature())) {
				//  flagAM2023available = false;
				//}
			}

			// TR064
			// to dertermine if fritz box is used, check if it can be reached by domain name "fritz.box"
			static byte ctrTestForTr064 = 0;

			if (config.useTR064 && !tr064Wrapper.flagTr064Initalized && !tr064Wrapper.flagTr064Supported && ctrTestForTr064 < 3 && flag_wifiConnected && ctrSecond > 50)
			{
				ctrTestForTr064++;
				if (tr064Wrapper.testForTR064support(config))
				{
					Serial.println(F("TR064: FritzBox detected"));
				}
				else
				{
					Serial.println(F("TR064: no FritzBox detected, not using TR064 features"));
				}
			}

			// TR064 init connection
			// there seems to be some kind of timing problem which causes an exception when initialized as soon as wifi is available
			if (!tr064Wrapper.flagTr064Initalized && tr064Wrapper.flagTr064Supported && ctrSecond > 60 && flag_wifiConnected)
			{
				Serial.println(F("TR064: initilizing connection"));
				// since constructor of TR064 was provided with standard user and password, set final values here
				tr064Wrapper.initTr064(config);
			}

			// TR064 check presence of connected devices
			if (tr064Wrapper.flagTr064Supported && tr064Wrapper.flagTr064Initalized && ctrSecond == 65)
			{
				tr064Wrapper.updateOnlineUser(config);
			}

			// Standby mode
			// only evaluate once in full ctrSecond loop
			if (ctrSecond == 70)
			{
				bool flagActivateStandbyTr064 = false; // decativate standby by deafault
				bool flagActivateStandbyTime = true;   // activate standby by default. Deactivate only in predefined time frame

				// activate standby when nobody is at home (onlineUsers[] == false)
				if (config.useTR064 && tr064Wrapper.flagTr064Initalized && flag_wifiConnected)
				{
					flagActivateStandbyTr064 = true; // activate standby. Deactivate only if device is recognised to be online 
					for (int i = 0; i < NUM_USER; i++)
					{
						if (tr064Wrapper.onlineUsers[i])
						{
							flagActivateStandbyTr064 = false;
						}
					}
				}

				// assamble standy times based on config and current time
				// copy of curTime
				tmElements_t timeStandbyOn;
				tmElements_t timeStandbyOff;
				breakTime(curTime, timeStandbyOn);
				breakTime(curTime, timeStandbyOff);

				// update missing information from config
				// weekend
				if (weekday(curTime) == 7 || weekday(curTime) == 1 /*saturday and sunday*/)
				{
					timeStandbyOff.Hour = String(config.timeWeekendsSBoff).substring(0, 2).toInt();
					timeStandbyOff.Minute = String(config.timeWeekendsSBoff).substring(3, 5).toInt();
					timeStandbyOff.Second = 0;
					timeStandbyOn.Hour = String(config.timeWeekendsSBon).substring(0, 2).toInt();
					timeStandbyOn.Minute = String(config.timeWeekendsSBon).substring(3, 5).toInt();
					timeStandbyOn.Second = 0;
				}
				// weekday
				else
				{
					timeStandbyOff.Hour = String(config.timeWeekdaySBoff).substring(0, 2).toInt();
					timeStandbyOff.Minute = String(config.timeWeekdaySBoff).substring(3, 5).toInt();
					timeStandbyOff.Second = 0;
					timeStandbyOn.Hour = String(config.timeWeekdaySBon).substring(0, 2).toInt();
					timeStandbyOn.Minute = String(config.timeWeekdaySBon).substring(3, 5).toInt();
					timeStandbyOn.Second = 0;
				}

				// evaluate stanby times
				if (curTime > makeTime(timeStandbyOff) && curTime < makeTime(timeStandbyOn))
				{
					flagActivateStandbyTime = false;
				}

				// handle standby mode
				if (flagActivateStandbyTr064 || flagActivateStandbyTime)
				{
					flagInStandby = true;
					Serial.println(F("Standby: activated"));
					nixie.setCurrentDisplayState(nixie.standby);
					nixie.hvSupplyEnabled = false;
					nixie.ledPWM[0] = 5;
					nixie.ledPWM[1] = 5;
					nixie.ledPWM[2] = 5;
				}
				else
				{
					flagInStandby = false;	
					//nixie.setCurrentDisplayState(nixie.showTime);
					Serial.println(F("Standby: deactivated"));
					nixie.hvSupplyEnabled = true;
					nixie.ledPWM[0] = 50;
					nixie.ledPWM[1] = 50;
					nixie.ledPWM[2] = 50;
				}
			}
			// save config if required
			if (config.shouldSaveConfig)
			{
				saveConfiguration(config);
				config.shouldSaveConfig = false;
				#ifdef NIXIE_TYPE_IN12
					NeoPixel::rgbLEDColor = RgbColor(config.rgbLED_r, config.rgbLED_g, config.rgbLED_b);
				#endif
			}
			ctrSecond++;
		}

		// handle display updates between time changes
		nixie.update();
		#ifdef NIXIE_TYPE_IN12
			if(flagInStandby)
			{
				NeoPixel::update(AnimSelect(NeoPixel::Standby));	
			}
			else
			{
				NeoPixel::update(AnimSelect(config.rgbLED_anim));
			}
			
		#endif

		// allow other processes to do stuff
		yield();

		// handle ArduinoOTA connection
		if (flag_wifiConnected)
		{
			ArduinoOTA.handle();
			//webServerWrapper.handleClient();
		}
	}
}

//##################
//  EVENT HANDLER
//##################

void onSTAConnected(WiFiEventStationModeConnected ipInfo)
{
	if (!flag_wifiConnected)
	{

		Serial.printf("Wifi event: Connected to %s\r\n", ipInfo.ssid.c_str());
		flag_wifiConnected = true;
	}
}

void onSTAGotIP(WiFiEventStationModeGotIP ipInfo)
{
	Serial.printf("Wifi event: Got IP: %s\r\n", ipInfo.ip.toString().c_str());
	Serial.printf("Wifi event: Connected: %s\r\n", WiFi.status() == WL_CONNECTED ? "yes" : "no");
	
	// Start NTP only after IP network is connected
	Serial.println(F("Wifi event: Starting NTP sync"));
	NTP.begin(config.ntp_server, timeZone, true, minutesTimeZone);
	NTP.setInterval(63);

	// Start ArduinoOTA only after IP network is connected
	ArduinoOTA.begin();
	nixie.flagWifiavailable = true;
}

// Manage network disconnection
void onSTADisconnected(WiFiEventStationModeDisconnected event_info)
{
	if (flag_wifiConnected)
	{
		Serial.printf("Wifi event: Disconnected from SSID: %s\n", event_info.ssid.c_str());
		Serial.printf("Wifi event: Reason: %d\n", event_info.reason);

		Serial.println(F("Wifi event: setSyncProvider(RTC.get)"));
		setSyncProvider(RTC.get);

		Serial.println(F("Wifi event: Stopping NTP sync"));
		NTP.stop();

		flag_wifiConnected = false;
		nixie.flagNTPavailable = false;
	}
}

// Process of ntp events in processSyncEvent. Since events are triggered during time sensitive tasks, processing should take place in main loop
void processSyncEvent(NTPSyncEvent_t ntpEvent)
{
	/* NTP events
	timeSyncd,		// Time successfully got from NTP server
	noResponse,		// No response from server
	invalidAddress	// Address not reachable
	*/
	if (ntpEvent!=timeSyncd)
	{
		Serial.print(F("NTP event: Time Sync error: "));
		nixie.flagNTPavailable = false;

		if (ntpEvent == noResponse)
			Serial.println("NTP server not reachable");
		else if (ntpEvent == invalidAddress)
			Serial.println("Invalid NTP server address");

		if (RTC.isRunning)
		{
			Serial.println("NTP event: Getting time from RTC once");
			setTime(RTC.get());
			nixie.flagRTCavailable = true;
		}
	}
	else
	{
		Serial.print("NTP event: Recieved time ");
		Serial.println(NTP.getTimeDateString(NTP.getLastNTPSync()));

		nixie.flagNTPavailable = true;
		nixie.flagRTCavailable = RTC.isRunning();

		// Time difference between NTP and RTC greater than 1 s --> write NTP time to RTC module
		static char ctrDebounceTimeError;
		if (abs(RTC.get() - now()) > 1)
		{
			ctrDebounceTimeError++;
			if (ctrDebounceTimeError >= 3)
			{
				tmElements_t timeCache;
				breakTime(now(), timeCache); //break current time into elements of timeCache
				RTC.write(timeCache);
				Serial.println(F("NTP event: NTP time written to RTC clock to correct time error > 1 s"));
				// reset ctrDebounceTimeError
				ctrDebounceTimeError = 0;
			}
		}
		else
		{
			// reset ctrDebounceTimeError
			ctrDebounceTimeError = 0;
		}
	}
}

void setupEventHandler()
{

	NTP.onNTPSyncEvent([](NTPSyncEvent_t event) {
		ntpEvent = event;
		syncEventTriggered = true;
	});

	// Connect event handler
	static WiFiEventHandler e1, e2, e3;
	e1 = WiFi.onStationModeGotIP(onSTAGotIP); // As soon WiFi is connected, start NTP Client
	e2 = WiFi.onStationModeDisconnected(onSTADisconnected);
	e3 = WiFi.onStationModeConnected(onSTAConnected);
}
