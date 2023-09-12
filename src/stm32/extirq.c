#include "generic/armcm_boot.h"
#include "generic/irq.h"
#include "internal.h"
#include "gpio.h"
#include "sched.h"
#include "command.h"

#include "extirq.h"

#define LINE(n) (1 << n)

static ExternalInterruptCallback irq_callbacks[16];
static void* irq_data[16];

#define CHECK_IRQ(num, line) \
    irq_disable(); \
    if ((EXTI->PR & LINE(line)) != RESET) { \
        if (irq_callbacks[line] != 0) { \
            irq_callbacks[line](irq_data[line]); \
        } \
    } \
    NVIC_ClearPendingIRQ(num); \
    irq_enable();

void EXTI0_IRQn_IRQHandler(void)
{
    CHECK_IRQ(EXTI0_IRQn, 0);
}
DECL_ARMCM_IRQ(EXTI0_IRQn_IRQHandler, EXTI0_IRQn);

void EXTI1_IRQn_IRQHandler(void)
{
    CHECK_IRQ(EXTI1_IRQn, 1);
}
DECL_ARMCM_IRQ(EXTI1_IRQn_IRQHandler, EXTI1_IRQn);

void EXTI2_IRQn_IRQHandler(void)
{
    CHECK_IRQ(EXTI2_IRQn, 2);
}
DECL_ARMCM_IRQ(EXTI2_IRQn_IRQHandler, EXTI2_IRQn);

void EXTI3_IRQn_IRQHandler(void)
{
    CHECK_IRQ(EXTI3_IRQn, 3);
}
DECL_ARMCM_IRQ(EXTI3_IRQn_IRQHandler, EXTI3_IRQn);

void EXTI4_IRQn_IRQHandler(void)
{
    CHECK_IRQ(EXTI4_IRQn, 4);
}
DECL_ARMCM_IRQ(EXTI4_IRQn_IRQHandler, EXTI4_IRQn);

void EXTI9_5_IRQn_IRQHandler(void)
{
    irq_disable();
    if ((EXTI->PR & LINE(5)) != RESET && irq_callbacks[5] != 0) {
        irq_callbacks[5](irq_data[5]);
    }
    if ((EXTI->PR & LINE(6)) != RESET && irq_callbacks[6] != 0) {
        irq_callbacks[6](irq_data[6]);
    }
    if ((EXTI->PR & LINE(7)) != RESET && irq_callbacks[7] != 0) {
        irq_callbacks[7](irq_data[7]);
    }
    if ((EXTI->PR & LINE(8)) != RESET && irq_callbacks[8] != 0) {
        irq_callbacks[8](irq_data[8]);
    }
    if ((EXTI->PR & LINE(9)) != RESET && irq_callbacks[9] != 0) {
        irq_callbacks[9](irq_data[9]);
    }
    NVIC_ClearPendingIRQ(EXTI15_10_IRQn);
    irq_enable();
}
DECL_ARMCM_IRQ(EXTI9_5_IRQn_IRQHandler, EXTI9_5_IRQn);

void EXTI15_10_IRQn_IRQHandler(void)
{
    irq_disable();
    if ((EXTI->PR & LINE(10)) != RESET && irq_callbacks[10] != 0) {
        irq_callbacks[10](irq_data[10]);
    }
    if ((EXTI->PR & LINE(11)) != RESET && irq_callbacks[11] != 0) {
        irq_callbacks[11](irq_data[11]);
    }
    if ((EXTI->PR & LINE(12)) != RESET && irq_callbacks[12] != 0) {
        irq_callbacks[12](irq_data[12]);
    }
    if ((EXTI->PR & LINE(13)) != RESET && irq_callbacks[13] != 0) {
        irq_callbacks[13](irq_data[13]);
    }
    if ((EXTI->PR & LINE(14)) != RESET && irq_callbacks[14] != 0) {
        irq_callbacks[14](irq_data[14]);
    }
    if ((EXTI->PR & LINE(15)) != RESET && irq_callbacks[15] != 0) {
        irq_callbacks[15](irq_data[15]);
    }
    NVIC_ClearPendingIRQ(EXTI15_10_IRQn);
    irq_enable();
}
DECL_ARMCM_IRQ(EXTI15_10_IRQn_IRQHandler, EXTI15_10_IRQn);

static IRQn_Type get_pin_line(uint32_t pin) {
    IRQn_Type line;
    switch(pin % 16) {
        case 0: line = EXTI0_IRQn; break;
        case 1: line = EXTI1_IRQn; break;
        case 2: line = EXTI2_IRQn; break;
        case 3: line = EXTI3_IRQn; break;
        case 4: line = EXTI4_IRQn; break;
        case 5:
        case 6:
        case 7:
        case 8:
        case 9: line = EXTI9_5_IRQn; break;
        case 10:
        case 11:
        case 12:
        case 13:
        case 14:
        case 15: line = EXTI15_10_IRQn; break;
    }
    return line;
}

void config_external_interrupt(uint32_t pin, ExternalInterruptTriggerMode mode, ExternalInterruptCallback callback, void* data)
{
    IRQn_Type line =  get_pin_line(pin);

    if (mode == ExternalInterruptTriggerModeDisabled)
    {
        NVIC_DisableIRQ(line);

        CLEAR_BIT(EXTI->IMR, line);

        irq_disable();
        irq_callbacks[pin % 16] = 0;
        irq_data[pin % 16] = 0;
        irq_enable();

        return;
    }

    irq_disable();

    if (irq_callbacks[pin % 16] != 0) {
        try_shutdown("External interrupt conflict");
        return;
    }

    irq_callbacks[pin % 16] = callback;
    irq_data[pin % 16] = data;

    switch(mode) {
        case ExternalInterruptTriggerModeDisabled: break;
        case ExternalInterruptTriggerModeRising:
            SET_BIT(EXTI->RTSR, line);
            break;
        case ExternalInterruptTriggerModeFalling:
            SET_BIT(EXTI->FTSR, line);
            break;
        case ExternalInterruptTriggerModeRisingFalling:
            SET_BIT(EXTI->RTSR, line);
            SET_BIT(EXTI->FTSR, line);
            break;
    }

    // enable interrupt
    SET_BIT(EXTI->IMR, line);

    NVIC_SetPriority(line, 0);
    NVIC_EnableIRQ(line);

    irq_enable();
    return;
}
