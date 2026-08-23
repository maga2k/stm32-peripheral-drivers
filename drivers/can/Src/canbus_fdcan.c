//Author: Mattia Gamberini
//Version: 1.1 - 2026/08/23

/**
 ******************************************************************************
 * @file    canbus_fdcan.c
 * @brief   FDCAN backend for the generic canbus.h API (STM32G4/H7/U5 and
 *          other STM32 families using the FDCAN peripheral).
 *
 * NOTE: this file was written against the documented HAL_FDCAN_* API but,
 * unlike canbus_bxcan.c, it has NOT been compiled/tested against real HAL
 * headers in this session (that requires stm32g4xx_hal.h / stm32h7xx_hal.h
 * from the actual CubeMX-generated project). Build it in CubeIDE for your
 * target and fix any field/macro naming mismatch before relying on it.
 *
 * Uses one filter element per accepted ID (FDCAN_FILTER_TO_RXFIFO0), which
 * is simpler than bxCAN's list-mode packing but uses more filter RAM per ID
 * -- check your device's FDCAN_MESSAGE_RAM filter capacity if you have many
 * IDs to accept.
 ******************************************************************************
 */

#include "canbus.h"
#include <string.h>

/* Swap for stm32h7xx_hal.h / stm32u5xx_hal.h depending on your MCU family.
 * (Already pulled in transitively via canbus.h's stm32f1xx_hal.h in this
 * repo skeleton -- replace that include in canbus.h, or add your own guard,
 * so only the right family header is used per project.) */

#define CANBUS_MAX_INSTANCES 2

static CANBus_Handle_t *s_instances[CANBUS_MAX_INSTANCES] = {0};

static void RegisterInstance(CANBus_Handle_t *can) {
    for (int i = 0; i < CANBUS_MAX_INSTANCES; i++) {
        if (s_instances[i] == NULL) {
            s_instances[i] = can;
            return;
        }
    }
}

static CANBus_Handle_t *FindInstance(FDCAN_HandleTypeDef *hfdcan) {
    for (int i = 0; i < CANBUS_MAX_INSTANCES; i++) {
        if (s_instances[i] != NULL && s_instances[i]->halHandle == (void *)hfdcan) {
            return s_instances[i];
        }
    }
    return NULL;
}

static HAL_StatusTypeDef ConfigOneIdFilter(FDCAN_HandleTypeDef *hfdcan,
                                            uint32_t id, CAN_IdType_t idType,
                                            uint32_t filterIndex, CANBus_Fifo_t fifo)
{
    FDCAN_FilterTypeDef filter = {0};
    filter.IdType       = (idType == CAN_ID_TYPE_STD) ? FDCAN_STANDARD_ID : FDCAN_EXTENDED_ID;
    filter.FilterIndex  = filterIndex;
    filter.FilterType   = FDCAN_FILTER_DUAL;
    filter.FilterConfig = (fifo == CANBUS_FIFO0) ? FDCAN_FILTER_TO_RXFIFO0 : FDCAN_FILTER_TO_RXFIFO1;
    filter.FilterID1    = id;
    filter.FilterID2    = id; /* dual filter matching the same ID twice: simplest "single ID" filter */

    return HAL_FDCAN_ConfigFilter(hfdcan, &filter);
}

CANBus_Status_t CANBus_Init(CANBus_Handle_t *can, void *halHandle) {
    if (can == NULL || halHandle == NULL) return CANBUS_ERROR;

    can->halHandle  = halHandle;
    can->rxCallback = NULL;
    RegisterInstance(can);

    return CANBUS_OK;
}

CANBus_Status_t CANBus_Start(CANBus_Handle_t *can,
                              const CANBus_FilterConfig_t *filterCfg,
                              const uint32_t *acceptedStdIds, uint16_t stdLen,
                              const uint32_t *acceptedExtIds, uint16_t extLen)
{
    if (can == NULL || can->halHandle == NULL || filterCfg == NULL) return CANBUS_ERROR;

    FDCAN_HandleTypeDef *hfdcan = (FDCAN_HandleTypeDef *)can->halHandle;

    uint32_t idx = 0;
    for (uint16_t i = 0; i < stdLen; i++, idx++) {
        if (ConfigOneIdFilter(hfdcan, acceptedStdIds[i], CAN_ID_TYPE_STD, idx, filterCfg->fifo) != HAL_OK) {
            return CANBUS_ERROR;
        }
    }
    for (uint16_t i = 0; i < extLen; i++, idx++) {
        if (ConfigOneIdFilter(hfdcan, acceptedExtIds[i], CAN_ID_TYPE_EXT, idx, filterCfg->fifo) != HAL_OK) {
            return CANBUS_ERROR;
        }
    }

    /* Reject anything not matching a configured filter (default: non-matching
     * standard/extended frames rejected rather than routed to FIFO0). Adjust
     * if you want a different bring-up behaviour. */
    if (HAL_FDCAN_ConfigGlobalFilter(hfdcan, FDCAN_REJECT, FDCAN_REJECT,
                                      FDCAN_REJECT_REMOTE, FDCAN_REJECT_REMOTE) != HAL_OK) {
        return CANBUS_ERROR;
    }

    if (HAL_FDCAN_Start(hfdcan) != HAL_OK) return CANBUS_ERROR;

    uint32_t notif = (filterCfg->fifo == CANBUS_FIFO0)
        ? FDCAN_IT_RX_FIFO0_NEW_MESSAGE
        : FDCAN_IT_RX_FIFO1_NEW_MESSAGE;

    if (HAL_FDCAN_ActivateNotification(hfdcan, notif, 0) != HAL_OK) return CANBUS_ERROR;

    return CANBUS_OK;
}

CANBus_Status_t CANBus_StartAcceptAll(CANBus_Handle_t *can, CANBus_Fifo_t fifo) {
    if (can == NULL || can->halHandle == NULL) return CANBUS_ERROR;

    FDCAN_HandleTypeDef *hfdcan = (FDCAN_HandleTypeDef *)can->halHandle;

    if (HAL_FDCAN_ConfigGlobalFilter(hfdcan,
            (fifo == CANBUS_FIFO0) ? FDCAN_ACCEPT_IN_RX_FIFO0 : FDCAN_ACCEPT_IN_RX_FIFO1,
            (fifo == CANBUS_FIFO0) ? FDCAN_ACCEPT_IN_RX_FIFO0 : FDCAN_ACCEPT_IN_RX_FIFO1,
            FDCAN_FILTER_REMOTE, FDCAN_FILTER_REMOTE) != HAL_OK) {
        return CANBUS_ERROR;
    }

    if (HAL_FDCAN_Start(hfdcan) != HAL_OK) return CANBUS_ERROR;

    uint32_t notif = (fifo == CANBUS_FIFO0)
        ? FDCAN_IT_RX_FIFO0_NEW_MESSAGE
        : FDCAN_IT_RX_FIFO1_NEW_MESSAGE;

    if (HAL_FDCAN_ActivateNotification(hfdcan, notif, 0) != HAL_OK) return CANBUS_ERROR;

    return CANBUS_OK;
}

CANBus_Status_t CANBus_Send(CANBus_Handle_t *can, const CAN_Message_t *msg) {
    if (can == NULL || can->halHandle == NULL || msg == NULL) return CANBUS_ERROR;

    FDCAN_HandleTypeDef *hfdcan = (FDCAN_HandleTypeDef *)can->halHandle;

    if (HAL_FDCAN_GetTxFifoFreeLevel(hfdcan) == 0) {
        return CANBUS_BUSY;
    }

    FDCAN_TxHeaderTypeDef header = {0};
    header.Identifier          = msg->id;
    header.IdType              = (msg->idType == CAN_ID_TYPE_STD) ? FDCAN_STANDARD_ID : FDCAN_EXTENDED_ID;
    header.TxFrameType         = FDCAN_DATA_FRAME;
    header.DataLength          = msg->len;  /* map len -> FDCAN_DLC_BYTES_x yourself if using classic DLC codes */
    header.ErrorStateIndicator = FDCAN_ESI_ACTIVE;
    header.BitRateSwitch       = FDCAN_BRS_OFF;   /* set ON if you configured a data-phase bitrate */
    header.FDFormat            = FDCAN_CLASSIC_CAN; /* set FDCAN_FD_CAN for real CAN FD frames */
    header.TxEventFifoControl  = FDCAN_NO_TX_EVENTS;
    header.MessageMarker       = 0;

    if (HAL_FDCAN_AddMessageToTxFifoQ(hfdcan, &header, (uint8_t *)msg->data) != HAL_OK) {
        return CANBUS_ERROR;
    }
    return CANBUS_OK;
}

void CANBus_RegisterRxCallback(CANBus_Handle_t *can, CAN_RxCallback_t callback) {
    if (can == NULL) return;
    can->rxCallback = callback;
}

static void DispatchRx(FDCAN_HandleTypeDef *hfdcan, uint32_t fifo) {
    CANBus_Handle_t *can = FindInstance(hfdcan);
    if (can == NULL) return;

    FDCAN_RxHeaderTypeDef rxHeader;
    CAN_Message_t msg = {0};

    if (HAL_FDCAN_GetRxMessage(hfdcan, fifo, &rxHeader, msg.data) != HAL_OK) {
        return;
    }

    msg.idType = (rxHeader.IdType == FDCAN_STANDARD_ID) ? CAN_ID_TYPE_STD : CAN_ID_TYPE_EXT;
    msg.id     = rxHeader.Identifier;
    msg.len    = (uint8_t)rxHeader.DataLength; /* convert from FDCAN_DLC_BYTES_x if needed */

    if (can->rxCallback != NULL) {
        can->rxCallback(&msg);
    }
}

void HAL_FDCAN_RxFifo0Callback(FDCAN_HandleTypeDef *hfdcan, uint32_t RxFifo0ITs) {
    if (RxFifo0ITs & FDCAN_IT_RX_FIFO0_NEW_MESSAGE) {
        DispatchRx(hfdcan, FDCAN_RX_FIFO0);
    }
}

void HAL_FDCAN_RxFifo1Callback(FDCAN_HandleTypeDef *hfdcan, uint32_t RxFifo1ITs) {
    if (RxFifo1ITs & FDCAN_IT_RX_FIFO1_NEW_MESSAGE) {
        DispatchRx(hfdcan, FDCAN_RX_FIFO1);
    }
}
