#ifndef CONFIGSTRUCT_H
#define CONFIGSTRUCT_H

// Configuration that we'll store on disk
// define your default values here, if there are different values in config.json, they are overwritten.
// length should be max size + 1

struct Config
{
  bool bootToConfig = true;
  bool useDynIP = true;
  bool shouldSaveConfig = false;
  int timeoutWifimgr = 300;
  int port = 2731;
  char mac_dev[2][20] = {"F4:09:D8:66:42:45", "E0:33:8E:4B:38:C6"};
  char mqtt_server[40] = "mqtt_server";
  int mqtt_port = 8883;
  char blynk_token[33] = "blynk_token";
  char ntp_server[40] = "de.pool.ntp.org";
  char userTR064[40] = "nixeClock";
  char passwordTR064[40] = "nixieTR064";
  int portTR064 = 49000;
  char ipTR064[16] = "192.168.178.1";
  bool useTR064 = true;
  char timeWeekdaySBoff[6] = "16:00";
  char timeWeekdaySBon[6] = "23:00";
  char timeWeekendsSBoff[6] = "08:30";
  char timeWeekendsSBon[6] =  "23:59";
  uint8_t rgbLED_r = 0;
  uint8_t rgbLED_g = 0;
  uint8_t rgbLED_b = 0;
  uint8_t rgbLED_anim = 0;
};

#endif