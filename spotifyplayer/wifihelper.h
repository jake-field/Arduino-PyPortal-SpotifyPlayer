#pragma once
#ifndef __WIFI_HELPER_H__
#define __WIFI_HELPER_H__

//Prototype
class CWifiHelper
{
	//Member Functions
public:
	CWifiHelper();
	~CWifiHelper();

	bool Init(uint32_t _uiTimeout = 30000);
	bool Connect(String _strSSID, String _strPass);
	void Disconnect();

	int GetStatus() const;
};

#endif //__WIFI_HELPER_H__
