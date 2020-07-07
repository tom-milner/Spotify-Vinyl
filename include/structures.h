//
// Created by Tom Milner on 07/07/2020.
//

#ifndef RECORDPLAYER_STRUCTURES_H
#define RECORDPLAYER_STRUCTURES_H

enum AuthGrantType {
    AUTHORIZATION_CODE,
    REFRESH_TOKEN
};

struct HttpResponse {
    String response;
    int code;
};


#endif //RECORDPLAYER_STRUCTURES_H
