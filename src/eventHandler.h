#ifndef _EventHandler_h
#define _EventHandler_h

//flag for wifi connection
bool flag_wifiConnected = false;

bool flag_wifiFirstConnected = false;

//flag for ntp sync event. Evaluate in main loop
bool syncEventTriggered = false;

NTPSyncEvent_t ntpEvent; // get last triggered ntp event
/* NTP events
timeSyncd,		// Time successfully got from NTP server
noResponse,		// No response from server
invalidAddress	// Address not reachable
*/

void processSyncEvent(NTPSyncEvent_t ntpEvent);
void setupEventHandler();

#endif
