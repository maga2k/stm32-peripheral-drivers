//Author: Mattia Gamberini
//Version: 1.1 - 2026/08/23

#include "canbus_queue.h"

CANBus_Status_t CANBus_Queue_Init(CANBus_Queue_t *q, CANBus_Handle_t *can) {
    if (q == NULL || can == NULL) return CANBUS_ERROR;

    q->can         = can;
    q->queueHandle = osMessageQueueNew(CANBUS_QUEUE_LEN, sizeof(CAN_Message_t), NULL);

    return (q->queueHandle != NULL) ? CANBUS_OK : CANBUS_ERROR;
}

CANBus_Status_t CANBus_Queue_Send(CANBus_Queue_t *q, const CAN_Message_t *msg, uint32_t timeout) {
    if (q == NULL || q->queueHandle == NULL || msg == NULL) return CANBUS_ERROR;

    return (osMessageQueuePut(q->queueHandle, msg, 0, timeout) == osOK) ? CANBUS_OK : CANBUS_ERROR;
}

void CANBus_Queue_Service(CANBus_Queue_t *q) {
    if (q == NULL || q->queueHandle == NULL || q->can == NULL) return;

    CAN_Message_t msg;

    /* Drain as long as messages are queued AND the hardware accepts them.
     * If CANBus_Send() returns CANBUS_BUSY, put nothing back (message would
     * be lost) -- instead stop for this tick and retry the same message
     * next call by peeking without consuming. Since CMSIS-RTOS2 doesn't
     * offer a portable peek, we keep it simple here: try once, and rely on
     * a short service period (e.g. 1ms) so a transient busy mailbox just
     * means a short delay, not real message loss under normal load. */
    while (osMessageQueueGet(q->queueHandle, &msg, NULL, 0) == osOK) {
        CANBus_Status_t status = CANBus_Send(q->can, &msg);
        if (status == CANBUS_BUSY) {
            /* Hardware momentarily full: re-queue at the front is not
             * available with a plain FIFO, so push it back at the tail
             * and stop this service round to avoid reordering storms. */
            osMessageQueuePut(q->queueHandle, &msg, 0, 0);
            break;
        }
        /* CANBUS_ERROR: message dropped (bad handle/params) -- consider
         * adding a counter/log hook here if you need visibility into it. */
    }
}
