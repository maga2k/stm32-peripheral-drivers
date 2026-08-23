//Author: Mattia Gamberini
//Version: 1.0 - 2026/08/22

#ifndef EXAMPLES_CAN_CANDB_H_
#define EXAMPLES_CAN_CANDB_H_

#include "canbus.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Add one enum entry + one row in CANDB_Frames[] (candb.c) per frame.
 * "id" is the BASE id, before any project-specific offset you may apply. */
typedef enum {
    CANDB_BMSC_Req = 0,
    CANDB_BMSC_ForceBalancing,
    CANDB_BoardInfo,
    CANDB_BootloaderCmd,
    CANDB_ExampleExtended,      /* example 29-bit frame */
    CANDB_FRAME_COUNT           /* keep last */
} CANDB_FrameId_t;

typedef struct {
    uint32_t     id;
    CAN_IdType_t idType;
    uint8_t      len;
    const char  *name;   /* for logs/debug */
} CANDB_Frame_t;

extern const CANDB_Frame_t CANDB_Frames[CANDB_FRAME_COUNT];

/* Linear search in CANDB_Frames. Returns CANDB_FRAME_COUNT if not found. */
CANDB_FrameId_t CANDB_Lookup(uint32_t id, CAN_IdType_t idType);

/* Fills id/idType/len from the database entry and zeroes the payload;
 * caller sets msg->data[] afterwards. Returns false if frameId is invalid. */
bool CANDB_CreateMessage(CANDB_FrameId_t frameId, CAN_Message_t *msg);

#ifdef __cplusplus
}
#endif

#endif /* EXAMPLES_CAN_CANDB_H_ */
