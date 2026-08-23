//Author: Mattia Gamberini
//Version: 1.1 - 2026/08/23

#ifndef MOCK_CMSIS_OS2_H
#define MOCK_CMSIS_OS2_H

#include <stdint.h>
#include <stddef.h>

typedef void *osMessageQueueId_t;
typedef struct { int _dummy; } osMessageQueueAttr_t;

typedef enum {
    osOK             = 0,
    osError          = -1,
    osErrorTimeout   = -2,
    osErrorResource  = -3
} osStatus_t;

osMessageQueueId_t osMessageQueueNew(uint32_t msg_count, uint32_t msg_size, const osMessageQueueAttr_t *attr);
osStatus_t osMessageQueuePut(osMessageQueueId_t mq_id, const void *msg_ptr, uint8_t msg_prio, uint32_t timeout);
osStatus_t osMessageQueueGet(osMessageQueueId_t mq_id, void *msg_ptr, uint8_t *msg_prio, uint32_t timeout);

/* Not implemented in the mock: only declared so example/task code referencing
 * it can still compile if pulled into a host build by mistake. */
osStatus_t osDelay(uint32_t ticks);

#endif
