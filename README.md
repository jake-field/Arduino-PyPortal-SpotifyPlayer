# Arduino Spotify Player (Adafruit PyPortal)
This is a personal project to learn the intricacies of working with Arduino and a device such as the PyPortal.
The project allows you to log into your Spotify account via a QRcode (using implicit grant) for temporary access to display your Now Playing.

A basic play/pause button is displayed which will pause your currently active device.
The progress bar will scroll along in time with the song and the album art will be displayed behind the song information.

Unfortunately due to the singlethreaded nature, when the device checks for updates from the API, there will be a pause until the update request processes. I would like to explore options in the future, possibly just make my own async networking and furthermore replace the JPEG library included as it is not ideal, but it works for now.

Besides the networking and jpg library, I would probably make a quick UI library to tidy up a lot of the design and code, adding in the missing features like volume/next+prev/shuffle etc.

The project relies on:
	- arduino_secrets.h (Your WiFi details and your Spotify API client id/secret)
		- SECRET_SSID
		- SECRET_PASS
		- SECRET_CLIENT_ID
		- SECRET_CLIENT_SECRET
	- JPEGDEC (used for drawing the album art to the display)
	- QRCode
	- ArduinoJson
	- Arduino_OAuth
	- General Adafruit requirements for the PyPortal

# Login via QR code
![Alt text](/login.jpg?raw=true "Login preview")

# Player screen
![Alt text](/song.jpg?raw=true "Player preview")