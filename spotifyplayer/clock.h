#pragma once
#ifndef __CLOCK_H__
#define __CLOCK_H__

//Prototype
class CClock
{
    //Member Functions
public:
    static CClock &GetInstance();
    static void DestroyInstance();

    bool SyncNTP();

    void SetUnixTime(unsigned long _ulTime);
    unsigned long GetTime();            //Returns time in seconds since program start
    static unsigned long GetUnixTime(); //Returns unix time in seconds

private:
    CClock();
    ~CClock();

    //Member Variables
protected:
    static CClock *m_pThis;
    unsigned long m_ulLastUnixTimestamp; //Offset std::time
    unsigned long m_ulLastUnixTime;      //Last receieved update time
};

#endif //__CLOCK_H__
