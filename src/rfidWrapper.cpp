//
// Created by Tom Milner on 08/07/2020.
//

#include "rfidWrapper.h"
#include <MFRC522.h>

uint16_t RFIDWrapper::generateURIChecksum(const char *data) {
    uint16_t checksum = 0;
    while (*data != '\0') {
        checksum += *data++;
    }
    return checksum;
}

String RFIDWrapper::getIdFromNTAG() {
    byte minBufSize = 18; // imposed by library :(
    byte pageIdx = 0x04;


    // Get the tag metadata (length, checksum)
    byte metaBuffer[minBufSize];
    mfrc522.MIFARE_Read(pageIdx++, metaBuffer, &minBufSize);
    int idLength = metaBuffer[0]; // The length of the Spotify URI will always be less than 255.

    Serial.println(metaBuffer[1], BIN);
    Serial.println(metaBuffer[2], BIN);
    Serial.println();

    // Read the checksum.
    uint16_t foundChecksum = metaBuffer[1] << 8 | metaBuffer[2]; // Checksum is stored across 2 bytes.
    Serial.println();
    Serial.print("Found checksum: ");
    Serial.println(foundChecksum, BIN);
    Serial.println(foundChecksum);

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
    uint16_t calcChecksum = generateURIChecksum(id.c_str());
    Serial.println();
    Serial.print("Calculated checksum: ");
    Serial.println(calcChecksum);

    if (calcChecksum != foundChecksum) {
        return "";
    }
    Serial.println(id);
    return id;
}
