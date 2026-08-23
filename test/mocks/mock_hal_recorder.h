//Author: Mattia Gamberini
//Version: 1.1 - 2026/08/23

#ifndef MOCK_HAL_RECORDER_H
#define MOCK_HAL_RECORDER_H

#include "stm32f1xx_hal.h"

extern CAN_FilterTypeDef mock_filterCalls[];
extern int mock_filterCallCount;
extern HAL_StatusTypeDef mock_HAL_CAN_ConfigFilter_retval;

extern uint32_t mock_freeMailboxes;
extern CAN_TxHeaderTypeDef mock_txHeaders[];
extern uint8_t mock_txData[][8];
extern int mock_txCallCount;
extern HAL_StatusTypeDef mock_HAL_CAN_AddTxMessage_retval;

extern CAN_RxHeaderTypeDef mock_rxHeaderToReturn;
extern uint8_t mock_rxDataToReturn[8];
extern HAL_StatusTypeDef mock_HAL_CAN_GetRxMessage_retval;

extern uint32_t mock_lastActivatedITs;

void Mock_HAL_Reset(void);
void Mock_Queues_Reset(void);

#endif
