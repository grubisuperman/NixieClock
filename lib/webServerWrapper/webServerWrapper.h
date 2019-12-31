#ifndef _WebServerWrapper_h
#define _WebServerWrapper_h


//#include <WebServer.h>
#if defined(ESP8266)
#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino
#else
#include <WiFi.h>
#endif

#include <Hash.h>
#include <ESPAsyncWebServer.h>
#include <functional>
#include "FS.h"

// size of buffer used to capture HTTP requests
#define REQ_BUF_SZ   60

struct Config;


class WebServerWrapper {
    public:

        
        WebServerWrapper(AsyncWebServer *server, Config *ptrConfig);      
        
        void init();
      

        
    private:
        AsyncWebServer *server;
        Config *ptrConfig;


};

#endif