#include <sys/cdefs.h>


// WIFI!!!

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>   // Include the WebServer library
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <EEPROM.h>
#include <MFRC522.h>
#include <base64.h>

#include "credentials.h"

#define RST_PIN    0
#define SS_PIN    15

ESP8266WebServer server(80);
HTTPClient http;
MFRC522 mfrc522(SS_PIN, RST_PIN); // Create MFRC522 instance


enum AuthGrantType {
    AUTHORIZATION_CODE,
    REFRESH_TOKEN
};

struct HttpResponse {
    String response;
    int code;
};


// Function prototypes for HTTP handlers.
void handleAuth();

int refreshAccessToken();

void handleNotFound();

void dumpByteArray(byte *buffer, byte bufferSize);

void saveRefreshToken();

void dumpEEPROM();

void getAccessToken();

HttpResponse makeAuthRequest(AuthGrantType grantType, String code = "");

String getIdFromNTAG();

String readRefreshToken();


void playSpotifyResource(String id);

char spotifyConnected = 0;
String lastID = "", currID = "";
String spotifyAccess, spotifyRefresh;


void setup(void) {
    Serial.begin(115200);         // Start the Serial communication to send messages to the computer
    EEPROM.begin(512);
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
    spotifyRefresh = readRefreshToken();

    // If there is no refresh token, setup the webserver for authentication.
    if (spotifyRefresh.equals("")) {

        Serial.println("No refresh found.");

        // Setup the server for authentication.
        server.on("/auth", handleAuth);
        server.onNotFound(handleNotFound);

        server.begin();

        // Start the server
        Serial.println("HTTP server started");

    } else {
        Serial.print("Refresh found: ");
        Serial.println(spotifyRefresh);
        spotifyConnected = 1;

        // Get new access token.
        refreshAccessToken();
    }

    SPI.begin();      // Init SPI bus
    mfrc522.PCD_Init();   // Init MFRC522


}

void loop(void) {

    if (!spotifyConnected) {
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
        playSpotifyResource(currID);
    }

    lastID = currID;

    delay(500);


}

int refreshAccessToken() {

    const HttpResponse response = makeAuthRequest(REFRESH_TOKEN);
    Serial.println(response.response);

    StaticJsonDocument<JSON_OBJECT_SIZE(4) + 350> doc;
    String json = response.response;
    Serial.println();
    Serial.println(json);
    Serial.println(json.charAt(0));
    Serial.println();

    DeserializationError error = deserializeJson(doc, json);
    if (error) {
        Serial.println(error.c_str());
        return error.code();
    }
    spotifyAccess = doc["access_token"].as<char *>();
    Serial.println("Access Token acquired:");
    Serial.println(spotifyAccess);

    return 0;


}


void playSpotifyResource(String id) {
    Serial.println(id);
    Serial.println(spotifyAccess);
    String url = SPOTIFY_BASE_API;
    url += "/v1/me/player/play";


    http.begin(url, SPOTIFY_CERT_FINGERPRINT);

    http.addHeader("Authorization", "Bearer " + spotifyAccess);

    Serial.println(http.header("Authorization"));


    const int capacity = JSON_OBJECT_SIZE(1);
    StaticJsonDocument<capacity> doc;
    doc["context_uri"] = id.c_str();
    String payload;
    serializeJson(doc, payload);


    Serial.println(payload);
    int responseCode = http.PUT(payload);
    Serial.println(responseCode);
    Serial.println(http.getString());

    http.end();

}


// Handle authenticating the user to spotify.
void handleAuth() {
    Serial.println("Auth hit!!");

    // Get the auth code from the request params.
    Serial.println(server.arg("code"));
    String code = server.arg("code");


    const HttpResponse response = makeAuthRequest(AUTHORIZATION_CODE, code);
    Serial.println(response.response);

    if (response.code != 200) {
        return server.send(500, "text/plain", "There was an error. Try again!");
    }

    // Handle the request response.


    // Parse the json response.
    StaticJsonDocument<JSON_OBJECT_SIZE(5) + 480> doc;
    DeserializationError error = deserializeJson(doc, response.response);
    if (error) {
        Serial.println("Deserialization failed.");
    } else {
        spotifyAccess = doc["access_token"].as<char *>();
        spotifyRefresh = doc["refresh_token"].as<char *>();
        saveRefreshToken();

        Serial.println("Saved refresh token: ");
        Serial.println(spotifyRefresh);

        Serial.println("Read refresh token: ");
        Serial.println(readRefreshToken());

        server.send(200, "text/plain", "Success!");
        server.stop(); ///< No longer need the server.
        spotifyConnected = 1;

    }

}

void handleNotFound() {
    server.send(404, "text/plain", "404: Not found"); // Send HTTP status 404 (Not Found) when there's no handler for the URI in the request
}

void saveRefreshToken() {

    int addr = 0;

    // Write the token flag, indicating that the EEPROM contains the token.
    for (int i = 0; i < 4; i++) {
        EEPROM.write(addr++, 15); // 00001111
    }


    // Write the token length to memory.
    uint8_t tokenLength = (uint8_t) spotifyRefresh.length();
    Serial.println();
    Serial.print("Token Length: ");
    Serial.println(tokenLength);
    EEPROM.write(addr++, tokenLength);

    // Write the token to memory.
    for (int i = 0; i < tokenLength; i++) {
        EEPROM.write(addr++, spotifyRefresh.charAt(i));
    }

    EEPROM.commit();
}

// Authorise the user using spotify's accounts API.
HttpResponse makeAuthRequest(AuthGrantType grantType, String code) {
    http.begin(SPOTIFY_ACCOUNTS_API + "/token", SPOTIFY_CERT_FINGERPRINT);
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");
    String basicAuth = base64::encode(SPOTIFY_CLIENT_ID + ":" + SPOTIFY_CLIENT_SECRET);
    http.setAuthorization(basicAuth.c_str());
    String payload = "grant_type=";

    switch (grantType) {
        case REFRESH_TOKEN:
            payload += "refresh_token&refresh_token=";
            payload += spotifyRefresh.c_str();
            break;
        case AUTHORIZATION_CODE:
            payload += "authorization_code&redirect_uri=http://192.168.1.78/auth&code=";
            payload += code;
            break;
    }

    HttpResponse res;
    res.code = http.POST(payload);
    res.response = http.getString();

    http.end();
    return res;

}


String readRefreshToken() {
    int addr = 0;

    // Check the refresh token is present - first 4 bytes must all be 15.
    for (int i = 0; i < 4; i++) {
        if (EEPROM.read(addr++) != 15) {
            return "";
        }

    }
    Serial.println();

    // Read the token length.
    uint8_t tokenLength = EEPROM.read(addr++);


    // Read the refresh token.
    String token = "";
    for (int i = 0; i < tokenLength; i++) {
        token.concat((char) EEPROM.read(addr++));
    }

    return token;
}

void dumpEEPROM() {
    Serial.println();
    for (int i = 0; i < 512; i++) {
        Serial.print((char) EEPROM.read(i));
        Serial.print(" ");
    }
    Serial.println();

}

String getIdFromNTAG() {

    byte minBufSize = 18; // imposed by library :(
    byte pageIdx = 0x04;


    // Read length
    byte tempBuffer[minBufSize];
    mfrc522.MIFARE_Read(pageIdx++, tempBuffer, &minBufSize);
    int idLength = tempBuffer[0];
//    Serial.println(idLength);

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
    return id;
}

void dumpByteArray(byte *buffer, byte bufferSize) {
    Serial.println();
    for (byte i = 0; i < bufferSize; i++) {
        Serial.print(buffer[i] < 0x10 ? " 0" : " ");
        Serial.print(buffer[i], HEX);
    }
    Serial.println();
}



