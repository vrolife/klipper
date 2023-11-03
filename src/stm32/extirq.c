#include "generic/armcm_boot.h"
#include "generic/irq.h"
#include "internal.h"
#include "gpio.h"
#include "sched.h"
#include "command.h"

#include "extirq.h"

#define BIT(n) (1 << n)

#define CLEAR_IT_FLAG(pin) WRITE_REG(EXTI->PR, BIT(pin))
#define IS_IT_ACTIVE(pin) ((EXTI->PR & BIT(pin)) != RESET)

#define CHECK_IRQ(pin) \
    if (IS_IT_ACTIVE(pin)) { \
        CLEAR_IT_FLAG(pin); /* clear flag */ \
        if (irq_callbacks[pin] != 0) { \
            irq_callbacks[pin](irq_data[pin]); \
        } \
    }

static ExternalInterruptCallback irq_callbacks[16] = { 0 };
static void* irq_data[16] = { 0 };

void EXTI0_IRQn_IRQHandler(void)
{
    irq_disable();
    CHECK_IRQ(0);
    irq_enable();
}
DECL_ARMCM_IRQ(EXTI0_IRQn_IRQHandler, EXTI0_IRQn);

void EXTI1_IRQn_IRQHandler(void)
{
    irq_disable();
    CHECK_IRQ(1);
    irq_enable();
}
DECL_ARMCM_IRQ(EXTI1_IRQn_IRQHandler, EXTI1_IRQn);

void EXTI2_IRQn_IRQHandler(void)
{
    irq_disable();
    CHECK_IRQ(2);
    irq_enable();
}
DECL_ARMCM_IRQ(EXTI2_IRQn_IRQHandler, EXTI2_IRQn);

void EXTI3_IRQn_IRQHandler(void)
{
    irq_disable();
    CHECK_IRQ(3);
    irq_enable();
}
DECL_ARMCM_IRQ(EXTI3_IRQn_IRQHandler, EXTI3_IRQn);

void EXTI4_IRQn_IRQHandler(void)
{
    irq_disable();
    CHECK_IRQ(4);
    irq_enable();
}
DECL_ARMCM_IRQ(EXTI4_IRQn_IRQHandler, EXTI4_IRQn);

void EXTI9_5_IRQn_IRQHandler(void)
{
    irq_disable();
    CHECK_IRQ(5);
    CHECK_IRQ(6);
    CHECK_IRQ(7);
    CHECK_IRQ(8);
    CHECK_IRQ(9);
    irq_enable();
}
DECL_ARMCM_IRQ(EXTI9_5_IRQn_IRQHandler, EXTI9_5_IRQn);

void EXTI15_10_IRQn_IRQHandler(void)
{
    irq_disable();
    CHECK_IRQ(10);
    CHECK_IRQ(11);
    CHECK_IRQ(12);
    CHECK_IRQ(13);
    CHECK_IRQ(14);
    CHECK_IRQ(15);
    irq_enable();
}
DECL_ARMCM_IRQ(EXTI15_10_IRQn_IRQHandler, EXTI15_10_IRQn);

static IRQn_Type get_pin_irq(uint32_t pin) {
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

static uint32_t get_pin_afio_line(uint32_t pin)
{
    switch(pin % 16) {
        case 0 : return (0x000FU << 16U | 0U);
        case 1 : return (0x00F0U << 16U | 0U);
        case 2 : return (0x0F00U << 16U | 0U);
        case 3 : return (0xF000U << 16U | 0U);
        case 4 : return (0x000FU << 16U | 1U);
        case 5 : return (0x00F0U << 16U | 1U);
        case 6 : return (0x0F00U << 16U | 1U);
        case 7 : return (0xF000U << 16U | 1U);
        case 8 : return (0x000FU << 16U | 2U);
        case 9 : return (0x00F0U << 16U | 2U);
        case 10: return (0x0F00U << 16U | 2U);
        case 11: return (0xF000U << 16U | 2U);
        case 12: return (0x000FU << 16U | 3U);
        case 13: return (0x00F0U << 16U | 3U);
        case 14: return (0x0F00U << 16U | 3U);
        case 15: return (0xF000U << 16U | 3U);
    }
    return 0;
}

void config_external_interrupt(uint32_t pin, ExternalInterruptTriggerMode mode, ExternalInterruptCallback callback, void* data)
{
    uint32_t pin_num = pin % 16;
    uint32_t pin_port = pin / 16;
    uint32_t pin_bit = GPIO2BIT(pin);
    IRQn_Type pin_irq =  get_pin_irq(pin);

    if (mode == ExternalInterruptTriggerModeDisabled)
    {
        irq_disable();

        NVIC_DisableIRQ(pin_irq);

        // disable IT
        CLEAR_BIT(EXTI->IMR, pin_bit);

        irq_callbacks[pin_num] = 0;
        irq_data[pin_num] = 0;

        irq_enable();
        return;
    }

    irq_disable();

    if (irq_callbacks[pin_num] != 0 && irq_callbacks[pin_num] != callback) {
        try_shutdown("External interrupt conflict");
        irq_enable();
        return;
    }

    irq_callbacks[pin_num] = callback;
    irq_data[pin_num] = data;

    // enable AFIO clock
    RCC->APB2ENR |= 1;
    (void)RCC->APB2ENR;

    // link port
    uint32_t afio_line = get_pin_afio_line(pin);
    MODIFY_REG(AFIO->EXTICR[afio_line & 0xFF], (afio_line >> 16), pin_port << POSITION_VAL((afio_line >> 16)));

    // enable IT
    SET_BIT(EXTI->IMR, pin_bit);

    switch(mode) {
        case ExternalInterruptTriggerModeDisabled: break;
        case ExternalInterruptTriggerModeRising:
            SET_BIT(EXTI->RTSR, pin_bit);
            break;
        case ExternalInterruptTriggerModeFalling:
            SET_BIT(EXTI->FTSR, pin_bit);
            break;
        case ExternalInterruptTriggerModeRisingFalling:
            SET_BIT(EXTI->RTSR, pin_bit);
            SET_BIT(EXTI->FTSR, pin_bit);
            break;
    }

    NVIC_EnableIRQ(pin_irq);
    NVIC_SetPriority(pin_irq, 0);

    irq_enable();
    return;
}
