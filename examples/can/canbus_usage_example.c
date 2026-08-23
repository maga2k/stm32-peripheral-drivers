//Author: Mattia Gamberini
//Version: 1.1 - 2026/08/23

/**
 ******************************************************************************
 * @file    canbus_usage_example.c
 * @brief   Reference example wiring canbus.h + canbus_queue.h + candb.h
 *          together. Keep out of the main build (examples/ folder) and
 *          copy the relevant snippets into your own init code / task.
 ******************************************************************************
 */

#include "canbus.h"
#include "canbus_queue.h"
#include "candb.h"
#include <string.h>

extern CAN_HandleTypeDef hcan1;   /* adjust to your actual handle name */

static CANBus_Handle_t can1;
static CANBus_Queue_t  can1Queue;

/* --------------------------------------------------------------------
 * RX callback (ISR context, keep it short)
 * -------------------------------------------------------------------- */
static void CAN1_RxCallback(const CAN_Message_t *msg)
{
    CANDB_FrameId_t frameId = CANDB_Lookup(msg->id, msg->idType);

    switch (frameId) {
        case CANDB_BMSC_Req:
            /* msg->data[0], msg->data[1], ... */
            break;

        case CANDB_BMSC_ForceBalancing: {
            uint32_t cellsMask = 0;
            memcpy(&cellsMask, msg->data, sizeof(cellsMask));
            /* BMSTaskForceBalancing(cellsMask); */
            break;
        }

        case CANDB_BoardInfo:
            /* someone is requesting our firmware version */
            break;

        default:
            /* unknown frame: ignore, or log for debug */
            break;
    }
}

/* --------------------------------------------------------------------
 * Init: call once at startup, after MX_CAN1_Init()
 * -------------------------------------------------------------------- */
void CAN1_Init(void)
{
    static const uint32_t stdIds[] = {
        0x100, /* CANDB_BMSC_Req            */
        0x101, /* CANDB_BMSC_ForceBalancing */
        0x102, /* CANDB_BoardInfo           */
        0x103, /* CANDB_BootloaderCmd       */
    };
    static const uint32_t extIds[] = {
        0x1ABCDEF, /* CANDB_ExampleExtended */
    };

    CANBus_FilterConfig_t filterCfg = {
        .fifo                 = CANBUS_FIFO0,
        .bankStart            = 0,
        .bankCount            = 14,
        .slaveStartFilterBank = 14  /* single-CAN: all banks assigned here */
    };

    CANBus_Init(&can1, &hcan1);
    CANBus_RegisterRxCallback(&can1, CAN1_RxCallback);

    CANBus_Start(&can1, &filterCfg,
                 stdIds, sizeof(stdIds) / sizeof(stdIds[0]),
                 extIds, sizeof(extIds) / sizeof(extIds[0]));

    CANBus_Queue_Init(&can1Queue, &can1);
}

/* --------------------------------------------------------------------
 * Direct transmission: immediate, may return CANBUS_BUSY
 * -------------------------------------------------------------------- */
void CAN1_SendBoardInfo_Direct(uint8_t versionMajor, uint8_t versionMinor)
{
    CAN_Message_t msg;
    CANDB_CreateMessage(CANDB_BoardInfo, &msg);
    msg.data[0] = versionMajor;
    msg.data[1] = versionMinor;

    CANBus_Send(&can1, &msg);
}

/* --------------------------------------------------------------------
 * Queued transmission: asynchronous, preferable for periodic traffic
 * -------------------------------------------------------------------- */
void CAN1_SendBoardInfo_Queued(uint8_t versionMajor, uint8_t versionMinor)
{
    CAN_Message_t msg;
    CANDB_CreateMessage(CANDB_BoardInfo, &msg);
    msg.data[0] = versionMajor;
    msg.data[1] = versionMinor;

    CANBus_Queue_Send(&can1Queue, &msg, 0);
}

/* --------------------------------------------------------------------
 * Call from a periodic task, e.g. every 1ms
 * -------------------------------------------------------------------- */
void CAN_TxTask(void *argument)
{
    (void)argument;

    for (;;) {
        CANBus_Queue_Service(&can1Queue);
        osDelay(1);
    }
}
