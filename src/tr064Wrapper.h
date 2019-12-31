#ifndef _tr064Wrapper_h
#define _tr064Wrapper_h

//    FILE: tr064Wrapper.h
//  AUTHOR: CvG
// PURPOSE: Simple wrapper for tr064
// VERSION: 1.0
//
#include "tr064.h"
//#include <ESP8266WiFi.h>

#define NUM_USER 2 // number of users or devices to check for
struct Config;

class TR064Wrapper
{
public:
    TR064Wrapper() {}

    WiFiClient client;
    TR064 tr064;

    //bool isInitilized();
    bool testForTR064support(Config &config);
    bool initTr064(Config &config);
    bool onlineUsers[NUM_USER];
    bool flagTr064Supported = false;
    bool flagTr064Initalized = false;
    //bool isUserOnline(char * MACs[], byte numberOfMACs);
    void updateOnlineUser(Config &config);
    void getStatusOfMAC(String mac, String (&r)[4][2]);

private:
    //Array-settings. No need to change these!
    const String STATUS_MAC = "MAC";
    const String STATUS_IP = "IP";
    const String STATUS_ACTIVE = "ACTIVE";
    const String STATUS_HOSTNAME = "HOSTNAME";
    const int STATUS_MAC_INDEX = 0;
    const int STATUS_IP_INDEX = 1;
    const int STATUS_ACTIVE_INDEX = 3;
    const int STATUS_HOSTNAME_INDEX = 2;
};

#endif