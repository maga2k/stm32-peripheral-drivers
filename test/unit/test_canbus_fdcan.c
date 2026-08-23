//Author: Mattia Gamberini
//Version: 1.1 - 2026/08/23

#include "unity.h"
#include "canbus.h"
#include "mock_hal_fdcan_recorder.h"
#include <string.h>

static CANBus_Handle_t can1;
static FDCAN_HandleTypeDef hfdcan1;

static int rxCallCount = 0;
static CAN_Message_t lastRxMsg;

static void TestRxCallback(const CAN_Message_t *msg) {
    rxCallCount++;
    lastRxMsg = *msg;
}

void setUp(void) {
    Mock_FDCAN_Reset();
    rxCallCount = 0;
    CANBus_Init(&can1, &hfdcan1);
}

void tearDown(void) {}

/* ---------------- Filters ---------------- */

void test_Start_ConfiguresOneFilterPerStdId(void) {
    uint32_t stdIds[] = {0x100, 0x101, 0x102};
    CANBus_FilterConfig_t cfg = { .fifo = CANBUS_FIFO0 };

    CANBus_Status_t res = CANBus_Start(&can1, &cfg, stdIds, 3, NULL, 0);

    TEST_ASSERT_EQUAL(CANBUS_OK, res);
    TEST_ASSERT_EQUAL_INT(3, mock_fdcanFilterCallCount);
    TEST_ASSERT_EQUAL_UINT32(FDCAN_STANDARD_ID, mock_fdcanFilterCalls[0].IdType);
    TEST_ASSERT_EQUAL_UINT32(0x100, mock_fdcanFilterCalls[0].FilterID1);
    TEST_ASSERT_EQUAL_UINT32(0x102, mock_fdcanFilterCalls[2].FilterID1);
}

void test_Start_ConfiguresExtendedFilters(void) {
    uint32_t extIds[] = {0x1ABCDEF};
    CANBus_FilterConfig_t cfg = { .fifo = CANBUS_FIFO0 };

    CANBus_Status_t res = CANBus_Start(&can1, &cfg, NULL, 0, extIds, 1);

    TEST_ASSERT_EQUAL(CANBUS_OK, res);
    TEST_ASSERT_EQUAL_INT(1, mock_fdcanFilterCallCount);
    TEST_ASSERT_EQUAL_UINT32(FDCAN_EXTENDED_ID, mock_fdcanFilterCalls[0].IdType);
    TEST_ASSERT_EQUAL_UINT32(0x1ABCDEF, mock_fdcanFilterCalls[0].FilterID1);
}

void test_Start_RejectsNonMatchingFrames(void) {
    uint32_t stdIds[] = {0x100};
    CANBus_FilterConfig_t cfg = { .fifo = CANBUS_FIFO0 };

    CANBus_Start(&can1, &cfg, stdIds, 1, NULL, 0);

    TEST_ASSERT_EQUAL_UINT32(FDCAN_REJECT, mock_lastGlobalFilter_NonMatchingStd);
    TEST_ASSERT_EQUAL_UINT32(FDCAN_REJECT, mock_lastGlobalFilter_NonMatchingExt);
}

void test_StartAcceptAll_AcceptsEverythingOnFifo0(void) {
    CANBus_Status_t res = CANBus_StartAcceptAll(&can1, CANBUS_FIFO0);

    TEST_ASSERT_EQUAL(CANBUS_OK, res);
    TEST_ASSERT_EQUAL_UINT32(FDCAN_ACCEPT_IN_RX_FIFO0, mock_lastGlobalFilter_NonMatchingStd);
}

/* ---------------- TX ---------------- */

void test_Send_StdId_BuildsCorrectHeader(void) {
    mock_fdcanFreeTxFifo = 1;

    CAN_Message_t msg = { .id = 0x102, .idType = CAN_ID_TYPE_STD, .len = 3, .data = {1,2,3} };
    CANBus_Status_t res = CANBus_Send(&can1, &msg);

    TEST_ASSERT_EQUAL(CANBUS_OK, res);
    TEST_ASSERT_EQUAL_INT(1, mock_fdcanTxCallCount);
    TEST_ASSERT_EQUAL_UINT32(0x102, mock_fdcanTxHeaders[0].Identifier);
    TEST_ASSERT_EQUAL_UINT32(FDCAN_STANDARD_ID, mock_fdcanTxHeaders[0].IdType);
    TEST_ASSERT_EQUAL_UINT32(3, mock_fdcanTxHeaders[0].DataLength);
    TEST_ASSERT_EQUAL_UINT8(2, mock_fdcanTxData[0][1]);
}

void test_Send_ExtId_BuildsCorrectHeader(void) {
    mock_fdcanFreeTxFifo = 1;

    CAN_Message_t msg = { .id = 0x1ABCDEF, .idType = CAN_ID_TYPE_EXT, .len = 8, .data = {0} };
    CANBus_Status_t res = CANBus_Send(&can1, &msg);

    TEST_ASSERT_EQUAL(CANBUS_OK, res);
    TEST_ASSERT_EQUAL_UINT32(FDCAN_EXTENDED_ID, mock_fdcanTxHeaders[0].IdType);
    TEST_ASSERT_EQUAL_UINT32(0x1ABCDEF, mock_fdcanTxHeaders[0].Identifier);
}

void test_Send_ReturnsBusy_WhenTxFifoFull(void) {
    mock_fdcanFreeTxFifo = 0;

    CAN_Message_t msg = { .id = 0x100, .idType = CAN_ID_TYPE_STD, .len = 1, .data = {9} };
    CANBus_Status_t res = CANBus_Send(&can1, &msg);

    TEST_ASSERT_EQUAL(CANBUS_BUSY, res);
    TEST_ASSERT_EQUAL_INT(0, mock_fdcanTxCallCount);
}

/* ---------------- RX dispatch ---------------- */

void test_Rx_StdMessage_InvokesCallbackWithCorrectFields(void) {
    CANBus_RegisterRxCallback(&can1, TestRxCallback);

    mock_fdcanRxHeaderToReturn.IdType     = FDCAN_STANDARD_ID;
    mock_fdcanRxHeaderToReturn.Identifier = 0x103;
    mock_fdcanRxHeaderToReturn.DataLength = 4;
    uint8_t payload[64] = {0};
    payload[1] = 20;
    memcpy(mock_fdcanRxDataToReturn, payload, 64);

    HAL_FDCAN_RxFifo0Callback(&hfdcan1, FDCAN_IT_RX_FIFO0_NEW_MESSAGE);

    TEST_ASSERT_EQUAL_INT(1, rxCallCount);
    TEST_ASSERT_EQUAL_UINT32(0x103, lastRxMsg.id);
    TEST_ASSERT_EQUAL(CAN_ID_TYPE_STD, lastRxMsg.idType);
    TEST_ASSERT_EQUAL_UINT8(4, lastRxMsg.len);
    TEST_ASSERT_EQUAL_UINT8(20, lastRxMsg.data[1]);
}

void test_Rx_IgnoresCallback_WhenOtherInterruptFlag(void) {
    CANBus_RegisterRxCallback(&can1, TestRxCallback);

    /* Some other IT bit set, not FDCAN_IT_RX_FIFO0_NEW_MESSAGE */
    HAL_FDCAN_RxFifo0Callback(&hfdcan1, 0);

    TEST_ASSERT_EQUAL_INT(0, rxCallCount);
}

void test_Rx_UnknownInstance_DoesNotCrash_DoesNotInvokeCallback(void) {
    CANBus_RegisterRxCallback(&can1, TestRxCallback);

    FDCAN_HandleTypeDef someOtherHandle; /* never passed to CANBus_Init */
    HAL_FDCAN_RxFifo0Callback(&someOtherHandle, FDCAN_IT_RX_FIFO0_NEW_MESSAGE);

    TEST_ASSERT_EQUAL_INT(0, rxCallCount);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_Start_ConfiguresOneFilterPerStdId);
    RUN_TEST(test_Start_ConfiguresExtendedFilters);
    RUN_TEST(test_Start_RejectsNonMatchingFrames);
    RUN_TEST(test_StartAcceptAll_AcceptsEverythingOnFifo0);
    RUN_TEST(test_Send_StdId_BuildsCorrectHeader);
    RUN_TEST(test_Send_ExtId_BuildsCorrectHeader);
    RUN_TEST(test_Send_ReturnsBusy_WhenTxFifoFull);
    RUN_TEST(test_Rx_StdMessage_InvokesCallbackWithCorrectFields);
    RUN_TEST(test_Rx_IgnoresCallback_WhenOtherInterruptFlag);
    RUN_TEST(test_Rx_UnknownInstance_DoesNotCrash_DoesNotInvokeCallback);
    return UNITY_END();
}
