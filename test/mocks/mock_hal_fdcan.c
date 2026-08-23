//Author: Mattia Gamberini
//Version: 1.1 - 2026/08/23

#include "stm32g4xx_hal.h"
#include <string.h>

/* ---------------- HAL_FDCAN_ConfigFilter ---------------- */
#define MOCK_MAX_FILTER_CALLS 32
FDCAN_FilterTypeDef mock_fdcanFilterCalls[MOCK_MAX_FILTER_CALLS];
int mock_fdcanFilterCallCount = 0;
HAL_StatusTypeDef mock_HAL_FDCAN_ConfigFilter_retval = HAL_OK;

HAL_StatusTypeDef HAL_FDCAN_ConfigFilter(FDCAN_HandleTypeDef *hfdcan, FDCAN_FilterTypeDef *sFilterConfig) {
    (void)hfdcan;
    if (mock_fdcanFilterCallCount < MOCK_MAX_FILTER_CALLS) {
        mock_fdcanFilterCalls[mock_fdcanFilterCallCount++] = *sFilterConfig;
    }
    return mock_HAL_FDCAN_ConfigFilter_retval;
}

/* ---------------- Global filter / Start / Notification ---------------- */
HAL_StatusTypeDef mock_HAL_FDCAN_ConfigGlobalFilter_retval = HAL_OK;
uint32_t mock_lastGlobalFilter_NonMatchingStd = 0xFFFFFFFF;
uint32_t mock_lastGlobalFilter_NonMatchingExt = 0xFFFFFFFF;

HAL_StatusTypeDef HAL_FDCAN_ConfigGlobalFilter(FDCAN_HandleTypeDef *hfdcan,
                                                uint32_t NonMatchingStd, uint32_t NonMatchingExt,
                                                uint32_t RejectRemoteStd, uint32_t RejectRemoteExt) {
    (void)hfdcan; (void)RejectRemoteStd; (void)RejectRemoteExt;
    mock_lastGlobalFilter_NonMatchingStd = NonMatchingStd;
    mock_lastGlobalFilter_NonMatchingExt = NonMatchingExt;
    return mock_HAL_FDCAN_ConfigGlobalFilter_retval;
}

HAL_StatusTypeDef mock_HAL_FDCAN_Start_retval = HAL_OK;
HAL_StatusTypeDef HAL_FDCAN_Start(FDCAN_HandleTypeDef *hfdcan) {
    (void)hfdcan;
    return mock_HAL_FDCAN_Start_retval;
}

uint32_t mock_lastActivatedFdcanITs = 0;
HAL_StatusTypeDef mock_HAL_FDCAN_ActivateNotification_retval = HAL_OK;
HAL_StatusTypeDef HAL_FDCAN_ActivateNotification(FDCAN_HandleTypeDef *hfdcan, uint32_t ActiveITs, uint32_t BufferIndexes) {
    (void)hfdcan; (void)BufferIndexes;
    mock_lastActivatedFdcanITs = ActiveITs;
    return mock_HAL_FDCAN_ActivateNotification_retval;
}

/* ---------------- TX ---------------- */
uint32_t mock_fdcanFreeTxFifo = 3;
uint32_t HAL_FDCAN_GetTxFifoFreeLevel(FDCAN_HandleTypeDef *hfdcan) {
    (void)hfdcan;
    return mock_fdcanFreeTxFifo;
}

#define MOCK_MAX_TX_CALLS 32
FDCAN_TxHeaderTypeDef mock_fdcanTxHeaders[MOCK_MAX_TX_CALLS];
uint8_t               mock_fdcanTxData[MOCK_MAX_TX_CALLS][64];
int mock_fdcanTxCallCount = 0;
HAL_StatusTypeDef mock_HAL_FDCAN_AddMessageToTxFifoQ_retval = HAL_OK;

HAL_StatusTypeDef HAL_FDCAN_AddMessageToTxFifoQ(FDCAN_HandleTypeDef *hfdcan, FDCAN_TxHeaderTypeDef *pTxHeader, uint8_t *pTxData) {
    (void)hfdcan;
    if (mock_fdcanTxCallCount < MOCK_MAX_TX_CALLS) {
        mock_fdcanTxHeaders[mock_fdcanTxCallCount] = *pTxHeader;
        memcpy(mock_fdcanTxData[mock_fdcanTxCallCount], pTxData, 64);
        mock_fdcanTxCallCount++;
    }
    return mock_HAL_FDCAN_AddMessageToTxFifoQ_retval;
}

/* ---------------- RX ---------------- */
FDCAN_RxHeaderTypeDef mock_fdcanRxHeaderToReturn;
uint8_t               mock_fdcanRxDataToReturn[64];
HAL_StatusTypeDef      mock_HAL_FDCAN_GetRxMessage_retval = HAL_OK;

HAL_StatusTypeDef HAL_FDCAN_GetRxMessage(FDCAN_HandleTypeDef *hfdcan, uint32_t RxLocation,
                                          FDCAN_RxHeaderTypeDef *pRxHeader, uint8_t *pRxData) {
    (void)hfdcan; (void)RxLocation;
    *pRxHeader = mock_fdcanRxHeaderToReturn;
    memcpy(pRxData, mock_fdcanRxDataToReturn, 64);
    return mock_HAL_FDCAN_GetRxMessage_retval;
}

/* ---------------- reset helper for tests ---------------- */
void Mock_FDCAN_Reset(void) {
    mock_fdcanFilterCallCount = 0;
    mock_HAL_FDCAN_ConfigFilter_retval = HAL_OK;
    mock_HAL_FDCAN_ConfigGlobalFilter_retval = HAL_OK;
    mock_lastGlobalFilter_NonMatchingStd = 0xFFFFFFFF;
    mock_lastGlobalFilter_NonMatchingExt = 0xFFFFFFFF;
    mock_HAL_FDCAN_Start_retval = HAL_OK;
    mock_lastActivatedFdcanITs = 0;
    mock_HAL_FDCAN_ActivateNotification_retval = HAL_OK;
    mock_fdcanFreeTxFifo = 3;
    mock_fdcanTxCallCount = 0;
    mock_HAL_FDCAN_AddMessageToTxFifoQ_retval = HAL_OK;
    memset(&mock_fdcanRxHeaderToReturn, 0, sizeof(mock_fdcanRxHeaderToReturn));
    memset(mock_fdcanRxDataToReturn, 0, sizeof(mock_fdcanRxDataToReturn));
    mock_HAL_FDCAN_GetRxMessage_retval = HAL_OK;
}
