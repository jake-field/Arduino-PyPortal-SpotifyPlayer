//Library Includes
#include <SPI.h>
#include <WiFiNINA.h>
#include <WiFiUdp.h>

//This Include
#include "clock.h"

//Static def
CClock *CClock::m_pThis = nullptr;

//Implementation
CClock::CClock()
    : m_ulLastUnixTimestamp(0)
    , m_ulLastUnixTime(0)
{
    //Constructor
}

CClock::~CClock()
{
    //Destructor
}

CClock
&CClock::GetInstance()
{
    if (!m_pThis) m_pThis = new CClock;
    return (*m_pThis);
}

void
CClock::DestroyInstance()
{
    if (m_pThis)
    {
        delete m_pThis;
        m_pThis = nullptr;
    }
}

bool
CClock::SyncNTP()
{
    //UDP
    //Request NTP response
    //Sync
    bool bSynced = false;

    //If the wifi module is connected (We won't do that here)
    if (WiFi.status() == WL_CONNECTED)
    {
        //NTP timestamp is in the first 48 bytes of the message
        const int NTP_PACKET_SIZE = 48;

        //Open UDP socket
        WiFiUDP Udp;
        bool bSocketOpen = Udp.begin(2390); //Random port, magic numbers mmmmm

        //sendNTPpacket()
        byte packetBuffer[NTP_PACKET_SIZE];
        memset(packetBuffer, 0, NTP_PACKET_SIZE);
        packetBuffer[0] = 0b11100011; //LI, Version, Mode
        packetBuffer[1] = 0;          //Stratum, or type of clock
        packetBuffer[2] = 6;          //Polling Interval
        packetBuffer[3] = 0xEC;       //Peer Clock Precision
        //8 bytes of zero for Root Delay & Root Dispersion
        packetBuffer[12] = 49;
        packetBuffer[13] = 0x4E;
        packetBuffer[14] = 49;
        packetBuffer[15] = 52;

        //Before we send, store the current time (it's okay if this not correct)
        unsigned long ulPacketTime = GetUnixTime();

        //Send UDP packet to NTP servers
        IPAddress timeServer(129, 6, 15, 28); //time.nist.gov NTP server
        Udp.beginPacket(timeServer, 123);     //NTP requests are to port 123
        Udp.write(packetBuffer, NTP_PACKET_SIZE);
        Udp.endPacket();

        //Wait for packet until recieved or timed-out
        const unsigned long ulTimeout = 10;
        while (GetUnixTime() < (ulPacketTime + ulTimeout))
        {
            //Check if packet has arrived
            if (Udp.parsePacket())
            {
                Serial.println("NTP packet recieved");

                Udp.read(packetBuffer, NTP_PACKET_SIZE); //Read the packet into the buffer

                //The timestamp starts at byte 40 of the received packet and is four bytes,
                //	or two words, long. First, extract the two words:
                unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
                unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);

                //Combine the four bytes (two words) into a long integer
                //This is NTP time (seconds since Jan 1 1900):
                unsigned long secsSince1900 = highWord << 16 | lowWord;
                Serial.print("Seconds since Jan 1 1900 = ");
                Serial.println(secsSince1900);

                // now convert NTP time into everyday time:
                Serial.print("Unix time = ");

                // Unix time starts on Jan 1 1970. In seconds, that's 2208988800:
                const unsigned long seventyYears = 2208988800UL;

                // subtract seventy years:
                m_ulLastUnixTime = secsSince1900 - seventyYears;
                m_ulLastUnixTimestamp = millis(); //Timestamp is relative to device start

                // print Unix time:
                Serial.println(m_ulLastUnixTime);

                //Exit the while loop early
                bSynced = true;
                break;
            }

            //Don't burn power
            delay(100);
        }

        //Close UDP socket
        Udp.stop();
    }

    //Returns true if time was successfully synced
    return (bSynced);
}

void
CClock::SetUnixTime(unsigned long _ulTime)
{
    m_ulLastUnixTime = _ulTime;
}

unsigned long
CClock::GetTime()
{
    return (millis() / 1000);
}

unsigned long
CClock::GetUnixTime()
{
    unsigned long ulUnixTime = 0;

    //Static function, need to call back to ourselves
    if (m_pThis)
    {
        //Use stored Unix time provided by NTP (if it were synced)
        ulUnixTime = m_pThis->m_ulLastUnixTime;

        //Calculate (time since code started) - timestamp to get a (near) accurate unix time
        ulUnixTime += (millis() - m_pThis->m_ulLastUnixTimestamp) / 1000;
    }

    return (ulUnixTime);
}
