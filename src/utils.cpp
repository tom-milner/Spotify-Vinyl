//
// Created by Tom Milner on 08/07/2020.
//

#include <Arduino.h>

class Utils {

    static void dumpByteArray(byte *buffer, byte bufferSize) {
        Serial.println();
        for (byte i = 0; i < bufferSize; i++) {
            Serial.print(buffer[i] < 0x10 ? " 0" : " ");
            Serial.print(buffer[i], HEX);
        }
        Serial.println();
    }

};