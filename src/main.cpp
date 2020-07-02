


// WIFI!!!

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>   // Include the WebServer library
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <EEPROM.h>
#include <MFRC522.h>


#include "credentials.h"

#define RST_PIN    0
#define SS_PIN    15

ESP8266WebServer server(80);
HTTPClient http;
MFRC522 mfrc522(SS_PIN, RST_PIN); // Create MFRC522 instance


// Function prototypes for HTTP handlers.
void handleAuth();

void handleNotFound();

void dumpByteArray(byte *buffer, byte bufferSize);

void saveRefreshToken();

void dumpEEPROM();


String getIdFromNTAG(int length);

String readRefreshToken();


char spotifyConnected = 0;

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
    }

    SPI.begin();      // Init SPI bus
    mfrc522.PCD_Init();   // Init MFRC522


}

void loop(void) {

    if (!spotifyConnected) {
        server.handleClient();  // Listen for HTTP requests from clients
        return;
    }

    // Reset the loop if no new card present on the sensor/reader. This saves the entire process when idle.
    if (!mfrc522.PICC_IsNewCardPresent()) {
        return;
    }

    // Select one of the cards
    if (!mfrc522.PICC_ReadCardSerial()) {
        return;
    }

    Serial.println();

    String id = getIdFromNTAG(22);
    Serial.println(id);


    delay(3000);




}


// Handle authenticating the user to spotify.
void handleAuth() {
    Serial.println("Auth hit!!");

    // Get the auth code from the request params.
    Serial.println(server.arg("code"));
    String code = server.arg("code");


    // Build post payload.
    Serial.println();
    String payload = "code=" + code;
    payload += "&grant_type=authorization_code";
    payload += "&redirect_uri=http://192.168.1.78/auth";
    payload += "&client_secret=" + SPOTIFY_CLIENT_SECRET;
    payload += "&client_id=" + SPOTIFY_CLIENT_ID;
    Serial.println(payload);

    // Build the request
    http.begin(SPOTIFY_ACCOUNTS_API + "/api/token", SPOTIFY_CERT_FINGERPRINT);
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");

    // Make the request.
    int httpCode = http.POST(payload);
    const String response = http.getString();

    // Handle the request response.
    if (httpCode == 200) {
        server.send(200, "text/plain", "Success!");
        server.stop(); ///< No longer need the server.
        spotifyConnected = 1;

        // Parse the json response.
        StaticJsonDocument<510> doc;
        DeserializationError error = deserializeJson(doc, response.c_str());
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

        }


    } else {
        server.send(500, "text/plain", "There was an error. Try again!");
    }

    Serial.println(httpCode);
    Serial.println(response);

    // Close the connection.
    http.end();

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

String getIdFromNTAG(int length) {


    byte loops = ceil((length) / 16.0) + 1;
    byte bufferSize = (16 * loops) + 2;
    byte buffer[bufferSize];
    byte *bufferPtr = buffer;
    byte pageIdx = 0x04;

    for (int i = 0 ; i < loops; i++) {
        mfrc522.MIFARE_Read(pageIdx, bufferPtr, &bufferSize);
        bufferPtr += 16;
        pageIdx += 4;
    }


    return String( (char *) buffer);
}

void dumpByteArray(byte *buffer, byte bufferSize) {
    Serial.println();
    for (byte i = 0; i < bufferSize; i++) {
        Serial.print(buffer[i] < 0x10 ? " 0" : " ");
        Serial.print(buffer[i], HEX);
    }
    Serial.println();
}

