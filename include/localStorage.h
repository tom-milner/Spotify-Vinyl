//
// Created by Tom Milner on 07/07/2020.
//

#ifndef RECORDPLAYER_LOCALSTORAGE_H
#define RECORDPLAYER_LOCALSTORAGE_H


class LocalStorage {

public:

    LocalStorage();

    void saveRefreshToken(String refreshToken);

    String readRefreshToken();

    void dumpEEPROM();

};


#endif //RECORDPLAYER_LOCALSTORAGE_H
