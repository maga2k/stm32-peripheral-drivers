//Author: Mattia Gamberini
//Version: 1.1 - 2026/08/23

#ifndef MOCK_STM32G4XX_HAL_H
#define MOCK_STM32G4XX_HAL_H

#include <stdint.h>

typedef enum { HAL_OK = 0, HAL_ERROR = 1, HAL_BUSY = 2, HAL_TIMEOUT = 3 } HAL_StatusTypeDef;

typedef struct { int _dummy; } FDCAN_HandleTypeDef;

typedef struct {
    uint32_t IdType;
    uint32_t FilterIndex;
    uint32_t FilterType;
    uint32_t FilterConfig;
    uint32_t FilterID1;
    uint32_t FilterID2;
} FDCAN_FilterTypeDef;

typedef struct {
    uint32_t Identifier;
    uint32_t IdType;
    uint32_t TxFrameType;
    uint32_t DataLength;
    uint32_t ErrorStateIndicator;
    uint32_t BitRateSwitch;
    uint32_t FDFormat;
    uint32_t TxEventFifoControl;
    uint32_t MessageMarker;
} FDCAN_TxHeaderTypeDef;

typedef struct {
    uint32_t Identifier;
    uint32_t IdType;
    uint32_t RxFrameType;
    uint32_t DataLength;
    uint32_t ErrorStateIndicator;
    uint32_t RxTimestamp;
    uint32_t FilterIndex;
    uint32_t IsFilterMatchingFrame;
} FDCAN_RxHeaderTypeDef;

/* --- ID type --- */
#define FDCAN_STANDARD_ID  0
#define FDCAN_EXTENDED_ID  1

/* --- Filter config --- */
#define FDCAN_FILTER_DUAL         1
#define FDCAN_FILTER_TO_RXFIFO0   0
#define FDCAN_FILTER_TO_RXFIFO1   1

/* --- Global filter behaviour --- */
#define FDCAN_REJECT               0
#define FDCAN_REJECT_REMOTE        0
#define FDCAN_ACCEPT_IN_RX_FIFO0   1
#define FDCAN_ACCEPT_IN_RX_FIFO1   2
#define FDCAN_FILTER_REMOTE        0

/* --- Frame / format --- */
#define FDCAN_DATA_FRAME      0
#define FDCAN_ESI_ACTIVE      0
#define FDCAN_BRS_OFF         0
#define FDCAN_CLASSIC_CAN     0
#define FDCAN_FD_CAN          1
#define FDCAN_NO_TX_EVENTS    0

/* --- FIFOs / interrupts --- */
#define FDCAN_RX_FIFO0                    0
#define FDCAN_RX_FIFO1                    1
#define FDCAN_IT_RX_FIFO0_NEW_MESSAGE     (1UL << 0)
#define FDCAN_IT_RX_FIFO1_NEW_MESSAGE     (1UL << 1)

HAL_StatusTypeDef HAL_FDCAN_ConfigFilter(FDCAN_HandleTypeDef *hfdcan, FDCAN_FilterTypeDef *sFilterConfig);
HAL_StatusTypeDef HAL_FDCAN_ConfigGlobalFilter(FDCAN_HandleTypeDef *hfdcan,
                                                uint32_t NonMatchingStd, uint32_t NonMatchingExt,
                                                uint32_t RejectRemoteStd, uint32_t RejectRemoteExt);
HAL_StatusTypeDef HAL_FDCAN_Start(FDCAN_HandleTypeDef *hfdcan);
HAL_StatusTypeDef HAL_FDCAN_ActivateNotification(FDCAN_HandleTypeDef *hfdcan, uint32_t ActiveITs, uint32_t BufferIndexes);
uint32_t          HAL_FDCAN_GetTxFifoFreeLevel(FDCAN_HandleTypeDef *hfdcan);
HAL_StatusTypeDef HAL_FDCAN_AddMessageToTxFifoQ(FDCAN_HandleTypeDef *hfdcan, FDCAN_TxHeaderTypeDef *pTxHeader, uint8_t *pTxData);
HAL_StatusTypeDef HAL_FDCAN_GetRxMessage(FDCAN_HandleTypeDef *hfdcan, uint32_t RxLocation, FDCAN_RxHeaderTypeDef *pRxHeader, uint8_t *pRxData);

/* Weak HAL callbacks, overridden in canbus_fdcan.c and invoked directly by tests. */
void HAL_FDCAN_RxFifo0Callback(FDCAN_HandleTypeDef *hfdcan, uint32_t RxFifo0ITs);
void HAL_FDCAN_RxFifo1Callback(FDCAN_HandleTypeDef *hfdcan, uint32_t RxFifo1ITs);

#endif
