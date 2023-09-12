#ifndef __STM32_EXTIRQ_H
#define __STM32_EXTIRQ_H

#include <stdint.h>

typedef enum {
    ExternalInterruptTriggerModeDisabled = 0,
    ExternalInterruptTriggerModeRising,
    ExternalInterruptTriggerModeFalling,
    ExternalInterruptTriggerModeRisingFalling,
} ExternalInterruptTriggerMode;

typedef void (*ExternalInterruptCallback)(void*);

void config_external_interrupt(uint32_t pin, ExternalInterruptTriggerMode mode, ExternalInterruptCallback callback, void* data);

#endif // extirq.h
