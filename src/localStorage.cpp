//
// Created by Tom Milner on 07/07/2020.
//

#include <EEPROM.h>
//#include <WString.h>
#include <HardwareSerial.h>


#include "localStorage.h"

String LocalStorage::readRefreshToken() {
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


void LocalStorage::saveRefreshToken(String refreshToken) {

    int addr = 0;

    // Write the token flag, indicating that the EEPROM contains the token.
    for (int i = 0; i < 4; i++) {
        EEPROM.write(addr++, 15); // 00001111
    }


    // Write the token length to memory.
    uint8_t tokenLength = (uint8_t) refreshToken.length();
    Serial.println();
    Serial.print("Token Length: ");
    Serial.println(tokenLength);
    EEPROM.write(addr++, tokenLength);

    // Write the token to memory.
    for (int i = 0; i < tokenLength; i++) {
        EEPROM.write(addr++, refreshToken.charAt(i));
    }

    // A checksum isn't used here as the internal EEPROM is generally very reliable.

    EEPROM.commit();
}

LocalStorage::LocalStorage() {
    EEPROM.begin(512);
}


void LocalStorage::dumpEEPROM() {
    Serial.println();
    for (int i = 0; i < 512; i++) {
        Serial.print((char) EEPROM.read(i));
        Serial.print(" ");
    }
    Serial.println();

}