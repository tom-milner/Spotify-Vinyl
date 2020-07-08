//
// Created by Tom Milner on 08/07/2020.
//

#ifndef RECORDPLAYER_RFIDWRAPPER_H
#define RECORDPLAYER_RFIDWRAPPER_H


#include <MFRC522.h>

#define RST_PIN    0
#define SS_PIN    15


class RFIDWrapper {

public :

    RFIDWrapper(): mfrc522(SS_PIN, RST_PIN) {};

    uint16_t generateURIChecksum(char *data);

    String getIdFromNTAG();

private:
    MFRC522 mfrc522;


};


#endif //RECORDPLAYER_RFIDWRAPPER_H
