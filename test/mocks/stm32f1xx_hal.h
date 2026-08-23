//Author: Mattia Gamberini
//Version: 1.1 - 2026/08/23

#ifndef MOCK_STM32F1XX_HAL_H
#define MOCK_STM32F1XX_HAL_H

#include <stdint.h>

typedef enum { HAL_OK = 0, HAL_ERROR = 1, HAL_BUSY = 2, HAL_TIMEOUT = 3 } HAL_StatusTypeDef;

typedef struct { int _dummy; } CAN_HandleTypeDef;

typedef struct {
    uint32_t FilterIdHigh;
    uint32_t FilterIdLow;
    uint32_t FilterMaskIdHigh;
    uint32_t FilterMaskIdLow;
    uint32_t FilterFIFOAssignment;
    uint32_t FilterBank;
    uint32_t FilterMode;
    uint32_t FilterScale;
    uint32_t FilterActivation;
    uint32_t SlaveStartFilterBank;
} CAN_FilterTypeDef;

typedef struct {
    uint32_t StdId;
    uint32_t ExtId;
    uint32_t IDE;
    uint32_t RTR;
    uint32_t DLC;
    uint32_t TransmitGlobalTime;
} CAN_TxHeaderTypeDef;

typedef struct {
    uint32_t StdId;
    uint32_t ExtId;
    uint32_t IDE;
    uint32_t RTR;
    uint32_t DLC;
    uint32_t FilterMatchIndex;
    uint32_t Timestamp;
} CAN_RxHeaderTypeDef;

#define ENABLE   1
#define DISABLE  0

#define CAN_ID_STD   0
#define CAN_ID_EXT   1
#define CAN_RTR_DATA 0

#define CAN_FILTERMODE_IDMASK  0
#define CAN_FILTERMODE_IDLIST  1
#define CAN_FILTERSCALE_16BIT  0
#define CAN_FILTERSCALE_32BIT  1

#define CAN_FILTER_FIFO0  0
#define CAN_FILTER_FIFO1  1
#define CAN_RX_FIFO0      0
#define CAN_RX_FIFO1      1

#define CAN_IT_RX_FIFO0_MSG_PENDING  (1UL << 1)
#define CAN_IT_RX_FIFO1_MSG_PENDING  (1UL << 2)

HAL_StatusTypeDef HAL_CAN_ConfigFilter(CAN_HandleTypeDef *hcan, CAN_FilterTypeDef *sFilterConfig);
HAL_StatusTypeDef HAL_CAN_Start(CAN_HandleTypeDef *hcan);
HAL_StatusTypeDef HAL_CAN_ActivateNotification(CAN_HandleTypeDef *hcan, uint32_t ActiveITs);
uint32_t          HAL_CAN_GetTxMailboxesFreeLevel(CAN_HandleTypeDef *hcan);
HAL_StatusTypeDef HAL_CAN_AddTxMessage(CAN_HandleTypeDef *hcan, CAN_TxHeaderTypeDef *pHeader, uint8_t aData[], uint32_t *pTxMailbox);
HAL_StatusTypeDef HAL_CAN_GetRxMessage(CAN_HandleTypeDef *hcan, uint32_t RxFifo, CAN_RxHeaderTypeDef *pHeader, uint8_t aData[]);

/* Weak HAL callbacks, overridden in canbus_bxcan.c and invoked directly by tests. */
void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan);
void HAL_CAN_RxFifo1MsgPendingCallback(CAN_HandleTypeDef *hcan);

#endif
