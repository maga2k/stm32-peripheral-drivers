//Author: Mattia Gamberini
//Version: 1.0 - 2026/08/22

#include "candb.h"
#include <string.h>

const CANDB_Frame_t CANDB_Frames[CANDB_FRAME_COUNT] = {
    [CANDB_BMSC_Req]            = { .id = 0x100,     .idType = CAN_ID_TYPE_STD, .len = 2, .name = "BMSC_Req" },
    [CANDB_BMSC_ForceBalancing] = { .id = 0x101,     .idType = CAN_ID_TYPE_STD, .len = 4, .name = "BMSC_ForceBalancing" },
    [CANDB_BoardInfo]           = { .id = 0x102,     .idType = CAN_ID_TYPE_STD, .len = 8, .name = "BoardInfo" },
    [CANDB_BootloaderCmd]       = { .id = 0x103,     .idType = CAN_ID_TYPE_STD, .len = 1, .name = "BootloaderCmd" },
    [CANDB_ExampleExtended]     = { .id = 0x1ABCDEF, .idType = CAN_ID_TYPE_EXT, .len = 8, .name = "ExampleExtended" },
};

CANDB_FrameId_t CANDB_Lookup(uint32_t id, CAN_IdType_t idType) {
    for (uint32_t i = 0; i < CANDB_FRAME_COUNT; i++) {
        if (CANDB_Frames[i].id == id && CANDB_Frames[i].idType == idType) {
            return (CANDB_FrameId_t)i;
        }
    }
    return CANDB_FRAME_COUNT;
}

bool CANDB_CreateMessage(CANDB_FrameId_t frameId, CAN_Message_t *msg) {
    if (msg == NULL || frameId >= CANDB_FRAME_COUNT) return false;

    memset(msg, 0, sizeof(*msg));
    msg->id     = CANDB_Frames[frameId].id;
    msg->idType = CANDB_Frames[frameId].idType;
    msg->len    = CANDB_Frames[frameId].len;

    return true;
}
