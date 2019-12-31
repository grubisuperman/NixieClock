

#include "webServerWrapper.h"
#include "AsyncJson.h"
#include "ArduinoJson.h"
#include "../../src/configstruct.h"

WebServerWrapper::WebServerWrapper(AsyncWebServer *server, Config *ptrConfig)
    : server(server),
      ptrConfig(ptrConfig){};

void WebServerWrapper::init()
{
    server->serveStatic("/webfonts", SPIFFS, "/webfonts");
    server->serveStatic("/js", SPIFFS, "/js");
    server->serveStatic("/css", SPIFFS, "/css");
    server->serveStatic("/", SPIFFS, "/").setDefaultFile("index.html");

    server->on("/getConfig.json", HTTP_GET, [this](AsyncWebServerRequest *request) {
        Serial.println(F("WebServerWrapper: GET /getConfig recieved"));
        StaticJsonBuffer<312> jsonBuffer;
        JsonObject &configJson = jsonBuffer.createObject();

        configJson.set("bootToConfig", this->ptrConfig->bootToConfig);
        configJson.set("timeWeekdaySBon", this->ptrConfig->timeWeekdaySBon);
        configJson.set("timeWeekdaySBoff", this->ptrConfig->timeWeekdaySBoff);
        configJson.set("timeWeekendsSBon", this->ptrConfig->timeWeekendsSBon);
        configJson.set("timeWeekendsSBoff", this->ptrConfig->timeWeekendsSBoff);
        configJson.set("mac1", this->ptrConfig->mac_dev[0]);
        configJson.set("mac2", this->ptrConfig->mac_dev[1]);
        configJson.set("ipTR064", this->ptrConfig->ipTR064);
        configJson.set("portTR064", this->ptrConfig->portTR064);
        configJson.set("useTR064", this->ptrConfig->useTR064);
        configJson.set("rgbLED_r", this->ptrConfig->rgbLED_r);
        configJson.set("rgbLED_g", this->ptrConfig->rgbLED_g);
        configJson.set("rgbLED_b", this->ptrConfig->rgbLED_b);
        configJson.set("rgbLED_anim", this->ptrConfig->rgbLED_anim);
        

        configJson.prettyPrintTo(Serial);
        String json;
        configJson.printTo(json);

        //String json = "{\"timeWeekdaySBon\":\"13:36\",\"timeWeekdaySBoff\":\"05:30\",\"timeWeekendsSBoff\":\"14:14\",\"timeWeekendsSBon\":\"16:36\",\"mac1\":\"F4:09:D8:66:42:45\",\"mac2\":\"E0:33:8E:4B:38:C6\",\"ipTR064\":\"192.168.178.1\",\"portTR064\":\"49000\",\"useTR064\":\"true\"}";
        request->send(200, "application/json", json);
        Serial.print(F("WebServerWrapper: RESPONSE application/json:"));
        Serial.println(json);
    });


    AsyncCallbackJsonWebHandler *handlerPostConfig = new AsyncCallbackJsonWebHandler("/postConfig.json", [this](AsyncWebServerRequest *request, JsonVariant &json) {
        Serial.println(F("WebServerWrapper: JSON /postConfig.json recieved"));
        JsonObject &jsonObj = json.as<JsonObject>();
        if (jsonObj.success())
        {
            jsonObj.prettyPrintTo(Serial);

            strlcpy(this->ptrConfig->timeWeekdaySBon, jsonObj.get<String>("timeWeekdaySBon").c_str(), sizeof(this->ptrConfig->timeWeekdaySBon));
            strlcpy(this->ptrConfig->timeWeekdaySBoff, jsonObj.get<String>("timeWeekdaySBoff").c_str(), sizeof(this->ptrConfig->timeWeekdaySBoff));
            strlcpy(this->ptrConfig->timeWeekendsSBon, jsonObj.get<String>("timeWeekendsSBon").c_str(), sizeof(this->ptrConfig->timeWeekendsSBon));
            strlcpy(this->ptrConfig->timeWeekendsSBoff, jsonObj.get<String>("timeWeekendsSBoff").c_str(), sizeof(this->ptrConfig->timeWeekendsSBoff));
            strlcpy(this->ptrConfig->mac_dev[0], jsonObj.get<String>("mac1").c_str(), sizeof(this->ptrConfig->mac_dev[0]));
            strlcpy(this->ptrConfig->mac_dev[1], jsonObj.get<String>("mac2").c_str(), sizeof(this->ptrConfig->mac_dev[1]));
            strlcpy(this->ptrConfig->ipTR064, jsonObj.get<String>("ipTR064").c_str(), sizeof(this->ptrConfig->ipTR064));
            this->ptrConfig->portTR064 = jsonObj.get<int>("portTR064");
            this->ptrConfig->useTR064 = jsonObj.get<bool>("useTR064");
            this->ptrConfig->bootToConfig = jsonObj.get<bool>("bootToConfig");

            this->ptrConfig->shouldSaveConfig = true;
            request->send(200, "text/plain", "OK");
            return;
        }
        else
        {
            Serial.println(F("Internal Server Error"));
            request->send(500, "text/plain", "Internal Server Error");
        }
    });
    server->addHandler(handlerPostConfig);

    AsyncCallbackJsonWebHandler *handlerPostLedColor = new AsyncCallbackJsonWebHandler("/postLedColor.json", [this](AsyncWebServerRequest *request, JsonVariant &json) {
        Serial.println(F("WebServerWrapper: JSON /postLedColor.json recieved"));
        JsonObject &jsonObj = json.as<JsonObject>();
        if (jsonObj.success())
        {
            jsonObj.prettyPrintTo(Serial);
            this->ptrConfig->rgbLED_r =  (uint8_t) jsonObj.get<int>("r");
            this->ptrConfig->rgbLED_g =  (uint8_t) jsonObj.get<int>("g");
            this->ptrConfig->rgbLED_b =  (uint8_t) jsonObj.get<int>("b");
            
            this->ptrConfig->shouldSaveConfig = true;
            request->send(200, "text/plain", "OK");
            return;
        }
        else
        {
            Serial.println(F("Internal Server Error"));
            request->send(500, "text/plain", "Internal Server Error");
        }
    });
    server->addHandler(handlerPostLedColor);

    AsyncCallbackJsonWebHandler *handlerPostLedAnim = new AsyncCallbackJsonWebHandler("/postLedAnim.json", [this](AsyncWebServerRequest *request, JsonVariant &json) {
        Serial.println(F("WebServerWrapper: JSON /postLedAnim.json recieved"));
        JsonObject &jsonObj = json.as<JsonObject>();
        if (jsonObj.success())
        {
            jsonObj.prettyPrintTo(Serial);
            this->ptrConfig->rgbLED_anim = (uint8_t) jsonObj.get<int>("anim");

            this->ptrConfig->shouldSaveConfig = true;
            request->send(200, "text/plain", "OK");
            return;
        }
        else
        {
            Serial.println(F("Internal Server Error"));
            request->send(500, "text/plain", "Internal Server Error");
        }
    });
    server->addHandler(handlerPostLedAnim);



    server->onNotFound([](AsyncWebServerRequest *request) {
        Serial.print(F("WebServerWrapper: url "));
        Serial.print(request->url());
        Serial.println(F(" not found"));
        request->send(404, "text/plain", "Not found");
    });

    server->begin();
    Serial.println(F("WebServerWrapper: webserver initilized"));
};
