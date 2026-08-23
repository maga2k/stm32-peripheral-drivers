//Author: Mattia Gamberini
//Version: 1.1 - 2026/08/23

#include "unity.h"
#include "canbus.h"
#include "mock_hal_recorder.h"
#include <string.h>

static CANBus_Handle_t can1;
static CAN_HandleTypeDef hcan1;

static int rxCallCount = 0;
static CAN_Message_t lastRxMsg;

static void TestRxCallback(const CAN_Message_t *msg) {
    rxCallCount++;
    lastRxMsg = *msg;
}

void setUp(void) {
    Mock_HAL_Reset();
    rxCallCount = 0;
    CANBus_Init(&can1, &hcan1);
}

void tearDown(void) {}

/* ---------------- Filters ---------------- */

void test_Start_PacksFourStdIdsInOneBank(void) {
    uint32_t stdIds[] = {0x100, 0x101, 0x102, 0x103};
    CANBus_FilterConfig_t cfg = { .fifo = CANBUS_FIFO0, .bankStart = 0, .bankCount = 14, .slaveStartFilterBank = 14 };

    CANBus_Status_t res = CANBus_Start(&can1, &cfg, stdIds, 4, NULL, 0);

    TEST_ASSERT_EQUAL(CANBUS_OK, res);
    TEST_ASSERT_EQUAL_INT(1, mock_filterCallCount);
    TEST_ASSERT_EQUAL_UINT32(0x100u << 5, mock_filterCalls[0].FilterIdLow);
    TEST_ASSERT_EQUAL_UINT32(0x103u << 5, mock_filterCalls[0].FilterMaskIdHigh);
}

void test_Start_PacksExtendedIds(void) {
    uint32_t extIds[] = {0x1ABCDEF};
    CANBus_FilterConfig_t cfg = { .fifo = CANBUS_FIFO0, .bankStart = 0, .bankCount = 14, .slaveStartFilterBank = 14 };

    CANBus_Status_t res = CANBus_Start(&can1, &cfg, NULL, 0, extIds, 1);

    TEST_ASSERT_EQUAL(CANBUS_OK, res);
    TEST_ASSERT_EQUAL_INT(1, mock_filterCallCount);
    TEST_ASSERT_EQUAL(CAN_FILTERSCALE_32BIT, mock_filterCalls[0].FilterScale);
}

void test_Start_FailsWhenNotEnoughBanks(void) {
    uint32_t stdIds[8] = {0x100,0x101,0x102,0x103,0x104,0x105,0x106,0x107}; /* needs 2 banks */
    CANBus_FilterConfig_t cfg = { .fifo = CANBUS_FIFO0, .bankStart = 0, .bankCount = 1, .slaveStartFilterBank = 14 };

    CANBus_Status_t res = CANBus_Start(&can1, &cfg, stdIds, 8, NULL, 0);

    TEST_ASSERT_EQUAL(CANBUS_ERROR, res);
    TEST_ASSERT_EQUAL_INT(0, mock_filterCallCount);
}

/* ---------------- TX ---------------- */

void test_Send_StdId_BuildsCorrectHeader(void) {
    mock_freeMailboxes = 1;

    CAN_Message_t msg = { .id = 0x102, .idType = CAN_ID_TYPE_STD, .len = 3, .data = {1,2,3} };
    CANBus_Status_t res = CANBus_Send(&can1, &msg);

    TEST_ASSERT_EQUAL(CANBUS_OK, res);
    TEST_ASSERT_EQUAL_INT(1, mock_txCallCount);
    TEST_ASSERT_EQUAL_UINT32(0x102, mock_txHeaders[0].StdId);
    TEST_ASSERT_EQUAL_UINT32(CAN_ID_STD, mock_txHeaders[0].IDE);
    TEST_ASSERT_EQUAL_UINT32(3, mock_txHeaders[0].DLC);
    TEST_ASSERT_EQUAL_UINT8(2, mock_txData[0][1]);
}

void test_Send_ExtId_BuildsCorrectHeader(void) {
    mock_freeMailboxes = 1;

    CAN_Message_t msg = { .id = 0x1ABCDEF, .idType = CAN_ID_TYPE_EXT, .len = 8, .data = {0} };
    CANBus_Status_t res = CANBus_Send(&can1, &msg);

    TEST_ASSERT_EQUAL(CANBUS_OK, res);
    TEST_ASSERT_EQUAL_UINT32(CAN_ID_EXT, mock_txHeaders[0].IDE);
    TEST_ASSERT_EQUAL_UINT32(0x1ABCDEF, mock_txHeaders[0].ExtId);
}

void test_Send_ReturnsBusy_WhenNoMailboxFree(void) {
    mock_freeMailboxes = 0;

    CAN_Message_t msg = { .id = 0x100, .idType = CAN_ID_TYPE_STD, .len = 1, .data = {9} };
    CANBus_Status_t res = CANBus_Send(&can1, &msg);

    TEST_ASSERT_EQUAL(CANBUS_BUSY, res);
    TEST_ASSERT_EQUAL_INT(0, mock_txCallCount);
}

/* ---------------- RX dispatch ---------------- */

void test_Rx_StdMessage_InvokesCallbackWithCorrectFields(void) {
    CANBus_RegisterRxCallback(&can1, TestRxCallback);

    mock_rxHeaderToReturn.IDE   = CAN_ID_STD;
    mock_rxHeaderToReturn.StdId = 0x103;
    mock_rxHeaderToReturn.DLC   = 4;
    uint8_t payload[8] = {10,20,30,40,0,0,0,0};
    memcpy(mock_rxDataToReturn, payload, 8);

    HAL_CAN_RxFifo0MsgPendingCallback(&hcan1);

    TEST_ASSERT_EQUAL_INT(1, rxCallCount);
    TEST_ASSERT_EQUAL_UINT32(0x103, lastRxMsg.id);
    TEST_ASSERT_EQUAL(CAN_ID_TYPE_STD, lastRxMsg.idType);
    TEST_ASSERT_EQUAL_UINT8(4, lastRxMsg.len);
    TEST_ASSERT_EQUAL_UINT8(20, lastRxMsg.data[1]);
}

void test_Rx_UnknownInstance_DoesNotCrash_DoesNotInvokeCallback(void) {
    CANBus_RegisterRxCallback(&can1, TestRxCallback);

    CAN_HandleTypeDef someOtherHcan; /* never passed to CANBus_Init */
    HAL_CAN_RxFifo0MsgPendingCallback(&someOtherHcan);

    TEST_ASSERT_EQUAL_INT(0, rxCallCount);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_Start_PacksFourStdIdsInOneBank);
    RUN_TEST(test_Start_PacksExtendedIds);
    RUN_TEST(test_Start_FailsWhenNotEnoughBanks);
    RUN_TEST(test_Send_StdId_BuildsCorrectHeader);
    RUN_TEST(test_Send_ExtId_BuildsCorrectHeader);
    RUN_TEST(test_Send_ReturnsBusy_WhenNoMailboxFree);
    RUN_TEST(test_Rx_StdMessage_InvokesCallbackWithCorrectFields);
    RUN_TEST(test_Rx_UnknownInstance_DoesNotCrash_DoesNotInvokeCallback);
    return UNITY_END();
}
