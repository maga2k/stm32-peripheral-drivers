//Author: Mattia Gamberini
//Version: 1.1 - 2026/08/23

#include "unity.h"
#include "canbus.h"
#include "canbus_queue.h"
#include "mock_hal_recorder.h"

static CANBus_Handle_t can1;
static CAN_HandleTypeDef hcan1;
static CANBus_Queue_t q;

void setUp(void) {
    Mock_HAL_Reset();
    Mock_Queues_Reset();
    CANBus_Init(&can1, &hcan1);
    CANBus_Queue_Init(&q, &can1);
}

void tearDown(void) {}

void test_Service_SendsQueuedMessage_WhenMailboxFree(void) {
    mock_freeMailboxes = 1;

    CAN_Message_t msg = { .id = 0x100, .idType = CAN_ID_TYPE_STD, .len = 2, .data = {1,2} };
    CANBus_Queue_Send(&q, &msg, 0);

    CANBus_Queue_Service(&q);

    TEST_ASSERT_EQUAL_INT(1, mock_txCallCount);
    TEST_ASSERT_EQUAL_UINT32(0x100, mock_txHeaders[0].StdId);
}

void test_Service_DrainsMultipleMessages(void) {
    mock_freeMailboxes = 5;

    for (uint32_t i = 0; i < 3; i++) {
        CAN_Message_t msg = { .id = 0x100 + i, .idType = CAN_ID_TYPE_STD, .len = 1, .data = {(uint8_t)i} };
        CANBus_Queue_Send(&q, &msg, 0);
    }

    CANBus_Queue_Service(&q);

    TEST_ASSERT_EQUAL_INT(3, mock_txCallCount);
}

void test_Service_RequeuesMessage_WhenHardwareBusy(void) {
    mock_freeMailboxes = 0; /* CANBus_Send will return CANBUS_BUSY */

    CAN_Message_t msg = { .id = 0x100, .idType = CAN_ID_TYPE_STD, .len = 1, .data = {7} };
    CANBus_Queue_Send(&q, &msg, 0);

    CANBus_Queue_Service(&q);
    TEST_ASSERT_EQUAL_INT(0, mock_txCallCount); /* nothing actually sent */

    /* hardware becomes free: next service call should now send it */
    mock_freeMailboxes = 1;
    CANBus_Queue_Service(&q);
    TEST_ASSERT_EQUAL_INT(1, mock_txCallCount);
    TEST_ASSERT_EQUAL_UINT32(0x100, mock_txHeaders[0].StdId);
}

void test_Service_DoesNothing_WhenQueueEmpty(void) {
    mock_freeMailboxes = 5;

    CANBus_Queue_Service(&q);

    TEST_ASSERT_EQUAL_INT(0, mock_txCallCount);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_Service_SendsQueuedMessage_WhenMailboxFree);
    RUN_TEST(test_Service_DrainsMultipleMessages);
    RUN_TEST(test_Service_RequeuesMessage_WhenHardwareBusy);
    RUN_TEST(test_Service_DoesNothing_WhenQueueEmpty);
    return UNITY_END();
}
