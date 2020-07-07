#include <sys/cdefs.h>


// WIFI!!!

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>   // Include the WebServer library
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <MFRC522.h>
#include <base64.h>

#include "credentials.h"
#include "main.h"
#include "spotifyapi.h"
#include "structures.h"
#include "localStorage.h"

#define RST_PIN    0
#define SS_PIN    15

ESP8266WebServer server(80);
//HTTPClient http;
MFRC522 mfrc522(SS_PIN, RST_PIN); // Create MFRC522 instance

SpotifyAPI spotifyApi;
LocalStorage localStorage;

String lastID = "", currID = "";


// ============================================================
//                          Main
// ============================================================
void setup(void) {
    Serial.begin(115200);         // Start the Serial communication to send messages to the computer
    delay(10);
    Serial.println('\n');


    Serial.println("Connecting to ");
    Serial.print(WIFI_SSID);
    Serial.println();

    // Connect to the wifi.
    WiFi.hostname("eps8266");
    WiFi.begin(WIFI_SSID, WIFI_PASS);

    // Wait for the Wi-Fi to connect
    int i = 0;
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.print(++i);
        Serial.print(' ');
    }

    Serial.println('\n');
    Serial.print("Connected to ");
    Serial.println(WiFi.SSID());              // Tell us what network we're connected to
    Serial.print("IP address:\t");
    Serial.println(WiFi.localIP());           // Send the IP address of the ESP8266 to the computer



    // Get the past refresh token from EEPROM.
    spotifyApi.setRefreshToken(localStorage.readRefreshToken());

    // If there is no refresh token, setup the webserver for authentication.
    if (spotifyApi.getRefreshToken().equals("")) {

        Serial.println("No refresh found.");

        // Setup the server for authentication.
        server.on("/auth", handleAuth);
        server.onNotFound(handleNotFound);

        server.begin();

        // Start the server
        Serial.println("HTTP server started");

    } else {
        Serial.print("Refresh found: ");
        Serial.println(spotifyApi.getRefreshToken());
        spotifyApi.isConnected = 1;

        // Get new access token.
        spotifyApi.refreshAccessToken();
    }

    SPI.begin();      // Init SPI bus
    mfrc522.PCD_Init();   // Init MFRC522


}

void loop(void) {

    if (!spotifyApi.isConnected) {
        server.handleClient();  // Listen for HTTP requests from clients
        return;
    }

    // Look for new cards
    if (!mfrc522.PICC_IsNewCardPresent()) {
        return;
    }
    if (!mfrc522.PICC_ReadCardSerial()) {
        return;
    }


    currID = getIdFromNTAG();
//    Serial.println(currID);

    if (!currID.equals(lastID)) {
        spotifyApi.playSpotifyResource(currID);
    }

    lastID = currID;

    delay(500);


}




// ============================================================
//                          Webserver
// ============================================================
void handleAuth() {
    Serial.println("Auth hit!!");

    // Get the auth code from the request params.
    Serial.println(server.arg("code"));
    String code = server.arg("code");


    const HttpResponse response = spotifyApi.makeAuthRequest(AUTHORIZATION_CODE, code);
    Serial.println(response.response);

    if (response.code != 200) {
        return server.send(500, "text/plain", "There was an error. Try again!");
    }

    localStorage.saveRefreshToken(spotifyApi.getRefreshToken());

    server.send(200, "text/plain", "Success!");
    server.stop(); ///< No longer need the server.
    spotifyApi.isConnected = 1;


}

void handleNotFound() {
    server.send(404, "text/plain", "404: Not found"); // Send HTTP status 404 (Not Found) when there's no handler for the URI in the request
}




// ============================================================
//                          NFC Wrapper
// ============================================================


uint16_t generateURIChecksum(char *data) {

    uint16_t checksum = 0;
    while (*data != '\0') {
        checksum += *data++;
    }
    return checksum;

}

String getIdFromNTAG() {

    byte minBufSize = 18; // imposed by library :(
    byte pageIdx = 0x04;


    // Get the tag metadata (length, checksum)
    byte metaBuffer[minBufSize];
    mfrc522.MIFARE_Read(pageIdx++, metaBuffer, &minBufSize);
    int idLength = metaBuffer[0]; // The length of the Spotify URI will always be less than 255.

    // Read the checksum.
    uint16_t foundChecksum = metaBuffer[1] << 8 | metaBuffer[2]; // Checksum is stored across 2 bytes.

    // Calculate required buffer size.
    byte loops = ceil((idLength) / 16.0) + 1;
    byte bufferSize = (16 * loops) + 2;
    byte buffer[bufferSize];
    byte *bufferPtr = buffer;


    for (int i = 0; i < loops; i++) {
        mfrc522.MIFARE_Read(pageIdx, bufferPtr, &bufferSize);
        bufferPtr += 16;
        pageIdx += 4;
    }

    String id = "";
    for (int i = 0; i < idLength; i++) {
        id += (char) buffer[i];
    }

    // Calculate checksum
//    uint16_t calcChecksum = generateURIChecksum(id); // GOT HERE - refactor to remove strings!!!!

    return id;
}




// ============================================================
//                          Utility
// ============================================================




void dumpByteArray(byte *buffer, byte bufferSize) {
    Serial.println();
    for (byte i = 0; i < bufferSize; i++) {
        Serial.print(buffer[i] < 0x10 ? " 0" : " ");
        Serial.print(buffer[i], HEX);
    }
    Serial.println();
}

