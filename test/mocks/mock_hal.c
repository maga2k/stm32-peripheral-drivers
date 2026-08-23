//Author: Mattia Gamberini
//Version: 1.1 - 2026/08/23

#include "stm32f1xx_hal.h"
#include "cmsis_os2.h"
#include <string.h>
#include <stdlib.h>

/* ---------------- HAL_CAN_ConfigFilter ---------------- */
#define MOCK_MAX_FILTER_CALLS 32
CAN_FilterTypeDef mock_filterCalls[MOCK_MAX_FILTER_CALLS];
int mock_filterCallCount = 0;
HAL_StatusTypeDef mock_HAL_CAN_ConfigFilter_retval = HAL_OK;

HAL_StatusTypeDef HAL_CAN_ConfigFilter(CAN_HandleTypeDef *hcan, CAN_FilterTypeDef *sFilterConfig) {
    (void)hcan;
    if (mock_filterCallCount < MOCK_MAX_FILTER_CALLS) {
        mock_filterCalls[mock_filterCallCount++] = *sFilterConfig;
    }
    return mock_HAL_CAN_ConfigFilter_retval;
}

/* ---------------- HAL_CAN_Start / ActivateNotification ---------------- */
HAL_StatusTypeDef mock_HAL_CAN_Start_retval = HAL_OK;
HAL_StatusTypeDef HAL_CAN_Start(CAN_HandleTypeDef *hcan) {
    (void)hcan;
    return mock_HAL_CAN_Start_retval;
}

uint32_t mock_lastActivatedITs = 0;
HAL_StatusTypeDef mock_HAL_CAN_ActivateNotification_retval = HAL_OK;
HAL_StatusTypeDef HAL_CAN_ActivateNotification(CAN_HandleTypeDef *hcan, uint32_t ActiveITs) {
    (void)hcan;
    mock_lastActivatedITs = ActiveITs;
    return mock_HAL_CAN_ActivateNotification_retval;
}

/* ---------------- TX mailboxes ---------------- */
uint32_t mock_freeMailboxes = 3;
uint32_t HAL_CAN_GetTxMailboxesFreeLevel(CAN_HandleTypeDef *hcan) {
    (void)hcan;
    return mock_freeMailboxes;
}

#define MOCK_MAX_TX_CALLS 32
CAN_TxHeaderTypeDef mock_txHeaders[MOCK_MAX_TX_CALLS];
uint8_t             mock_txData[MOCK_MAX_TX_CALLS][8];
int mock_txCallCount = 0;
HAL_StatusTypeDef mock_HAL_CAN_AddTxMessage_retval = HAL_OK;

HAL_StatusTypeDef HAL_CAN_AddTxMessage(CAN_HandleTypeDef *hcan, CAN_TxHeaderTypeDef *pHeader,
                                        uint8_t aData[], uint32_t *pTxMailbox) {
    (void)hcan;
    if (mock_txCallCount < MOCK_MAX_TX_CALLS) {
        mock_txHeaders[mock_txCallCount] = *pHeader;
        memcpy(mock_txData[mock_txCallCount], aData, 8);
        mock_txCallCount++;
    }
    if (pTxMailbox) *pTxMailbox = 0;
    return mock_HAL_CAN_AddTxMessage_retval;
}

/* ---------------- RX ---------------- */
CAN_RxHeaderTypeDef mock_rxHeaderToReturn;
uint8_t             mock_rxDataToReturn[8];
HAL_StatusTypeDef   mock_HAL_CAN_GetRxMessage_retval = HAL_OK;

HAL_StatusTypeDef HAL_CAN_GetRxMessage(CAN_HandleTypeDef *hcan, uint32_t RxFifo,
                                        CAN_RxHeaderTypeDef *pHeader, uint8_t aData[]) {
    (void)hcan; (void)RxFifo;
    *pHeader = mock_rxHeaderToReturn;
    memcpy(aData, mock_rxDataToReturn, 8);
    return mock_HAL_CAN_GetRxMessage_retval;
}

/* ---------------- reset helper for tests ---------------- */
void Mock_HAL_Reset(void) {
    mock_filterCallCount = 0;
    mock_HAL_CAN_ConfigFilter_retval = HAL_OK;
    mock_HAL_CAN_Start_retval = HAL_OK;
    mock_lastActivatedITs = 0;
    mock_HAL_CAN_ActivateNotification_retval = HAL_OK;
    mock_freeMailboxes = 3;
    mock_txCallCount = 0;
    mock_HAL_CAN_AddTxMessage_retval = HAL_OK;
    memset(&mock_rxHeaderToReturn, 0, sizeof(mock_rxHeaderToReturn));
    memset(mock_rxDataToReturn, 0, sizeof(mock_rxDataToReturn));
    mock_HAL_CAN_GetRxMessage_retval = HAL_OK;
}

/* ---------------- cmsis_os2 message queue mock (ring buffer) ---------------- */
typedef struct {
    uint8_t *buffer;
    uint32_t msg_size;
    uint32_t capacity;
    uint32_t head, tail, count;
} MockQueue_t;

#define MOCK_MAX_QUEUES 4
static MockQueue_t mock_queues[MOCK_MAX_QUEUES];
static int mock_queueUsed[MOCK_MAX_QUEUES] = {0};

osMessageQueueId_t osMessageQueueNew(uint32_t msg_count, uint32_t msg_size, const osMessageQueueAttr_t *attr) {
    (void)attr;
    for (int i = 0; i < MOCK_MAX_QUEUES; i++) {
        if (!mock_queueUsed[i]) {
            mock_queues[i].buffer   = (uint8_t *)malloc((size_t)msg_count * msg_size);
            mock_queues[i].msg_size = msg_size;
            mock_queues[i].capacity = msg_count;
            mock_queues[i].head = mock_queues[i].tail = mock_queues[i].count = 0;
            mock_queueUsed[i] = 1;
            return (osMessageQueueId_t)&mock_queues[i];
        }
    }
    return NULL;
}

osStatus_t osMessageQueuePut(osMessageQueueId_t mq_id, const void *msg_ptr, uint8_t msg_prio, uint32_t timeout) {
    (void)msg_prio; (void)timeout;
    MockQueue_t *q = (MockQueue_t *)mq_id;
    if (!q || q->count >= q->capacity) return osErrorResource;
    memcpy(q->buffer + ((size_t)q->tail * q->msg_size), msg_ptr, q->msg_size);
    q->tail = (q->tail + 1) % q->capacity;
    q->count++;
    return osOK;
}

osStatus_t osMessageQueueGet(osMessageQueueId_t mq_id, void *msg_ptr, uint8_t *msg_prio, uint32_t timeout) {
    (void)msg_prio; (void)timeout;
    MockQueue_t *q = (MockQueue_t *)mq_id;
    if (!q || q->count == 0) return osErrorResource;
    memcpy(msg_ptr, q->buffer + ((size_t)q->head * q->msg_size), q->msg_size);
    q->head = (q->head + 1) % q->capacity;
    q->count--;
    return osOK;
}

osStatus_t osDelay(uint32_t ticks) {
    (void)ticks;
    return osOK;
}

void Mock_Queues_Reset(void) {
    for (int i = 0; i < MOCK_MAX_QUEUES; i++) {
        if (mock_queueUsed[i]) {
            free(mock_queues[i].buffer);
            mock_queueUsed[i] = 0;
        }
    }
}
