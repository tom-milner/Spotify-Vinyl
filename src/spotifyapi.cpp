//
// Created by Tom Milner on 07/07/2020.
//

#include <HardwareSerial.h>
#include <ArduinoJson.h>
#include <ESP8266HTTPClient.h>
#include <base64.h>


#include "spotifyapi.h"
#include "structures.h"
#include "credentials.h"


int SpotifyAPI::refreshAccessToken() {
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


// Authorise the user using spotify's accounts API.
HttpResponse SpotifyAPI::makeAuthRequest(AuthGrantType grantType, String code) {
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

    if (res.code == -1) return res;
    res.response = http.getString();

    http.end();

    // Deserialize the JSON.
    StaticJsonDocument<JSON_OBJECT_SIZE(5) + 480> doc; ///< Max size.

    DeserializationError error = deserializeJson(doc, res.response);
    if (error) {
        Serial.println(error.c_str());
        res.code = error.code();
        return res;
    }

    // Set the returned tokens.
    switch (grantType) {
        case AUTHORIZATION_CODE:
            spotifyRefresh = doc["refresh_token"].as<char *>();
            Serial.println("Saved refresh token: ");
            Serial.println(spotifyRefresh);


        case REFRESH_TOKEN:
            spotifyAccess = doc["access_token"].as<char *>(); ///< Access token is always returned.
            break;
    }

    Serial.println("Access Token acquired:");
    Serial.println(spotifyAccess);

    return res;

}


void SpotifyAPI::playSpotifyResource(String id) {
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

    if(responseCode != 204){
        refreshAccessToken();
        playSpotifyResource(id);
    }

}

String SpotifyAPI::getRefreshToken() {
    return spotifyRefresh;
}

void SpotifyAPI::setRefreshToken(String token) {
    spotifyRefresh = token;
}


