#pragma once
#ifndef __SPOTIFY_H__
#define __SPOTIFY_H__

//Library Includes
#include <Arduino_OAuth.h>
#include <JPEGDEC.h>

//Local Includes
#include "spotifytypes.h"

//Prototype
class Adafruit_ILI9341;
class CSpotify
{
    //Member Functions
public:
    CSpotify();
    ~CSpotify();

    void Init(Adafruit_ILI9341 *_pTFT);

    void ProcessPlayer(int _iTouchX, int _iTouchY);
    void DrawPlayer();

    //Player controls
    void Play();
    void Pause();
    void SkipForwards();
    void SkipBackwards();

    bool IsPaused() const;

    void SetPlaybackPosition(int _iPlaybackPosition); //Seek to x(ms)
    int GetPlaybackPosition() const;

    void SetVolume(int _iVolume);
    int GetVolume() const;

    //Get user to authorize us for playback controls
    //	Set up WifiServer
    //	Get address
    //	Display QR
    //	Build page which redirects to spotify auth
    //		REF: https://developer.spotify.com/documentation/general/guides/authorization-guide/
    //		REF: https://developer.spotify.com/documentation/general/guides/scopes/
    //		- user-read-playback-state
    //		- user-modify-playback-state
    //		- user-read-currently-playing
    //		- user-read-playback-position
    //	Wait for callback for auth token
    bool RequestAuth();

    //Update player information
    //	REF: https://developer.spotify.com/documentation/web-api/reference/
    bool SyncPlayerInfo();

    //Pull the album art and draw
    bool LoadAlbumArt(int _x, int _y, int _size = 2);
    static int DrawAlbumArt(JPEGDRAW *pDraw);

    //Member Variables
protected:
    WiFiSSLClient m_wifiSSLClient;
    OAuthClient *m_pOAuthClient;

    static Adafruit_ILI9341 *m_pTFT; //HACK: static due to crappy 3rd party jpg libraries which insist on drawing decoded jpgs for you >:C
    TPlayer m_tPlayerState;
};

#endif //__SPOTIFY_H__
