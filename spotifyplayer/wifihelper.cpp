//Library Includes
#include <WiFiNINA.h>

//This Include
#include "wifihelper.h"

//Implementation
CWifiHelper::CWifiHelper()
{
    //Constructor
}

CWifiHelper::~CWifiHelper()
{
    //Destructor
    WiFi.end(); //Turn off wifi
}

bool CWifiHelper::Init(uint32_t _uiTimeout)
{
    bool bHealthy = true;

    //Check WIFI module
    if (WiFi.status() == WL_NO_MODULE)
    {
        Serial.println("Error: ESP32 failure");
        bHealthy = false;
    }

    //Firmware version check
    String fv = WiFi.firmwareVersion();
    if (fv < WIFI_FIRMWARE_LATEST_VERSION)
    {
        Serial.println("Warning: ESP32 firmware needs updating");
    }

    //Set the timout for when we attempt to connect
    WiFi.setTimeout(_uiTimeout);

    //Returns true if wifi is healty and ready to go
    return (bHealthy);
}

bool CWifiHelper::Connect(String _strSSID, String _strPass)
{
    //Try find the network first, some networks take a moment to appear
    int iMaxAttempts = 3;
    bool bFoundNetwork = false; //Breakout
    for (int i = 0; i < iMaxAttempts; ++i)
    {
        Serial.println("Scanning for networks...");
        int iNetworks = WiFi.scanNetworks();
        Serial.print("Discovered ");
        Serial.print(iNetworks);
        Serial.println(" network(s)");

        for (int j = 0; j < iNetworks; ++j)
        {
            if (!strcmp(WiFi.SSID(j), _strSSID.c_str()))
            {
                Serial.print("Found ");
                Serial.println(WiFi.SSID(j));
                bFoundNetwork = true;
                break;
            }
        }

        if (bFoundNetwork)
            break;
        Serial.print("Failed to find desired SSID... (Attempt ");
        Serial.print((i + 1));
        Serial.print(" of ");
        Serial.print(iMaxAttempts);
        Serial.println(")");
        if (i < iMaxAttempts)
            delay(10000); //wait
    }

    if (!bFoundNetwork)
        Serial.println("Could not find network, it could be hidden...");

    int status;
    for (int i = 0; i < iMaxAttempts; ++i)
    {
        Serial.print("Attempting to connect to ");
        Serial.println(_strSSID);

        //Attempt connection
        status = WiFi.begin(_strSSID.c_str(), _strPass.c_str());

        if (status != WL_CONNECTED)
        {
            Serial.print("Failed to connect to ");
            Serial.println(_strSSID);
            Serial.print("\tReason: ");
            Serial.println(WiFi.reasonCode());
        }
        else
        {
            Serial.println("Connected!");
            break;
        }

        Serial.print("Failed to connect... (Attempt ");
        Serial.print((i + 1));
        Serial.print(" of ");
        Serial.print(iMaxAttempts);
        Serial.println(")");
        if (i < iMaxAttempts)
            delay(10000); //wait
    }

    if (status != WL_CONNECTED)
    {
        Serial.println("Gave up connection attempt after max attempts was reached.");
        WiFi.end(); //Turn off wifi due to error
    }

    //Return true if we successfully connected
    return (status == WL_CONNECTED);
}

void CWifiHelper::Disconnect()
{
    WiFi.disconnect();
}

int CWifiHelper::GetStatus() const
{
    return (WiFi.status());
}
