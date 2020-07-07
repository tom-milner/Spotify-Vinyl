//
// Created by Tom Milner on 07/07/2020.
//

#ifndef RECORDPLAYER_MAIN_H
#define RECORDPLAYER_MAIN_H







// ============================================================
//                          Webserver
// =============================================================
void handleAuth();

void handleNotFound();





// ============================================================
//                          NFC Wrapper
// ============================================================

uint16_t generateURIChecksum(char * data);

String getIdFromNTAG();





// ============================================================
//                          Utility
// ============================================================
void dumpByteArray(byte *buffer, byte bufferSize);

void dumpInternalEEPROM();



#endif //RECORDPLAYER_MAIN_H
