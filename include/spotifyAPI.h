//
// Created by Tom Milner on 07/07/2020.
//

#ifndef RECORDPLAYER_SPOTIFYAPI_H
#define RECORDPLAYER_SPOTIFYAPI_H


// ============================================================
//                          Spotify Api
// ============================================================

#include "structures.h"


class SpotifyAPI {

public:

    int refreshAccessToken();

    HttpResponse makeAuthRequest(AuthGrantType grantType, String code = "");

    void playSpotifyResource(String id);

    void setRefreshToken(String token);

    String getRefreshToken();

    char isConnected = 0;

private:
    String spotifyAccess;
    String spotifyRefresh;
    HTTPClient http;

};





#endif //RECORDPLAYER_SPOTIFYAPI_H
