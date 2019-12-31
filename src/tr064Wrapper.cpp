#include "tr064Wrapper.h"
#include "configstruct.h"

bool TR064Wrapper::testForTR064support(Config &config)
{
	if (!flagTr064Supported && !flagTr064Initalized)
	{
		flagTr064Supported = client.connect(config.ipTR064, config.portTR064);
	}
	return (flagTr064Supported);
}

bool TR064Wrapper::initTr064(Config &config)
{
	if (flagTr064Supported && !flagTr064Initalized)
	{
		tr064.init(config.portTR064, config.ipTR064, config.userTR064, config.passwordTR064);
		flagTr064Initalized = true;
	}
	return (flagTr064Initalized);
};

void TR064Wrapper::updateOnlineUser(Config &config)
{
	//For the next round, assume all users are offline
	for (int i = 0; i < NUM_USER; ++i)
	{
		onlineUsers[i] = false;
	}

	boolean flagNoDeviceOnline = true; //No online device found yet

	//Check for all users if at least one of the macs is online
	for (int i = 0; i < NUM_USER && flagNoDeviceOnline; ++i)
	{
		//Serial.printf("> USER %d -------------------------------\n", i);

		//Get the mac of the device to check
		//String curMac = macsPerUser[j];
		String curMac = config.mac_dev[i];
		//String curMac = macsPerUser[i][j];

		flagNoDeviceOnline = (curMac != ""); //If it is empty, we don't need to check it (or the next one)

		if (flagNoDeviceOnline)
		{
			//okay, ask the router for the status of this MAC
			String stat2[4][2];
			getStatusOfMAC(curMac, stat2);

			// Is the device (mac adress) online?
			if (stat2[STATUS_ACTIVE_INDEX][1] != "" && stat2[STATUS_ACTIVE_INDEX][1] != "0")
			{
				onlineUsers[i] = true;
				flagNoDeviceOnline = true;
				Serial.printf("TR064: Device %s of User%u is online\n", curMac.c_str(), i);
			}
			else
			{
				Serial.printf("TR064: Device %s of User%u is offline\n", curMac.c_str(), i);
			}
		}
	}
}

/**
* Prints the status of the device on the screen (arrays r of the getStatusOfMAC methods).
* return nothing
*/
void TR064Wrapper::getStatusOfMAC(String mac, String (&r)[4][2])
{
	//Ask for one specific device
	String params[][2] = {{"NewMACAddress", mac}};
	String req[][2] = {{"NewIPAddress", ""}, {"NewActive", ""}, {"NewHostName", ""}};
	tr064.action("urn:dslforum-org:service:Hosts:1", "GetSpecificHostEntry", params, 1, req, 2);
	//Serial.println(mac + " is online " + (req[1][1]));
	//Serial.flush();
	r[STATUS_MAC_INDEX][0] = STATUS_MAC;
	r[STATUS_MAC_INDEX][1] = mac;
	r[STATUS_IP_INDEX][0] = STATUS_IP;
	r[STATUS_IP_INDEX][1] = req[0][1];
	r[STATUS_HOSTNAME_INDEX][0] = STATUS_HOSTNAME;
	r[STATUS_HOSTNAME_INDEX][1] = req[2][1];
	r[STATUS_ACTIVE_INDEX][0] = STATUS_ACTIVE;
	r[STATUS_ACTIVE_INDEX][1] = req[1][1];
}
