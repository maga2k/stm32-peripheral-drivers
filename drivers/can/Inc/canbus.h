//Author: Mattia Gamberini
//Version: 1.1 - 2026/08/23

#ifndef DRIVERS_CAN_INC_CANBUS_H_
#define DRIVERS_CAN_INC_CANBUS_H_

/* Select the HAL header for your MCU family/backend. In your real CubeIDE
 * project, define CANBUS_USE_FDCAN in the build settings (or before this
 * include) when linking canbus_fdcan.c on a G4/H7/U5 target; otherwise the
 * bxCAN family header is used. Adjust the two #include lines below to match
 * your exact family (e.g. stm32h7xx_hal.h instead of stm32g4xx_hal.h). */
#if defined(CANBUS_USE_FDCAN)
    #include "stm32g4xx_hal.h"   /* FDCAN family example; swap for your family */
#else
    #include "stm32f1xx_hal.h"   /* bxCAN family example; swap for your family */
#endif

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ==========================================================================
 * GENERIC TYPES
 * ========================================================================== */

typedef enum {
    CANBUS_OK    = 0,
    CANBUS_ERROR = 1,
    CANBUS_BUSY  = 2
} CANBus_Status_t;

typedef enum {
    CAN_ID_TYPE_STD = 0,   /* 11-bit */
    CAN_ID_TYPE_EXT = 1    /* 29-bit */
} CAN_IdType_t;

#define CANBUS_MAX_DATA_LEN 64  /* Classic CAN uses 0..8, CAN FD up to 64 */

typedef struct {
    uint32_t     id;
    CAN_IdType_t idType;
    uint8_t      len;                         /* 0..8 classic, 0..64 FD */
    uint8_t      data[CANBUS_MAX_DATA_LEN];
} CAN_Message_t;

typedef void (*CAN_RxCallback_t)(const CAN_Message_t *msg);

/* Opaque-ish handle: `halHandle` points to the backend-specific HAL handle
 * (CAN_HandleTypeDef* for bxCAN, FDCAN_HandleTypeDef* for FDCAN). The
 * application never dereferences it directly. */
typedef struct {
    void             *halHandle;
    CAN_RxCallback_t  rxCallback;
} CANBus_Handle_t;

/* ==========================================================================
 * FILTER CONFIGURATION
 * ========================================================================== */

typedef enum {
    CANBUS_FIFO0 = 0,
    CANBUS_FIFO1 = 1
} CANBus_Fifo_t;

typedef struct {
    CANBus_Fifo_t fifo;
    uint8_t       bankStart;             /* bxCAN only: first filter bank to use */
    uint8_t       bankCount;             /* bxCAN only: banks available */
    uint8_t       slaveStartFilterBank;  /* bxCAN dual-CAN only, typ. 14 for single-CAN */
} CANBus_FilterConfig_t;

/* ==========================================================================
 * API IMPLEMENTED BY EACH BACKEND (canbus_bxcan.c / canbus_fdcan.c)
 * ========================================================================== */

/* Binds a CANBus_Handle_t to a HAL peripheral handle. Call once per CAN
 * instance, after MX_CANx_Init() / MX_FDCANx_Init(). */
CANBus_Status_t CANBus_Init(CANBus_Handle_t *can, void *halHandle);

/* Configures ID-list filters (std and/or ext) and starts the peripheral
 * with RX interrupts enabled on both FIFOs. */
CANBus_Status_t CANBus_Start(CANBus_Handle_t *can,
                              const CANBus_FilterConfig_t *filterCfg,
                              const uint32_t *acceptedStdIds, uint16_t stdLen,
                              const uint32_t *acceptedExtIds, uint16_t extLen);

/* Bring-up/debug helper: accepts every message. Avoid in production. */
CANBus_Status_t CANBus_StartAcceptAll(CANBus_Handle_t *can, CANBus_Fifo_t fifo);

/* Direct transmission. Returns CANBUS_BUSY if no HW TX slot is free right now. */
CANBus_Status_t CANBus_Send(CANBus_Handle_t *can, const CAN_Message_t *msg);

/* Registers the RX callback, invoked from ISR context for every message
 * accepted by the filters. Keep it short: no blocking calls inside. */
void CANBus_RegisterRxCallback(CANBus_Handle_t *can, CAN_RxCallback_t callback);

#ifdef __cplusplus
}
#endif

#endif /* DRIVERS_CAN_INC_CANBUS_H_ */
