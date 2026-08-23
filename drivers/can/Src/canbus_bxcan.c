//Author: Mattia Gamberini
//Version: 1.1 - 2026/08/23

/**
 ******************************************************************************
 * @file    canbus_bxcan.c
 * @brief   bxCAN backend for the generic canbus.h API (STM32F1/F4/F7 and
 *          other STM32 families using the classic bxCAN peripheral).
 *
 * This file is HAL-only: it does NOT depend on CMSIS-RTOS2. RTOS-based
 * queuing lives separately in canbus_queue.c.
 ******************************************************************************
 */

#include "canbus.h"
#include <string.h>

#define CANBUS_MAX_INSTANCES 2  /* e.g. CAN1 + CAN2 on dual-CAN devices */

/* --------------------------------------------------------------------
 * Instance registry: HAL RX callbacks only give us the raw
 * CAN_HandleTypeDef*, so we need to map it back to our CANBus_Handle_t.
 * -------------------------------------------------------------------- */
static CANBus_Handle_t *s_instances[CANBUS_MAX_INSTANCES] = {0};

static void RegisterInstance(CANBus_Handle_t *can) {
    for (int i = 0; i < CANBUS_MAX_INSTANCES; i++) {
        if (s_instances[i] == NULL) {
            s_instances[i] = can;
            return;
        }
    }
    /* No free slot: increase CANBUS_MAX_INSTANCES if you have more CAN
     * peripherals than that on your MCU. */
}

static CANBus_Handle_t *FindInstance(CAN_HandleTypeDef *hcan) {
    for (int i = 0; i < CANBUS_MAX_INSTANCES; i++) {
        if (s_instances[i] != NULL && s_instances[i]->halHandle == (void *)hcan) {
            return s_instances[i];
        }
    }
    return NULL;
}

/* --------------------------------------------------------------------
 * Filter ID packing helpers
 * -------------------------------------------------------------------- */
static inline uint16_t PackStdId(uint32_t id) {
    return (uint16_t)((id & 0x7FFUL) << 5);
}

static inline uint32_t PackExtId(uint32_t id) {
    return ((id & 0x1FFFFFFFUL) << 3) | (1UL << 2);
}

static HAL_StatusTypeDef ConfigIdListFilters(CAN_HandleTypeDef *hcan,
                                              const uint32_t *stdIds, uint16_t stdLen,
                                              const uint32_t *extIds, uint16_t extLen,
                                              const CANBus_FilterConfig_t *cfg)
{
    uint8_t stdBanksNeeded = (stdLen == 0) ? 0 : (uint8_t)((stdLen + 3) / 4);
    uint8_t extBanksNeeded = (extLen == 0) ? 0 : (uint8_t)((extLen + 1) / 2);

    if ((uint16_t)(stdBanksNeeded + extBanksNeeded) > cfg->bankCount) {
        return HAL_ERROR; /* not enough banks: reduce ID count or free up banks */
    }

    CAN_FilterTypeDef sFilterConfig = {0};
    uint8_t bank = cfg->bankStart;

    uint16_t idx = 0;
    for (uint8_t b = 0; b < stdBanksNeeded; b++) {
        uint16_t vals[4] = {0, 0, 0, 0};
        for (uint8_t slot = 0; slot < 4 && idx < stdLen; slot++, idx++) {
            vals[slot] = PackStdId(stdIds[idx]);
        }
        sFilterConfig.FilterBank           = bank++;
        sFilterConfig.FilterMode           = CAN_FILTERMODE_IDLIST;
        sFilterConfig.FilterScale          = CAN_FILTERSCALE_16BIT;
        sFilterConfig.FilterIdLow          = vals[0];
        sFilterConfig.FilterMaskIdLow      = vals[1];
        sFilterConfig.FilterIdHigh         = vals[2];
        sFilterConfig.FilterMaskIdHigh     = vals[3];
        sFilterConfig.FilterFIFOAssignment = (uint32_t)cfg->fifo;
        sFilterConfig.FilterActivation     = ENABLE;
        sFilterConfig.SlaveStartFilterBank = cfg->slaveStartFilterBank;

        if (HAL_CAN_ConfigFilter(hcan, &sFilterConfig) != HAL_OK) return HAL_ERROR;
    }

    idx = 0;
    for (uint8_t b = 0; b < extBanksNeeded; b++) {
        uint32_t id1 = 0, id2 = 0;
        if (idx < extLen) { id1 = PackExtId(extIds[idx]); idx++; }
        if (idx < extLen) { id2 = PackExtId(extIds[idx]); idx++; }

        sFilterConfig.FilterBank           = bank++;
        sFilterConfig.FilterMode           = CAN_FILTERMODE_IDLIST;
        sFilterConfig.FilterScale          = CAN_FILTERSCALE_32BIT;
        sFilterConfig.FilterIdHigh         = (id1 >> 16) & 0xFFFF;
        sFilterConfig.FilterIdLow          = id1 & 0xFFFF;
        sFilterConfig.FilterMaskIdHigh     = (id2 >> 16) & 0xFFFF;
        sFilterConfig.FilterMaskIdLow      = id2 & 0xFFFF;
        sFilterConfig.FilterFIFOAssignment = (uint32_t)cfg->fifo;
        sFilterConfig.FilterActivation     = ENABLE;
        sFilterConfig.SlaveStartFilterBank = cfg->slaveStartFilterBank;

        if (HAL_CAN_ConfigFilter(hcan, &sFilterConfig) != HAL_OK) return HAL_ERROR;
    }

    return HAL_OK;
}

/* ==========================================================================
 * PUBLIC API
 * ========================================================================== */

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

    CAN_HandleTypeDef *hcan = (CAN_HandleTypeDef *)can->halHandle;

    if (ConfigIdListFilters(hcan, acceptedStdIds, stdLen, acceptedExtIds, extLen, filterCfg) != HAL_OK) {
        return CANBUS_ERROR;
    }
    if (HAL_CAN_Start(hcan) != HAL_OK) {
        return CANBUS_ERROR;
    }
    if (HAL_CAN_ActivateNotification(hcan,
            CAN_IT_RX_FIFO0_MSG_PENDING | CAN_IT_RX_FIFO1_MSG_PENDING) != HAL_OK) {
        return CANBUS_ERROR;
    }
    return CANBUS_OK;
}

CANBus_Status_t CANBus_StartAcceptAll(CANBus_Handle_t *can, CANBus_Fifo_t fifo) {
    if (can == NULL || can->halHandle == NULL) return CANBUS_ERROR;

    CAN_HandleTypeDef *hcan = (CAN_HandleTypeDef *)can->halHandle;

    CAN_FilterTypeDef sFilterConfig = {0};
    sFilterConfig.FilterBank           = 0;
    sFilterConfig.FilterMode           = CAN_FILTERMODE_IDMASK;
    sFilterConfig.FilterScale          = CAN_FILTERSCALE_32BIT;
    sFilterConfig.FilterIdHigh         = 0x0000;
    sFilterConfig.FilterIdLow          = 0x0000;
    sFilterConfig.FilterMaskIdHigh     = 0x0000;
    sFilterConfig.FilterMaskIdLow      = 0x0000;
    sFilterConfig.FilterFIFOAssignment = (uint32_t)fifo;
    sFilterConfig.FilterActivation     = ENABLE;
    sFilterConfig.SlaveStartFilterBank = 14;

    if (HAL_CAN_ConfigFilter(hcan, &sFilterConfig) != HAL_OK) return CANBUS_ERROR;
    if (HAL_CAN_Start(hcan) != HAL_OK) return CANBUS_ERROR;
    if (HAL_CAN_ActivateNotification(hcan,
            CAN_IT_RX_FIFO0_MSG_PENDING | CAN_IT_RX_FIFO1_MSG_PENDING) != HAL_OK) {
        return CANBUS_ERROR;
    }
    return CANBUS_OK;
}

CANBus_Status_t CANBus_Send(CANBus_Handle_t *can, const CAN_Message_t *msg) {
    if (can == NULL || can->halHandle == NULL || msg == NULL) return CANBUS_ERROR;

    CAN_HandleTypeDef *hcan = (CAN_HandleTypeDef *)can->halHandle;

    if (HAL_CAN_GetTxMailboxesFreeLevel(hcan) == 0) {
        return CANBUS_BUSY;
    }

    CAN_TxHeaderTypeDef header = {0};
    header.DLC                = (msg->len > 8) ? 8 : msg->len; /* bxCAN: classic CAN, max 8 */
    header.RTR                = CAN_RTR_DATA;
    header.TransmitGlobalTime = DISABLE;

    if (msg->idType == CAN_ID_TYPE_STD) {
        header.IDE   = CAN_ID_STD;
        header.StdId = msg->id & 0x7FF;
    } else {
        header.IDE   = CAN_ID_EXT;
        header.ExtId = msg->id & 0x1FFFFFFF;
    }

    uint32_t txMailbox;
    if (HAL_CAN_AddTxMessage(hcan, &header, (uint8_t *)msg->data, &txMailbox) != HAL_OK) {
        return CANBUS_ERROR;
    }
    return CANBUS_OK;
}

void CANBus_RegisterRxCallback(CANBus_Handle_t *can, CAN_RxCallback_t callback) {
    if (can == NULL) return;
    can->rxCallback = callback;
}

/* --------------------------------------------------------------------
 * RX dispatch. These override the weak HAL callbacks: nothing else in
 * the project should redefine HAL_CAN_RxFifoxMsgPendingCallback.
 * -------------------------------------------------------------------- */
static void DispatchRx(CAN_HandleTypeDef *hcan, uint32_t fifo) {
    CANBus_Handle_t *can = FindInstance(hcan);
    if (can == NULL) return; /* message on an instance we don't manage */

    CAN_RxHeaderTypeDef rxHeader;
    CAN_Message_t msg = {0};

    if (HAL_CAN_GetRxMessage(hcan, fifo, &rxHeader, msg.data) != HAL_OK) {
        return;
    }

    if (rxHeader.IDE == CAN_ID_STD) {
        msg.idType = CAN_ID_TYPE_STD;
        msg.id     = rxHeader.StdId;
    } else {
        msg.idType = CAN_ID_TYPE_EXT;
        msg.id     = rxHeader.ExtId;
    }
    msg.len = (uint8_t)rxHeader.DLC;

    if (can->rxCallback != NULL) {
        can->rxCallback(&msg);
    }
}

void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan) {
    DispatchRx(hcan, CAN_RX_FIFO0);
}

void HAL_CAN_RxFifo1MsgPendingCallback(CAN_HandleTypeDef *hcan) {
    DispatchRx(hcan, CAN_RX_FIFO1);
}
