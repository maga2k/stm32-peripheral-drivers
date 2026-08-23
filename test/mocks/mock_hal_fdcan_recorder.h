//Author: Mattia Gamberini
//Version: 1.1 - 2026/08/23

#ifndef MOCK_HAL_FDCAN_RECORDER_H
#define MOCK_HAL_FDCAN_RECORDER_H

#include "stm32g4xx_hal.h"

extern FDCAN_FilterTypeDef mock_fdcanFilterCalls[];
extern int mock_fdcanFilterCallCount;
extern HAL_StatusTypeDef mock_HAL_FDCAN_ConfigFilter_retval;

extern HAL_StatusTypeDef mock_HAL_FDCAN_ConfigGlobalFilter_retval;
extern uint32_t mock_lastGlobalFilter_NonMatchingStd;
extern uint32_t mock_lastGlobalFilter_NonMatchingExt;

extern uint32_t mock_fdcanFreeTxFifo;
extern FDCAN_TxHeaderTypeDef mock_fdcanTxHeaders[];
extern uint8_t mock_fdcanTxData[][64];
extern int mock_fdcanTxCallCount;
extern HAL_StatusTypeDef mock_HAL_FDCAN_AddMessageToTxFifoQ_retval;

extern FDCAN_RxHeaderTypeDef mock_fdcanRxHeaderToReturn;
extern uint8_t mock_fdcanRxDataToReturn[64];
extern HAL_StatusTypeDef mock_HAL_FDCAN_GetRxMessage_retval;

extern uint32_t mock_lastActivatedFdcanITs;

void Mock_FDCAN_Reset(void);

#endif
