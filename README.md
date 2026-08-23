# STM32 Peripheral Drivers

Reusable, hardware-abstracted peripheral drivers for STM32 microcontrollers.

The goal is a collection of portable drivers reusable across multiple STM32
projects and MCU families, keeping application code independent from the
underlying HAL peripheral implementation. Currently focused on CAN/CAN FD.

## Repository Structure

```
stm32-peripheral-drivers/
├── drivers/can/
│   ├── Inc/
│   │   ├── canbus.h         # generic API (backend-agnostic)
│   │   └── canbus_queue.h   # optional CMSIS-RTOS2 TX queue
│   └── Src/
│       ├── canbus_bxcan.c   # backend: F1/F4/F7... (classic bxCAN) — tested, see test/
│       ├── canbus_fdcan.c   # backend: G4/H7/U5... (FDCAN) — NOT built/tested here, see note below
│       └── canbus_queue.c   # RTOS queue, only file depending on cmsis_os2.h
├── examples/can/
│   ├── candb.h / candb.c            # example project-specific frame database
│   └── canbus_usage_example.c       # end-to-end usage reference
├── test/
│   ├── mocks/             # host-side stand-ins for stm32f1xx_hal.h / stm32g4xx_hal.h / cmsis_os2.h
│   ├── unit/              # Unity test suites (bxcan, queue, fdcan)
│   ├── Unity/             # ThrowTheSwitch/Unity framework (vendored)
│   └── Makefile
└── README.md
```

## Architecture

```
                     APPLICATION
                          │
                          ▼
                 ┌─────────────────┐
                 │     CANDB       │   examples/can (project-specific)
                 └────────┬────────┘
                          │
                          ▼
                 ┌─────────────────┐
                 │   canbus.h      │   generic API
                 └────────┬────────┘
              ┌───────────┴───────────┐
              ▼                       ▼
       canbus_bxcan.c          canbus_fdcan.c
              │                       │
              ▼                       ▼
          STM32 HAL               STM32 HAL
```

Application code talks to `canbus.h` only. The active backend
(`canbus_bxcan.c` or `canbus_fdcan.c`) is chosen by which file you add to
your build — the application does not need to know which one is in use.

### A deliberate change from the original design sketch

An earlier draft had `CANBus_Queue_Init(&can1)` implying the RTOS queue
lives inside the CAN handle itself. That would make `CANBus_Handle_t`
(and therefore the hardware backend) depend on CMSIS-RTOS2, contradicting
the "RTOS is optional" principle below. Instead, the queue is its own
struct (`CANBus_Queue_t`) that *wraps* a `CANBus_Handle_t*`:

```c
typedef struct {
    CANBus_Handle_t   *can;
    osMessageQueueId_t queueHandle;
} CANBus_Queue_t;
```

`canbus_bxcan.c` / `canbus_fdcan.c` never include `cmsis_os2.h`. Only
`canbus_queue.c` does.

## Generic Message Type

```c
#define CANBUS_MAX_DATA_LEN 64  /* classic CAN: 0..8, CAN FD: up to 64 */

typedef struct {
    uint32_t     id;
    CAN_IdType_t idType;   /* CAN_ID_TYPE_STD (11-bit) or CAN_ID_TYPE_EXT (29-bit) */
    uint8_t      len;
    uint8_t      data[CANBUS_MAX_DATA_LEN];
} CAN_Message_t;
```

## Basic Usage

CubeMX/CubeIDE still owns low-level peripheral init (`MX_CAN1_Init()` /
`MX_FDCAN1_Init()`). The driver is initialized afterwards:

```c
#include "canbus.h"

extern CAN_HandleTypeDef hcan1;
static CANBus_Handle_t can1;

void CAN1_Init(void) {
    CANBus_Init(&can1, &hcan1);
    CANBus_StartAcceptAll(&can1, CANBUS_FIFO0); /* bring-up only, see below */
}
```

With real filters and the RX callback — see
`examples/can/canbus_usage_example.c` for the full version, including the
frame database and both direct/queued transmission.

## CAN Frame Database

The driver itself knows nothing about your protocol. Project-specific
frames live in `examples/can/candb.h`/`.c` — copy and extend these into
your own project (rename if you like, `candb.h` is just a convention):

```c
const CANDB_Frame_t CANDB_Frames[CANDB_FRAME_COUNT] = {
    [CANDB_BoardInfo] = { .id = 0x102, .idType = CAN_ID_TYPE_STD, .len = 8, .name = "BoardInfo" },
    /* ... */
};
```

```c
CAN_Message_t msg;
CANDB_CreateMessage(CANDB_BoardInfo, &msg);
msg.data[0] = versionMajor;
CANBus_Send(&can1, &msg);
```

Received messages map back to the database:

```c
CANDB_FrameId_t frame = CANDB_Lookup(msg->id, msg->idType);
switch (frame) {
    case CANDB_BoardInfo: /* ... */ break;
    default: /* unknown frame */ break;
}
```

## RX Handling

```c
CANBus_RegisterRxCallback(&can1, CAN1_RxCallback);
```

The callback runs from **interrupt context** — keep it short, no
`osDelay()`, no blocking mutexes, no logging. If you need heavier
processing, copy the message into a queue/event and handle it from a task.

## Direct vs Queued Transmission

- `CANBus_Send(&can1, &msg)` — immediate, may return `CANBUS_BUSY` if no
  HW TX slot is free right now.
- `CANBus_Queue_Send(&q, &msg, timeout)` + a periodic
  `CANBus_Queue_Service(&q)` (e.g. every 1ms from a task) — asynchronous,
  generally preferable for periodic/application-level traffic. If the
  hardware is momentarily busy, the message is put back at the tail of
  the queue and retried on the next service call.

## Filters

```c
static const uint32_t stdIds[] = {0x100, 0x101, 0x102, 0x103};
static const uint32_t extIds[] = {0x01ABCDEF};

CANBus_FilterConfig_t cfg = {
    .fifo = CANBUS_FIFO0,
    .bankStart = 0, .bankCount = 14, .slaveStartFilterBank = 14 /* single-CAN */
};

CANBus_Start(&can1, &cfg, stdIds, 4, extIds, 1);
```

bxCAN packs standard IDs 4-per-bank (16-bit scale) and extended IDs
2-per-bank (32-bit scale); `CANBus_Start` returns `CANBUS_ERROR` if
`bankCount` isn't enough for the IDs you passed, rather than silently
truncating the list. FDCAN uses one filter element per ID (see caveat
below on filter RAM usage). `CANBus_StartAcceptAll()` accepts everything —
bring-up/debug only.

## Adding a New CAN Backend

Implement the six functions declared in `canbus.h`
(`CANBus_Init`, `CANBus_Start`, `CANBus_StartAcceptAll`, `CANBus_Send`,
`CANBus_RegisterRxCallback`, plus the RX ISR dispatch) against your
peripheral's HAL. Look at `canbus_bxcan.c` for the pattern, including the
small instance registry needed because HAL RX callbacks only hand you the
raw peripheral handle, not your `CANBus_Handle_t`.

## Generated `can.c`

CubeMX may generate `Core/Inc/can.h` / `Core/Src/can.c`. **Do not** name
these files the same as the driver's (`canbus.h`, `canbus_bxcan.c`, ...) —
different names avoid conflicts when CubeMX regenerates the project.

## Testing

Unit tests run on the host (gcc), no STM32 hardware needed. They mock
`HAL_CAN_*` / `HAL_FDCAN_*` / `osMessageQueue*` so the driver logic is
verified in isolation.

```bash
cd test
make          # builds + runs all suites
```

- **bxCAN** (`canbus_bxcan.c`): 8 tests — filter packing (std/ext ID list,
  4-per-bank / 2-per-bank), not-enough-banks error, TX header construction
  (std/ext), busy-mailbox handling, RX dispatch, unknown-instance safety.
- **Queue** (`canbus_queue.c`): 4 tests — drain, multi-message drain,
  busy-hardware requeue, empty queue.
- **FDCAN** (`canbus_fdcan.c`): 10 tests — per-ID filter configuration
  (std/ext), global-filter reject-non-matching behaviour, accept-all, TX
  header construction (std/ext), TX-FIFO-full handling, RX dispatch
  (including the interrupt-flag check), unknown-instance safety. Built
  with `-DCANBUS_USE_FDCAN` against a mock `stm32g4xx_hal.h` — logic is
  verified, but **field names/macros haven't been cross-checked against a
  real CubeMX-generated `stm32g4xx_hal_fdcan.h`**. Build the driver itself
  (not just the mock) in CubeIDE against your actual target before relying
  on it in production, in case any HAL field/macro name differs from what's
  assumed here (this has happened across HAL versions before, e.g.
  `FDCAN_FilterTypeDef` field renames between some G4/H7 HAL releases).

All 22 tests pass as of this writing.

## Requirements

- STM32 HAL + CMSIS (from STM32CubeMX/CubeIDE)
- CMSIS-RTOS2, only if you use `canbus_queue.c`

## Design Principles

1. Application code uses `CANBus_Send()`, never `HAL_CAN_AddTxMessage()` directly.
2. HAL calls live only inside the backend (`canbus_bxcan.c` / `canbus_fdcan.c`).
3. RTOS functionality is optional and isolated in `canbus_queue.c`.
4. Protocol/frame definitions are project-specific, never inside the driver.
5. Compose: generic API + hardware backend + optional queue + project database.

## Roadmap

- [x] Bring `canbus_fdcan.c` under host test (mock FDCAN in `test/mocks/`)
- [ ] Cross-check `canbus_fdcan.c` field/macro names against a real CubeMX G4/H7 project
- [ ] SPI / I2C / UART drivers, same layered pattern
- [ ] CAN filter mask/range helpers (fewer filter slots for contiguous ID ranges)
- [ ] Bus-off recovery helpers
- [ ] DMA support

## License

MIT license
