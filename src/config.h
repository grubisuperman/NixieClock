//##################
//  CONFIG
//##################

#ifndef CONFIG_H
#define CONFIG_H

#include "configstruct.h"

#define CONFIG_FILE "/config.json"
//const char *filename = "/config.json";

// Loads the configuration from a file
void loadConfiguration(Config &config)
{
  // Open file for reading
  File file = SPIFFS.open(CONFIG_FILE, "r");

  // Allocate the memory pool on the stack.
  // Don't forget to change the capacity to match your JSON document.
  // Use arduinojson.org/assistant to compute the capacity.
  StaticJsonBuffer<1024> jsonBuffer;

  // Parse the configJson object
  JsonObject &configJson = jsonBuffer.parseObject(file);

  if (!configJson.success())
  {
    Serial.println(F("Failed to read file, using default configuration"));
  }
  else
  {
    // Copy values from the JsonObject to the Config

    // bool
    config.bootToConfig = configJson.get<bool>("bootToConfig");
    config.useDynIP = configJson.get<bool>("useStaticIP");

    // numeric
    config.port = configJson["port"].as<int>();
    config.mqtt_port = configJson["mqtt_port"].as<int>();
    config.timeoutWifimgr = configJson["timeoutWifimgr"].as<int>();
    config.portTR064 = configJson["portTR064"].as<int>();
    config.rgbLED_r = (uint8_t)configJson["rgbLED_r"].as<int>();
    config.rgbLED_g = (uint8_t)configJson["rgbLED_g"].as<int>();
    config.rgbLED_b = (uint8_t)configJson["rgbLED_b"].as<int>();
    config.rgbLED_anim = (uint8_t)configJson["rgbLED_anim"].as<int>();

    // string
    strlcpy(config.mac_dev[0], configJson.get<String>("mac_dev1").c_str(), sizeof(config.mac_dev[0]));
    strlcpy(config.mac_dev[1], configJson.get<String>("mac_dev2").c_str(), sizeof(config.mac_dev[1]));
    strlcpy(config.blynk_token, configJson.get<String>("blynk_token").c_str(), sizeof(config.blynk_token)); // <- destination's capacity
    strlcpy(config.ntp_server, configJson.get<String>("ntp_server").c_str(), sizeof(config.ntp_server));    // <- destination's capacity
    strlcpy(config.mqtt_server, configJson.get<String>("mqtt_server").c_str(), sizeof(config.mqtt_server)); // <- destination's capacity
    strlcpy(config.ipTR064, configJson.get<String>("ipTR064").c_str(), sizeof(config.ipTR064));
    strlcpy(config.userTR064, configJson.get<String>("userTR064").c_str(), sizeof(config.userTR064));             // <- destination's capacity
    strlcpy(config.passwordTR064, configJson.get<String>("passwordTR064").c_str(), sizeof(config.passwordTR064)); // <- destination's capacity
    strlcpy(config.timeWeekdaySBoff, configJson.get<String>("timeWeekdaySBoff").c_str(), sizeof(config.timeWeekdaySBoff));
    strlcpy(config.timeWeekdaySBon, configJson.get<String>("timeWeekdaySBon").c_str(), sizeof(config.timeWeekdaySBon));
    strlcpy(config.timeWeekendsSBoff, configJson.get<String>("timeWeekendsSBoff").c_str(), sizeof(config.timeWeekendsSBoff));
    strlcpy(config.timeWeekendsSBon, configJson.get<String>("timeWeekendsSBon").c_str(), sizeof(config.timeWeekendsSBon));
  }

  // Close the file (File's destructor doesn't close the file)
  file.close();
}

// Saves the configuration to a file
void saveConfiguration(const Config &config)
{
  Serial.println(F("CONFIG: writing config to file ..."));
  // Delete existing file, otherwise the configuration is appended to the file
  SPIFFS.remove(CONFIG_FILE);

  // Open file for writing
  File file = SPIFFS.open(CONFIG_FILE, "w");
  if (!file)
  {
    Serial.println(F("Failed to create file"));
    return;
  }

  // Allocate the memory pool on the stack
  // Don't forget to change the capacity to match your JSON document.
  // Use https://arduinojson.org/assistant/ to compute the capacity.
  StaticJsonBuffer<512> jsonBuffer;

  // Parse the configJson object
  JsonObject &configJson = jsonBuffer.createObject();

  // Set the values
  configJson.set("bootToConfig", config.bootToConfig);
  configJson.set("useDynIP", config.useDynIP);

  configJson.set("ntp_server", config.ntp_server);

  configJson.set("mqtt_server", config.mqtt_server);
  configJson.set("mqtt_port", config.mqtt_port);

  configJson.set("mqtt_server", config.mqtt_server);
  configJson.set("mqtt_port", config.mqtt_port);

  configJson.set("mac_dev1", config.mac_dev[0]);
  configJson.set("mac_dev2", config.mac_dev[1]);

  configJson.set("ipTR064", config.ipTR064);
  configJson.set("portTR064", config.portTR064);
  configJson.set("passwordTR064", config.passwordTR064);
  configJson.set("userTR064", config.userTR064);

  configJson.set("timeWeekdaySBon", config.timeWeekdaySBon);
  configJson.set("timeWeekdaySBoff", config.timeWeekdaySBoff);
  configJson.set("timeWeekendsSBoff", config.timeWeekendsSBoff);
  configJson.set("timeWeekendsSBon", config.timeWeekendsSBon);

  configJson.set("rgbLED_r", config.rgbLED_r);
  configJson.set("rgbLED_g", config.rgbLED_g);
  configJson.set("rgbLED_b", config.rgbLED_b);
  configJson.set("rgbLED_anim", config.rgbLED_anim);

  //configJson.set("port", config.port);

  /*
  configJson.set("static_ip", config.static_ip);
  configJson.set("static_gw", config.static_gw);    
  configJson.set("static_sn", config.static_sn);      
  */

  // Serialize JSON to file
  if (configJson.printTo(file) == 0)
  {
    Serial.println(F("Failed to write to file"));
  }

  // Close the file (File's destructor doesn't close the file)
  file.close();
  Serial.println(F("CONFIG: Done writing config to file"));
}

// Prints the content of a file to the Serial
void printFile(const char *filename)
{
  // Open file for reading
  File file = SPIFFS.open(CONFIG_FILE, "r");
  if (!file)
  {
    Serial.println(F("Failed to read file"));
    return;
  }

  // Extract each characters by one by one
  while (file.available())
  {
    Serial.print((char)file.read());
  }
  Serial.println();

  // Close the file (File's destructor doesn't close the file)
  file.close();
}

// check for valid mac adress
int isValidMacAddress(const char *mac)
{
  int indexCurrentDigit = 0; // index of current digit
  int seperatorCount = 0;    // seperatorCount

  while (*mac)
  {
    if (isxdigit(*mac))
    {
      indexCurrentDigit++;
    }
    else if (*mac == ':' || *mac == '-')
    {
      if (indexCurrentDigit == 0 || indexCurrentDigit / 2 - 1 != seperatorCount)
        break;
      ++seperatorCount;
    }
    else
    {
      seperatorCount = -1;
    }
    ++mac;
  }
  return (indexCurrentDigit == 12 && (seperatorCount == 5 || seperatorCount == 0));
}

#endif