# Host unit tests (gcc, no STM32 hardware needed).
# Usage:  make -C test        (builds + runs both suites)
#         make -C test clean

CC      := gcc
CFLAGS  := -Wall -Wextra -std=c11
UNITY   := Unity/src
MOCKS   := mocks
INC     := ../drivers/can/Inc
BIN     := /tmp

.PHONY: all clean bxcan queue fdcan

all: bxcan queue fdcan

bxcan:
	$(CC) $(CFLAGS) -I$(UNITY) -I$(MOCKS) -I$(INC) \
		$(UNITY)/unity.c $(MOCKS)/mock_hal.c \
		../drivers/can/Src/canbus_bxcan.c unit/test_canbus_bxcan.c \
		-o $(BIN)/test_canbus_bxcan
	$(BIN)/test_canbus_bxcan

queue:
	$(CC) $(CFLAGS) -I$(UNITY) -I$(MOCKS) -I$(INC) \
		$(UNITY)/unity.c $(MOCKS)/mock_hal.c \
		../drivers/can/Src/canbus_bxcan.c ../drivers/can/Src/canbus_queue.c unit/test_canbus_queue.c \
		-o $(BIN)/test_canbus_queue
	$(BIN)/test_canbus_queue

fdcan:
	$(CC) $(CFLAGS) -DCANBUS_USE_FDCAN -I$(UNITY) -I$(MOCKS) -I$(INC) \
		$(UNITY)/unity.c $(MOCKS)/mock_hal_fdcan.c \
		../drivers/can/Src/canbus_fdcan.c unit/test_canbus_fdcan.c \
		-o $(BIN)/test_canbus_fdcan
	$(BIN)/test_canbus_fdcan

clean:
	rm -f $(BIN)/test_canbus_bxcan $(BIN)/test_canbus_queue $(BIN)/test_canbus_fdcan
