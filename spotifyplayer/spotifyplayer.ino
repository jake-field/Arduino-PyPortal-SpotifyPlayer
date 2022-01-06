//Library Includes
#include "Adafruit_GFX.h"
#include "Adafruit_ILI9341.h"
#include "TouchScreen.h"
#include <WiFiNINA.h>

//Local Includes
#include "defines.h"
#include "arduino_secrets.h"
#include "wifihelper.h"
#include "clock.h"
#include "spotify.h"

//Global Variables
char ssid[] = SECRET_SSID; // your network SSID (name)
char pass[] = SECRET_PASS; // your network password (use for WPA, or use as key for WEP)

CWifiHelper g_wifi;
CSpotify g_spotifyAPI;
TouchScreen g_touchScreen = TouchScreen(XP, YP, XM, YM, 300);

// ILI9341 with 8-bit parallel interface:
Adafruit_ILI9341 tft = Adafruit_ILI9341(tft8bitbus, TFT_D0, TFT_WR, TFT_DC, TFT_CS, TFT_RST, TFT_RD);

//Implementation
void setup()
{
    //Init Serial
    Serial.begin(SERIAL_BAUDRATE);
    //while (!Serial); //Wait

    // Turn on backlight (required on PyPortal)
    pinMode(TFT_BACKLIGHT, OUTPUT);
    tft.begin();
    tft.setRotation(1);
    tft.fillScreen(ILI9341_BLACK);
    digitalWrite(TFT_BACKLIGHT, HIGH); //Turn on backlight here to avoid blinding the user with a white screen

    //Screen debugging
    tft.setCursor(0, 0);
    tft.setTextSize(1);
    tft.setTextColor(ILI9341_WHITE);
    tft.println("Wifi init()");

    //Create clock
    CClock &rClock = CClock::GetInstance();

    //Set up connection
    bool bWifiReady = g_wifi.Init();

    if (bWifiReady)
    {
        tft.print("Attempting to connect to ");
        tft.println(ssid);

        bool bConnected = g_wifi.Connect(ssid, pass);
        bConnected ? Serial.println("Wifi connected") : Serial.println("Wifi not connected");

        if (!bConnected)
        {
            tft.print("Failed to connect to ");
            tft.println(ssid);
            while (true)
                delay(1000); //Stop!
        }
        else
        {
            tft.println("Syncing clock with NTP");
            bool bClockSynced = rClock.SyncNTP();
            bClockSynced ? Serial.println("Clock synced") : Serial.println("Clock failed to sync");

            if (!bClockSynced)
            {
                int wifiTime = WiFi.getTime();

                while (wifiTime == 0 && (millis() < 20000))
                {
                    wifiTime = WiFi.getTime();
                }

                if (wifiTime != 0)
                {
                    CClock::GetInstance().SetUnixTime(wifiTime);
                    tft.println("Failed to sync time naturally, using WiFiNINA's unix time");
                }
                else
                {
                    tft.println("Failed to sync time, running without a synced clock");
                }
            }
        }
    }
    else
    {
        tft.println("Wifi failed init");
        while (true)
            delay(1000); //Stop!
    }

    tft.println("Init complete");

    g_spotifyAPI.Init(&tft);
    bool bAuthSuccess = g_spotifyAPI.RequestAuth();

    if (bAuthSuccess)
    {
        //g_spotifyAPI.SyncPlayerInfo(); //Initial sync
    }
    else
    {
        tft.fillScreen(ILI9341_BLACK); //Screen debugging
        tft.setCursor(0, 0);
        tft.setTextSize(1);
        tft.setTextColor(ILI9341_WHITE);
        tft.println("API auth failed. PROGRAM END");
        g_wifi.Disconnect(); //Prevents issues with networks reject us on reboot
        rClock.DestroyInstance();
        while (true)
            ; //Halt program
    }
}

void loop()
{
    TSPoint p = g_touchScreen.getPoint();
    p.x = map(p.x, X_MIN, X_MAX, 0, 240);
    p.y = map(p.y, Y_MIN, Y_MAX, 0, 320);

    g_spotifyAPI.ProcessPlayer((p.z > 100) ? p.x : 0, (p.z > 100) ? p.y : 0);
    g_spotifyAPI.DrawPlayer();

    delay(1); //Slow tick rate
}
