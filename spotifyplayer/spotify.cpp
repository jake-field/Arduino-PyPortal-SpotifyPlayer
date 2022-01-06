//Library Includes
#include <WiFiNINA.h>
#include "Adafruit_ILI9341.h"
#include "Adafruit_GFX.h"
#include <qrcode.h>

#define ARDUINOJSON_USE_LONG_LONG 1
#include <ArduinoJson.h>

//Local Includes
#include "arduino_secrets.h"
#include "clock.h"

//This Include
#include "spotify.h"

//Implementation
Adafruit_ILI9341* CSpotify::m_pTFT = nullptr;

CSpotify::CSpotify()
    : m_pOAuthClient(nullptr)
{
    //Constructor
}

CSpotify::~CSpotify()
{
    //Destructor
    if (m_pOAuthClient)
    {
        delete m_pOAuthClient;
        m_pOAuthClient = nullptr;
    }
}

void
CSpotify::Init(Adafruit_ILI9341* _pTFT)
{
    m_pTFT = _pTFT;
    m_pOAuthClient = new OAuthClient(m_wifiSSLClient, g_strApiUrl, 443);
    m_pOAuthClient->setCredentials("", "", "", "");

    //Use our clock for reliable NTP sync?
    //Otherwise WiFiNINA has a built in NTP clock now
    m_pOAuthClient->onGetTime(CClock::GetUnixTime);
}

void
CSpotify::ProcessPlayer(int _iTouchX, int _iTouchY)
{
    //TODO: Interpolate track progress between API requests
    //      Not possible unless we sort out some sort of async

    //Flipped due to screen orientation
    int iPlayButtonRadius = 35;
    int iPlayButtonY = (m_pTFT->width() / 2);
    int iPlayButtonX = (int)((float)iPlayButtonRadius * 1.75f);

    if(_iTouchX > (iPlayButtonX - iPlayButtonRadius) && _iTouchX < (iPlayButtonX + iPlayButtonRadius) &&
        _iTouchY > (iPlayButtonY - iPlayButtonRadius) && _iTouchY < (iPlayButtonY + iPlayButtonRadius))
    {
        if(IsPaused())
        {
            Play();
            //SetVolume(50);
        }
        else
        {
            Pause();
        }
    }

    //Progress interpolation
    static int iPauseTime = 0;
    int iProgress = m_tPlayerState.iProgress / 1000;
    if (!m_tPlayerState.bPlaying)
    {
        if (iPauseTime == 0) iPauseTime = CClock::GetInstance().GetUnixTime(); //If we weren't paused before, store the unix time
        iProgress += (iPauseTime - m_tPlayerState.ulTimestampLocal); //Set at paused time, this may provide weird results when Sync() is called
    }
    else
    {
        iPauseTime = 0; //Unset pause time
        iProgress += (CClock::GetInstance().GetUnixTime() - m_tPlayerState.ulTimestampLocal); //Interpolate progress from unix time
    }
    float fProgress = ((float)iProgress) / (float)(m_tPlayerState.tItem.iDuration / 1000);

    //Update player state every 10 seconds or if the song is at 99% or higher
    if ((m_tPlayerState.ulTimestampLocal + 10) < CClock::GetInstance().GetUnixTime() || fProgress > 99.0f)
    {
        SyncPlayerInfo();
    }
}

void
CSpotify::DrawPlayer()
{
    if (m_pTFT)
    {
        //TODO: Only clear RECT areas which have updates
        static String strPrevSong = "";
        static int iPauseTime = 0;
        static int iPrevTime = 0;
        static bool bPlaying = false;
        bool bForceRedraw = false;
        int iProgress = m_tPlayerState.iProgress / 1000;

        if (!m_tPlayerState.bPlaying)
        {
            if (iPauseTime == 0) iPauseTime = CClock::GetInstance().GetUnixTime(); //If we weren't paused before, store the unix time
            iProgress += (iPauseTime - m_tPlayerState.ulTimestampLocal); //Set at paused time, this may provide weird results when Sync() is called
        }
        else
        {
            iPauseTime = 0; //Unset pause time
            iProgress += (CClock::GetInstance().GetUnixTime() - m_tPlayerState.ulTimestampLocal); //Interpolate progress from unix time
        }

        //If new song or 1 second, force full screen update
        if (CClock::GetInstance().GetUnixTime() > iPrevTime)
        {
            //Clear screen
            if (m_tPlayerState.tItem.strName != strPrevSong)
            {
                m_pTFT->fillScreen(ILI9341_BLACK);
                bForceRedraw = true;

                //TODO: consider changing this to draw into a GFX canvas so that we can dim the image and make blurred cutouts for the media controls etc
                int iImageSize = 1;
                int iImageX = (m_pTFT->width() - m_tPlayerState.tItem.tAlbum.tImages[iImageSize].iWidth) / 2;
                int iImageY = (m_pTFT->height() - m_tPlayerState.tItem.tAlbum.tImages[iImageSize].iHeight) / 2;
                LoadAlbumArt(iImageX, iImageY, iImageSize);
            }

            int iProgWidth = 270;
            int iProgHeight = 8;
            int iProgRounding = 10;
            int iSeekRadius = 3;
            int iProgX = (m_pTFT->width() - iProgWidth) / 2;
            int iProgY = (m_pTFT->height() - iProgHeight) / 2;

            float fProgress = ((float)iProgress) / (float)(m_tPlayerState.tItem.iDuration / 1000);
            //Draw progress bar background
            m_pTFT->fillRoundRect(iProgX, iProgY, iProgWidth, iProgHeight, iProgRounding, ILI9341_DARKGREY);

            //Draw progress bar foreground
            m_pTFT->fillRoundRect(iProgX, iProgY, (int)(((float)iProgWidth) * fProgress), iProgHeight, iProgRounding, ILI9341_DARKGREEN);

            //Draw seek ball
            m_pTFT->fillCircle(iProgX + (int)(((float)iProgWidth) * fProgress), (iProgY + iSeekRadius), iSeekRadius, ILI9341_LIGHTGREY);


            //Draw song information
            m_pTFT->setCursor(iProgX, iProgY - 35);
            m_pTFT->setTextWrap(false);
            m_pTFT->setTextSize(2);
            m_pTFT->setTextColor(ILI9341_WHITE, ILI9341_BLACK); //fg bg
            m_pTFT->println(m_tPlayerState.tItem.strName);
            m_pTFT->setTextSize(1);

            m_pTFT->setCursor(iProgX, m_pTFT->getCursorY());
            m_pTFT->println(m_tPlayerState.tItem.tAlbum.strName);

            m_pTFT->setCursor(iProgX, m_pTFT->getCursorY());
            m_pTFT->println(m_tPlayerState.tItem.tArtists[0].strName);
            m_pTFT->setTextWrap(true); //reset for any other code


            //Draw progress timers
            m_pTFT->setTextColor(ILI9341_WHITE, ILI9341_DARKGREY); //fg bg
            //TODO: Convert to (-)h:M:SS
            //TODO: Draw a rounded box behind for pretty
            m_pTFT->setTextSize(1);
            m_pTFT->setCursor(iProgX, iProgY + iProgHeight);

            m_pTFT->print(iProgress / 60);
            m_pTFT->print(":");
            if(iProgress % 60 < 10) m_pTFT->print("0");
            m_pTFT->println(iProgress % 60);

            m_pTFT->setCursor((m_pTFT->width() - iProgX) - (4 * 6), iProgY + iProgHeight);

            int iDuration = m_tPlayerState.tItem.iDuration / 1000;
            m_pTFT->print(iDuration / 60);
            m_pTFT->print(":");
            if(iDuration % 60 < 10) m_pTFT->print("0");
            m_pTFT->println(iDuration % 60);

            //Only redraw play button if we need to
            if (bPlaying != m_tPlayerState.bPlaying || bForceRedraw)
            {
                //Draw Play Button
                int iButtonRadius = 35;
                int iButtonIconSize = iButtonRadius / 2;
                int iButtonX = (m_pTFT->width() / 2);// - iButtonRadius;
                int iButtonY = m_pTFT->height() - (int)((float)iButtonRadius * 1.75f);

                m_pTFT->fillCircle(iButtonX, iButtonY, iButtonRadius, ILI9341_DARKGREEN);

                //Draw play arrow or pause bars on top of play button
                if (m_tPlayerState.bPlaying)
                {
                    m_pTFT->fillRect((iButtonX - iButtonIconSize) + (iButtonIconSize / 4), iButtonY - iButtonIconSize, iButtonIconSize / 2, iButtonIconSize * 2, ILI9341_LIGHTGREY);
                    m_pTFT->fillRect((iButtonX) + (iButtonIconSize / 4), iButtonY - iButtonIconSize, iButtonIconSize / 2, iButtonIconSize * 2, ILI9341_LIGHTGREY);
                }
                else
                {
                    m_pTFT->fillTriangle(iButtonX - iButtonIconSize, iButtonY - iButtonIconSize,
                                         iButtonX + iButtonIconSize, iButtonY,
                                         iButtonX - iButtonIconSize, iButtonY + iButtonIconSize,
                                         ILI9341_LIGHTGREY);
                }
            }

            //Frame to frame updates
            bForceRedraw = false;
            bPlaying = m_tPlayerState.bPlaying;
            strPrevSong = m_tPlayerState.tItem.strName;
            iPrevTime = CClock::GetInstance().GetUnixTime();
        }
    }
}

void
CSpotify::Play()
{
    //Needs valid context to play or this will fail silently
    Serial.print("Play() ... ");
    m_pOAuthClient->put(g_ktSpotifyApiActions[(int)EApiEndpoints::RESUME_PLAYBACK].strURL, "application/json", "");
    int statusCode = m_pOAuthClient->responseStatusCode();
    Serial.println(statusCode == 204 ? "Success" : String(statusCode));
    if(statusCode == 204) m_tPlayerState.bPlaying = true;
}

void
CSpotify::Pause()
{
    Serial.print("Pause() ... ");
    m_pOAuthClient->put(g_ktSpotifyApiActions[(int)EApiEndpoints::PAUSE_PLAYBACK].strURL, "application/json", "");
    int statusCode = m_pOAuthClient->responseStatusCode();
    Serial.println(statusCode == 204 ? "Success" : String(statusCode));
    if(statusCode == 204) m_tPlayerState.bPlaying = false;
}

void
CSpotify::SkipForwards()
{
    Serial.print("SkipForwards() ... ");
    m_pOAuthClient->post(g_ktSpotifyApiActions[(int)EApiEndpoints::SKIP_NEXT].strURL, "application/json", "");
    int statusCode = m_pOAuthClient->responseStatusCode();
    Serial.println(statusCode == 204 ? "Success" : String(statusCode));
}

void
CSpotify::SkipBackwards()
{
    Serial.print("SkipBackwards() ... ");
    m_pOAuthClient->post(g_ktSpotifyApiActions[(int)EApiEndpoints::SKIP_PREV].strURL, "application/json", "");
    int statusCode = m_pOAuthClient->responseStatusCode();
    Serial.println(statusCode == 204 ? "Success" : String(statusCode));
}

bool
CSpotify::IsPaused() const
{
    return(!m_tPlayerState.bPlaying);
}

void
CSpotify::SetPlaybackPosition(int _iPlaybackPosition)
{
    Serial.print("SetPlaybackPosition(" + String(_iPlaybackPosition) + ") ... ");
    m_pOAuthClient->put(g_ktSpotifyApiActions[(int)EApiEndpoints::SEEK].strURL + String(_iPlaybackPosition), "application/json", "");
    int statusCode = m_pOAuthClient->responseStatusCode();
    Serial.println(statusCode == 204 ? "Success" : String(statusCode));
}

int
CSpotify::GetPlaybackPosition() const
{
    return(m_tPlayerState.iProgress);
}

void
CSpotify::SetVolume(int _iVolume)
{
    Serial.print("SetVolume(" + String(_iVolume) + ") ... ");
    m_pOAuthClient->put(g_ktSpotifyApiActions[(int)EApiEndpoints::SET_PLAYBACK_VOLUME].strURL + String(_iVolume), "application/json", "");
    int statusCode = m_pOAuthClient->responseStatusCode();
    Serial.println(statusCode == 204 ? "Success" : String(statusCode));
}

int
CSpotify::GetVolume() const
{
    return(m_tPlayerState.tDevice.iVolume);
}

bool
CSpotify::RequestAuth()
{
    Serial.println("RequestAuth()");

    //Clear screen
    m_pTFT->fillScreen(ILI9341_BLACK);

    //TODO: don't abuse Implicit Grant, use proper auth
    IPAddress myIP = WiFi.localIP();
    String strIP = String(myIP[0]);
    strIP += ".";
    strIP += String(myIP[1]);
    strIP += ".";
    strIP += String(myIP[2]);
    strIP += ".";
    strIP += String(myIP[3]);

    Serial.print("IP: ");
    Serial.println(strIP);

    //https://accounts.spotify.com/authorize?client_id=SECRET_CLIENT_ID&redirect_uri=MY_IP&scope=user-read-private%20user-read-email&response_type=token
    String contentURL = "https://accounts.spotify.com/authorize";
    contentURL += "?client_id=";
    contentURL += SECRET_CLIENT_ID;
    contentURL += "&redirect_uri=http:%2F%2F" + strIP + "%2Fcallback";
    contentURL += "&scope=user-read-playback-state%20user-modify-playback-state";
    contentURL += "&response_type=token";

    Serial.print("URL: ");
    Serial.println(contentURL);

    //EEC_LOW alphanumeric limits quick lookup (only up to 10, but you can add up to 40
    //	REF: https://github.com/ricmoo/QRCode
    const int qrScalingAlphanumeric[] = {25, 47, 77, 114, 154, 195, 224, 279, 335, 395};
    int qrSize;

    for (qrSize = 0; qrSize < 10; ++qrSize)
    {
        //If the scale fits our content size, break out of the for loop
        if (qrScalingAlphanumeric[qrSize] > contentURL.length()) break;
    }

    ++qrSize; //non-zero offset
    ++qrSize; //fix for broken garb
    qrSize = 11;

    Serial.print("URL length: ");
    Serial.println(contentURL.length());
    Serial.print("Creating QR, version ");
    Serial.println(qrSize);

    QRCode qrcode;
    uint8_t qrcodeBytes[qrcode_getBufferSize(qrSize)];
    qrcode_initText(&qrcode, qrcodeBytes, qrSize, ECC_LOW, contentURL.c_str());

    Serial.print("QR Created, version ");
    Serial.println(qrSize);


    //Set up draw variables
    const int iFontPixels = 10;
    String strText = "SPOTIFY";
    String strText2 = "LOGIN";
    int iFontSize = 2;
    int iPitch = qrcode.size;
    int iScale = 2;
    int iOffsetX = (m_pTFT->width() / 2) - ((iPitch * iScale) / 2);
    int iOffsetY = (m_pTFT->height() / 2) - ((iPitch * iScale) / 2);
    int iTextOffX = (m_pTFT->width() / 2) - (strText.length() * (iFontSize * 6)) / 2;
    int iTextOffX2 = (m_pTFT->width() / 2) - (strText2.length() * (iFontSize * 6)) / 2;
    int iTextOffY = iOffsetY - (iFontSize * iFontPixels);
    int iTextOffY2 = (m_pTFT->height() / 2) + ((iPitch * iScale) / 2);

    //Draw text
    m_pTFT->setCursor(iTextOffX, iTextOffY);
    m_pTFT->setTextSize(iFontSize);
    m_pTFT->setTextColor(ILI9341_WHITE);
    m_pTFT->println(strText);
    m_pTFT->setCursor(iTextOffX2, iTextOffY2 + 7);
    m_pTFT->println(strText2);

    //Draw QRCode
    m_pTFT->fillRect(iOffsetX, iOffsetY, iPitch * iScale, iPitch * iScale, ILI9341_BLACK); //Speed up QR drawing by one big black fill before drawing modules
    for (uint8_t y = 0; y < iPitch; ++y)
    {
        for (uint8_t x = 0; x < iPitch; ++x)
        {
            if (qrcode_getModule(&qrcode, x, y)) m_pTFT->fillRect(iOffsetX + (x * iScale), iOffsetY + (y * iScale), iScale, iScale, ILI9341_WHITE);
        }
    }

    //Start web server on ip
    //Listen for spotify callback
    //Get access_token for API
    //Move to Loop();
    WiFiServer server(80);
    server.begin();

    //Short loop
    String spotify_token = "";
    bool bCallback = false;
    while (true)
    {
        WiFiClient client = server.available(); //Listen for incoming clients

        if (client)
        {
            Serial.print("Incoming connection ... ");
            String currentLine = "";
            while (client.connected())
            {
                if (client.available()) //if there is data to be read
                {
                    char c = client.read();
                    if (c == '\n') { // if the byte is a newline character
                        // if the current line is blank, you got two newline characters in a row.
                        // that's the end of the client HTTP request, so send a response:
                        if (currentLine.length() == 0) {
                            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
                            // and a content-type so the client knows what's coming, then a blank line:
                            client.println("HTTP/1.1 200 OK");
                            client.println("Content-type:text/html");
                            client.println();

                            // the content of the HTTP response follows the header:
                            if (bCallback)
                            {
                                Serial.println("Sending callback page");

                                //HACK: force the client to give us a readable token by stripping the hash
                                bCallback = false;
                                client.print("<html><head><title>Test</title></head><body>");
                                client.print("<script type=\"text/javascript\">");
                                client.print("if(window.location.hash){");
                                client.print("var hash = window.location.hash;");
                                client.print("hash = hash.substring(hash.indexOf(\"=\") + 1, hash.indexOf(\"&\"));");
                                client.print("window.location.href = \"/bind?token=\" + hash;}");
                                client.print("</script></body></html>");
                            }
                            else
                            {
                                Serial.println("Sending success page");

                                //We're done
                                client.print("You can close this window now");
                                client.print("<script>window.open('spotify://','_self')</script>");
                            }

                            // The HTTP response ends with another blank line:
                            client.println();
                            // break out of the while loop:
                            break;
                        } else {    // if you got a newline, then clear currentLine:
                            currentLine = "";
                        }
                    } else if (c != '\r') {  // if you got anything else but a carriage return character,
                        currentLine += c;      // add it to the end of the currentLine
                    }

                    // Check to see if the client request was "GET /H" or "GET /L":
                    if (currentLine.endsWith("GET /callback")) {
                        bCallback = true;
                    }

                    if (currentLine.endsWith("GET /bind")) {
                        currentLine = ""; //clear line
                        Serial.print("GET /bind");

                        while (c != '\r' && c != '\n' && c != ' ')
                        {
                            c = client.read();
                            currentLine += c;
                            Serial.write(c);
                        }

                        spotify_token = currentLine.substring(currentLine.indexOf('=') + 1, currentLine.indexOf('&'));
                    }
                }
            }

            client.stop();
            Serial.println("Client disconnected");

            if (spotify_token != "") break;
        }
    }

    //Refresh screen
    m_pTFT->fillScreen(ILI9341_BLACK);

    // assign the OAuth credentials
    m_pOAuthClient->setCredentials("", "", ("Bearer " + spotify_token).c_str(), "");

    //Return true if we got our token
    return (spotify_token != "");
}

bool
CSpotify::SyncPlayerInfo()
{
    Serial.print("SyncPlayerInfo() ... ");

    m_pOAuthClient->get(g_ktSpotifyApiActions[(int)EApiEndpoints::GET_PLAYBACK_STATE].strURL);

    int statusCode = m_pOAuthClient->responseStatusCode();
    String response = m_pOAuthClient->responseBody();

    if (statusCode != 200 && statusCode != 204) { //200 is playing, 204 is nothing playing
        // An error occurred
        m_pTFT->fillScreen(ILI9341_BLACK);
        Serial.println(statusCode);
        Serial.println(response);
        m_pTFT->println(statusCode);
        m_pTFT->println(response);
        return(false);
    }
    else
    {
        Serial.print("Processing Json ... ");

        StaticJsonDocument<4096> jsonDoc;
        DeserializationError err = deserializeJson(jsonDoc, response);

        //Debug error
        //if (err == DeserializationError::Ok) Serial.println("Response ok");
        if (err == DeserializationError::EmptyInput) Serial.println("Response empty");
        else if (err == DeserializationError::IncompleteInput) Serial.println("Response malformed");
        else if (err == DeserializationError::InvalidInput) Serial.println("Response not supported");
        else if (err == DeserializationError::NoMemory) Serial.println("Response too large");
        else if (err == DeserializationError::TooDeep) Serial.println("Response too deep");

        //Quick error breakout
        if (err) return (false);

        //Update the player state
        jsonDoc["timestamp"] = jsonDoc["timestamp"].as<String>().substring(0, 10); //Trim to match our format (seconds, not milliseconds)

        m_tPlayerState.strContentType 		= jsonDoc["currently_playing_type"].as<String>();	//Type of content, such as Track or Podcast
        m_tPlayerState.bShuffle 			= jsonDoc["shuffle_state"];				//Music is shuffled?
        m_tPlayerState.bPlaying 			= jsonDoc["is_playing"];				//True if music is playing
        m_tPlayerState.ulTimestampServer	= jsonDoc["timestamp"].as<unsigned long long>();					//Last update
        m_tPlayerState.ulTimestampLocal		= CClock::GetUnixTime();					//Last update
        m_tPlayerState.iProgress 			= jsonDoc["progress_ms"];				//Track progress

        //If the server timestamp is incorrect, use local in lieu
        if (m_tPlayerState.ulTimestampServer == 0)m_tPlayerState.ulTimestampServer = m_tPlayerState.ulTimestampLocal;

        //Context provided by the player, usually a playlist, album, artist etc.
        m_tPlayerState.tContext.strType = jsonDoc["context"]["type"].as<String>(); //Playlist, Artist, Album, Show etc.
        m_tPlayerState.tContext.strHREF = jsonDoc["context"]["href"].as<String>(); //Web API endpoint for track details

        //Current device of player
        m_tPlayerState.tDevice.strID 			= jsonDoc["device"]["id"].as<String>();					//Device ID
        m_tPlayerState.tDevice.strName 			= jsonDoc["device"]["name"].as<String>();				//Name of device
        m_tPlayerState.tDevice.strType 			= jsonDoc["device"]["type"].as<String>();				//Type of device
        m_tPlayerState.tDevice.iVolume 			= jsonDoc["device"]["volume_percent"]; 		//Percent (0..100)
        m_tPlayerState.tDevice.bIsActive 		= jsonDoc["device"]["is_active"];			//Currently active device?
        m_tPlayerState.tDevice.bPrivateSession 	= jsonDoc["device"]["is_private_session"];	//Private session (no history)
        m_tPlayerState.tDevice.bRestricted 		= jsonDoc["device"]["is_restricted"];		//If true, we can control this device

        //Information on the current item playing
        m_tPlayerState.tItem.strID 				= jsonDoc["item"]["id"].as<String>();			//ID of item
        m_tPlayerState.tItem.strName 			= jsonDoc["item"]["name"].as<String>();			//Name of item
        m_tPlayerState.tItem.strType 			= jsonDoc["item"]["type"].as<String>();			//Type of item (eg. track)
        m_tPlayerState.tItem.strHREF 			= jsonDoc["item"]["href"].as<String>();			//Web API endpoint for item details
        m_tPlayerState.tItem.iTrackNumber 		= jsonDoc["item"]["track_number"];	//Track number of item
        m_tPlayerState.tItem.iDiscNumber 		= jsonDoc["item"]["disc_number"];	//Disc number of item
        m_tPlayerState.tItem.iDuration 			= jsonDoc["item"]["duration_ms"];	//Duration of the itement
        m_tPlayerState.tItem.iPopularity 		= jsonDoc["item"]["popularity"];	//Popularity of content (0..100), higher being more popular
        m_tPlayerState.tItem.bExplicit 			= jsonDoc["item"]["explicit"];		//Contains explicit language
        m_tPlayerState.tItem.bIsLocal 			= jsonDoc["item"]["is_local"];		//Content is local to device (not on spotify)

        //Get the following array items for iteration
        JsonArray jsonSongArtists 	= jsonDoc["item"]["artists"].as<JsonArray>();
        JsonArray jsonAlbumArtists 	= jsonDoc["item"]["album"]["artists"].as<JsonArray>();
        JsonArray jsonAlbumImages 	= jsonDoc["item"]["album"]["images"].as<JsonArray>();

        //Get the number of artists for the item and album
        m_tPlayerState.tItem.iNumOfArtistsItem 	= jsonSongArtists.size();		//Number of artists on this item, and what you will find in tItem.tArtists
        m_tPlayerState.tItem.iNumOfArtistsAlbum = jsonAlbumArtists.size();		//Number of artists on this album, and what you will find in tItem.tAlbum.tArtists
        m_tPlayerState.tItem.iNumOfAlbumImages = jsonAlbumImages.size();		//Number of images for this album, and what you will find in tItem.tAlbum.tImages

        //Artist information
        int i = 0;
        for (JsonObject jsonArtist : jsonSongArtists)
        {
            m_tPlayerState.tItem.tArtists[i].strID 		= jsonArtist["id"].as<String>();	//ID of artist
            m_tPlayerState.tItem.tArtists[i].strName 	= jsonArtist["name"].as<String>();	//Name of artist
            m_tPlayerState.tItem.tArtists[i].strHREF 	= jsonArtist["href"].as<String>();	//Web API endpoint for artist details
            ++i;
        }

        //Album information
        m_tPlayerState.tItem.tAlbum.strID 			= jsonDoc["item"]["album"]["id"].as<String>();			//ID of album
        m_tPlayerState.tItem.tAlbum.strName 		= jsonDoc["item"]["album"]["name"].as<String>();			//Name of album
        m_tPlayerState.tItem.tAlbum.strReleaseDate 	= jsonDoc["item"]["album"]["release_date"].as<String>();	//Release date of album
        m_tPlayerState.tItem.tAlbum.strType 		= jsonDoc["item"]["album"]["type"].as<String>();			//Type of album, such as a single
        m_tPlayerState.tItem.tAlbum.strHREF 		= jsonDoc["item"]["album"]["href"].as<String>();			//Web API endpoint for album details
        m_tPlayerState.tItem.tAlbum.iTotalTracks 	= jsonDoc["item"]["album"]["total_tracks"];	//Total number of tracks in this album

        //Album artists
        i = 0;
        for (JsonObject jsonArtist : jsonAlbumArtists)
        {
            //Artists signed on this album, probably no more than 10 as well, ... maybe
            m_tPlayerState.tItem.tAlbum.tArtists[i].strID 	= jsonArtist["id"].as<String>();	//ID of artist
            m_tPlayerState.tItem.tAlbum.tArtists[i].strName = jsonArtist["name"].as<String>();	//Name of artist
            m_tPlayerState.tItem.tAlbum.tArtists[i].strHREF = jsonArtist["href"].as<String>();	//Web API endpoint for artist details
            ++i;
        }

        //Album images
        i = 0;
        for (JsonObject jsonImage : jsonAlbumImages)
        {
            //One would assume that height == width for spotify but we've all been wrong before
            m_tPlayerState.tItem.tAlbum.tImages[i].iHeight 	= jsonImage["height"]; 	//Height of image
            m_tPlayerState.tItem.tAlbum.tImages[i].iWidth 	= jsonImage["width"]; 	//Width of image
            m_tPlayerState.tItem.tAlbum.tImages[i].strURL 	= jsonImage["url"].as<String>(); 		//URL of image
            ++i;
        }

        //Actions (dis)allowed by the API
        m_tPlayerState.tActions.bInterruptPlayback 		= !jsonDoc["actions"]["disallows"].containsKey("interrupting_playback");	//Allow interruption of playback
        m_tPlayerState.tActions.bPause 					= !jsonDoc["actions"]["disallows"].containsKey("pausing");					//Allow pausing	(false if already paused)
        m_tPlayerState.tActions.bResume 				= !jsonDoc["actions"]["disallows"].containsKey("resuming");				//Allow resuming (false if already playing)
        m_tPlayerState.tActions.bSeek 					= !jsonDoc["actions"]["disallows"].containsKey("seeking");					//Allow seeking
        m_tPlayerState.tActions.bSkipNext 				= !jsonDoc["actions"]["disallows"].containsKey("skipping_next");			//Allow skipping to next song
        m_tPlayerState.tActions.bSkipPrev 				= !jsonDoc["actions"]["disallows"].containsKey("skipping_prev");			//Allow skipping to previous song (false if no previous song, you may still seek 0)
        m_tPlayerState.tActions.bToggleRepeatContext 	= !jsonDoc["actions"]["disallows"].containsKey("toggling_repeat_context");	//Allow repeat context
        m_tPlayerState.tActions.bToggleRepeatTrack 		= !jsonDoc["actions"]["disallows"].containsKey("toggling_shuffle");		//Allow repeat of this track
        m_tPlayerState.tActions.bToggleShuffle 			= !jsonDoc["actions"]["disallows"].containsKey("toggling_repeat_track");	//Allow toggling shuffle
        m_tPlayerState.tActions.bTransferPlayback 		= !jsonDoc["actions"]["disallows"].containsKey("transferring_playback");	//Allow transferral of playback to another device

        //Repeat state
        String strRepeatState = jsonDoc["repeat_state"];
        if (strRepeatState == "off") m_tPlayerState.eRepeatState = TPlayer::ERepeatState::off;
        else if (strRepeatState == "track") m_tPlayerState.eRepeatState = TPlayer::ERepeatState::track;
        else if (strRepeatState == "context") m_tPlayerState.eRepeatState = TPlayer::ERepeatState::context;

        Serial.println("Success");
        return(true);
    }

    return (false);
}

bool
CSpotify::LoadAlbumArt(int _x, int _y, int _size)
{
    bool bSuccess = false;

    String url = m_tPlayerState.tItem.tAlbum.tImages[_size].strURL;
    String host = url.substring(0, url.lastIndexOf('/') - 1);
    host = host.substring(0, host.lastIndexOf('/'));
    host = host.substring(host.lastIndexOf(':') + 3);
    String imgid = "/image" + url.substring(url.lastIndexOf('/'));
    
    Serial.print("LoadAlbumArt() -> ");
    Serial.print(url);
    Serial.print(" ... ");

    WiFiClient wifi;
    HttpClient client = HttpClient(wifi, host, 80); //TODO: we need to make this part of the program as we consume sockets too quickly and then this will never work
    client.get(imgid);

    int statusCode = client.responseStatusCode();
    String response = client.responseBody();

    if (statusCode != 200) {
        // An error occurred
        Serial.println(statusCode);
        client.stop();
        wifi.stop();
        return(false);
    }
    else
    {
        m_pTFT->startWrite(); // Not sharing TFT bus on PyPortal, just CS once and leave it

        JPEGDEC jpeg;
        if (jpeg.openRAM((uint8_t *)response.c_str(), response.length(), CSpotify::DrawAlbumArt))
        {
            jpeg.decode(_x, _y, 0);

            switch (jpeg.getLastError())
            {
                case JPEG_INVALID_PARAMETER:
                    Serial.println("JPEG_INVALID_PARAMETER");
                    break;
                case JPEG_DECODE_ERROR:
                    Serial.println("JPEG_DECODE_ERROR");
                    break;
                case JPEG_UNSUPPORTED_FEATURE:
                    Serial.println("JPEG_UNSUPPORTED_FEATURE");
                    break;
                case JPEG_INVALID_FILE:
                    Serial.println("JPEG_INVALID_FILE");
                    break;
                case JPEG_SUCCESS:
                    Serial.println("Success");
                    bSuccess = true;
                    break;
                default:
                    break;
            }

            jpeg.close();
        }
        else
        {
            Serial.println("Failed to open image successfully");
        }

        m_pTFT->endWrite();
    }

    client.stop();
    wifi.stop();

    return (bSuccess);
}

int
CSpotify::DrawAlbumArt(JPEGDRAW* pDraw)
{
    if (m_pTFT && pDraw)
    {
        m_pTFT->dmaWait(); // Wait for prior writePixels() to finish
        m_pTFT->setAddrWindow(pDraw->x, pDraw->y, pDraw->iWidth, pDraw->iHeight);
        m_pTFT->writePixels(pDraw->pPixels, pDraw->iWidth * pDraw->iHeight, true, false); // Use DMA, big-endian
    }
    else
    {
        return (0);
    }

    // Return 1 to decode next block
    return (1);
}
