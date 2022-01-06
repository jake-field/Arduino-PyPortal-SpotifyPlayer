#pragma once
#ifndef __SPOTIFY_TYPES_H__
#define __SPOTIFY_TYPES_H__

//Quick API defines
#define SPOTIFY_EXPECTED_ARTISTS 5			//Expected max API response for artists on any one item, could be more
#define SPOTIFY_EXPECTED_ALBUM_IMAGES 3 	//Expected API response for images, from what I understand are 3 varying resolutions of one image

struct TApiAction
{
    String strMethod;				//PUT, POST, GET etc.
    String strURL;					//URL of api action
};

enum class EApiEndpoints
{
    GET_PLAYBACK_STATE,
    TRANSFER_PLAYBACK,
    GET_AVAILABLE_DEVICES,
    GET_CURRENTLY_PLAYING,
    RESUME_PLAYBACK,
    PAUSE_PLAYBACK,
    SKIP_NEXT,
    SKIP_PREV,
    SEEK,
    SET_REPEAT_MODE,
    SET_PLAYBACK_VOLUME,
    TOGGLE_SHUFFLE,
    GET_RECENTLY_PLAYED,
    ADD_TO_QUEUE,

    EApiEndpointsMAX //Dummy
};

//Base URL for the Spotify API
const String g_strApiUrl = "api.spotify.com";

//REF: https://developer.spotify.com/documentation/web-api/reference/#/operations/get-information-about-the-users-current-playback
// This is a shortcut for API config that can be updated in a hurry
//
//NOTE: For some reason all of these endpoints use queries instead of body content... ????
//
const TApiAction g_ktSpotifyApiActions[(int)EApiEndpoints::EApiEndpointsMAX] = {
    {"GET", 	"/v1/me/player?market=NA"},			        //Get playback state, basically fills TPlayer (Using market to reduce json response size significantly (12k to 4k elements)
    {"PUT", 	"/v1/me/player"},					        //Transfer playback to another device, requires [[device_ids], play] in body
    {"GET", 	"/v1/me/player/devices"},			        //Gets the user's available devices
    {"GET", 	"/v1/me/player/currently-playing"},	        //Gets information on the current item being played
    {"PUT", 	"/v1/me/player/play"},				        //Resume playback or start new context, requires [context_uri, uris, offset, position] in body if new context
    {"PUT", 	"/v1/me/player/pause"},				        //Pauses the playback on user's account
    {"POST", 	"/v1/me/player/next"},				        //Skip to the next song
    {"POST", 	"/v1/me/player/previous"},			        //Skip to the previous song
    {"PUT", 	"/v1/me/player/seek?position_ms="},			//Seek to the given position, requires [position_ms]
    {"PUT", 	"/v1/me/player/repeat"},			        //Set repeat mode, requires [state] = [off, track, context]
    {"PUT", 	"/v1/me/player/volume?volume_percent="},    //Sets the volume of the current device, requires [volume_percent] = [0..100]
    {"PUT", 	"/v1/me/player/shuffle"},			        //Sets the shuffle value, requires [state] = [true, false]. SO MUCH FOR TOGGLE HUH?!
    {"GET", 	"/v1/me/player/recently-played"},	        //Gets the recently played tracks, see API ref for specifics as it's a big one
    {"POST", 	"/v1/me/player/queue"}
};			//Add an item to the end of the playback queue, requires [uri] = [spotify:track:...]

//Spotify permission scopes to be used
const String g_kstrSpotifyScopes[] = {
	//"ugc-image-upload",
	//"playlist-modify-private",
	//"playlist-read-private",
	//"playlist-modify-public",
	//"playlist-read-collaborative",
	//"user-read-private",
	//"user-read-email",
	"user-read-playback-state",
	"user-modify-playback-state",
	//"user-read-currently-playing",
	//"user-library-modify",
	//"user-library-read",
	//"user-read-playback-position",
	//"user-read-recently-played",
	//"user-top-read",
	//"app-remote-control",
	//"streaming",
	//"user-follow-modify",
	//"user-follow-read",
};

//Types
struct TPlayer
{
    //REF: https://developer.spotify.com/documentation/web-api/reference/#/operations/get-information-about-the-users-current-playback
    //	Not all of the api JSON results will be found here, I've purposefully omitted some of the values not relevant to me for memory usage

    String strContentType;				//Type of content, such as Track or Podcast
    bool bShuffle;						//Music is shuffled?
    bool bPlaying;						//True if music is playing
    unsigned long ulTimestampServer;	//Last update
    unsigned long ulTimestampLocal;		//Last update
    int iProgress;						//Track progress

    //Current device of player
    struct TDevice
    {
        String strID;				//Device ID
        String strName;				//Name of device
        String strType;				//Type of device
        int iVolume; 				//Percent (0..100)
        bool bIsActive;				//Currently active device?
        bool bPrivateSession;		//Private session (no history)
        bool bRestricted;			//If true, we can control this device
    } tDevice;

    //Context provided by the player, usually a playlist, album, artist etc.
    struct TContext
    {
        String strType;				//Playlist, Artist, Album, Show etc.
        String strHREF;				//Web API endpoint for track details
    } tContext;

    //Information on the current item playing
    struct TItem
    {
        String strID;				//ID of item
        String strName;				//Name of item
        String strType;				//Type of item (eg. track)
        String strHREF;				//Web API endpoint for item details
        int iTrackNumber;			//Track number of item
        int iDiscNumber;			//Disc number of item
        int iDuration;				//Duration of the itement
        int iPopularity;			//Popularity of content (0..100), higher being more popular
        int iNumOfArtistsItem;		//Number of artists on this item, and what you will find in TItem.tArtists
        int iNumOfArtistsAlbum;		//Number of artists on this album, and what you will find in TItem.tAlbum.tArtists
        int iNumOfAlbumImages;		//Number of images for this album, and what you will find in TItem.tAlbum.TImages
        bool bExplicit;				//Contains explicit language
        bool bIsLocal;				//Content is local to device (not on spotify)

        //Artist information
        struct TArtist
        {
            String strID;						//ID of artist
            String strName;						//Name of artist
            String strHREF;						//Web API endpoint for artist details
        } tArtists[SPOTIFY_EXPECTED_ARTISTS]; 	//Probably no more than 10 could be here, but I'm sure someone will find the exception

        //Album information
        struct TAlbum
        {
            String strID;								//ID of album
            String strName;								//Name of album
            String strReleaseDate;						//Release date of album
            String strType;								//Type of album, such as a single
            String strHREF;								//Web API endpoint for album details
            int iTotalTracks;							//Total number of tracks in this album
            TArtist tArtists[SPOTIFY_EXPECTED_ARTISTS];	//Artists signed on this album, probably no more than 10 as well, ... maybe

            //Album images
            struct TImages
            {
                //One would assume that height == width for spotify but we've all been wrong before
                int iHeight;							//Height of image
                int iWidth;								//Width of image
                String strURL;							//URL of image
            } tImages[SPOTIFY_EXPECTED_ALBUM_IMAGES];	//Images for this album, should be 3 max from what I've seen from API responses
        } tAlbum;
    } tItem;

    //Actions (dis)allowed by the API
    struct TActions
    {
        bool bInterruptPlayback;	//Allow interruption of playback
        bool bPause;				//Allow pausing	(false if already paused)
        bool bResume;				//Allow resuming (false if already playing)
        bool bSeek;					//Allow seeking
        bool bSkipNext;				//Allow skipping to next song
        bool bSkipPrev;				//Allow skipping to previous song (false if no previous song, you may still seek 0)
        bool bToggleRepeatContext;	//Allow repeat context
        bool bToggleRepeatTrack;	//Allow repeat of this track
        bool bToggleShuffle;		//Allow toggling shuffle
        bool bTransferPlayback;		//Allow transferral of playback to another device
    } tActions;

    //Repeat state
    enum ERepeatState
    {
        off,						//No repeat
        track,						//Repeat current track
        context						//Repeat current context
    } eRepeatState;
};

#endif //__SPOTIFY_TYPES_H__
