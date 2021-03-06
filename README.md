# Vinyl

Vinyl is designed to bring back old-school record players, and combine them with the rich and convenient features of an audio streaming service.

## Features
- Full OAuth2 support as well as token persistence and automatic token renewal.
- Can stream artists, albums, playlists and podcasts.



## Explanation
The device consists of a simple stand, containing an MFRC522 reader and an ESP8266.
The MFRC522 is used to read and write Spotify URIs from Play Cards (explained later).
The ESP8266 controls the device, and connects to the Spotify API.

A Play Card is really any physical object that can fit on the device's stand. In this instance, a Play Card consists of a print-out of an album/playlist/artist cover stuck on a cardboard square, with the intention to replicate a real vinyl album cover. NFC tags (the current implementation uses NTAG-15 stickers) are stuck onto the Play Cards. When a Play Card is placed on the stand, a Spotify URI is read off the NFC tag and the ESP8266 sends a request to the Spotify API to play the content corresponding to that URI.
By using the exact Spotify URI instead of the NTAG-15 UIDs, any Spotify content can be streamed as the device doesn't need to link UIDs to spotify URIs - the URIs can just be sent straight to Spotify.


A seperate program is used to write the URIs to the stickers. Each URI is stored in the following format:

      1 byte     2 bytes    <length> bytes  
     <length>   <checksum>  <Spotify URI>

The checksum is an unsigned 16-bit value made by summing the ASCII values of all the characters in the URI.

The project uses the OAuth 2.0 Authorization Code grant to access the user's Spotify account. It is implemented using a seperate NTAG-15 sticker on the corner of the stand (out of range of the MFRC522). When read by a smartphone, the sticker redirects the user to a Spotify authorization page to authorize their account for use with the device.
From there, the user is redirected to a "Success" page served by the device. The device then automatically handles gaining refresh tokens and refreshing access tokens. This authorization process only needs to be performed once, even in cases of power loss as the refresh token is stored in the devices EEPROM.


## NOTE
At the moment, this only works if the Spotify account is already _active_ on a device - e.g. currently playing or paused. This project cannot start spotify on a chosen device (yet).

The project is built using PlatformIO. To use it yourself, populate `credentials.h.b` with your wifi details and Spotify developer details and rename it to `credentials.h`
