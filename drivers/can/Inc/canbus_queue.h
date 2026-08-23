//Author: Mattia Gamberini
//Version: 1.1 - 2026/08/23

#ifndef DRIVERS_CAN_INC_CANBUS_QUEUE_H_
#define DRIVERS_CAN_INC_CANBUS_QUEUE_H_

#include "canbus.h"
#include "cmsis_os2.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Deliberately NOT a field inside CANBus_Handle_t: the hardware backend
 * (canbus_bxcan.c / canbus_fdcan.c) must stay usable in bare-metal projects
 * with zero RTOS dependency. This struct just wraps a CANBus_Handle_t*
 * together with an RTOS queue. */
typedef struct {
    CANBus_Handle_t   *can;
    osMessageQueueId_t queueHandle;
} CANBus_Queue_t;

#define CANBUS_QUEUE_LEN 16

/* Creates the underlying CMSIS-RTOS2 message queue. Call after CANBus_Init(). */
CANBus_Status_t CANBus_Queue_Init(CANBus_Queue_t *q, CANBus_Handle_t *can);

/* Enqueues a message for asynchronous transmission. */
CANBus_Status_t CANBus_Queue_Send(CANBus_Queue_t *q, const CAN_Message_t *msg, uint32_t timeout);

/* Call periodically (task loop / timer) to drain the queue into the HW
 * peripheral via CANBus_Send(), as long as TX resources are free. */
void CANBus_Queue_Service(CANBus_Queue_t *q);

#ifdef __cplusplus
}
#endif

#endif /* DRIVERS_CAN_INC_CANBUS_QUEUE_H_ */
