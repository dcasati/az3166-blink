#include <stdint.h>
#include "stm32f4xx.h"

extern uint32_t _sidata, _sdata, _edata, _sbss, _ebss, _estack;
extern void boot_main(void);
extern void boot_fatal(uint32_t error);
void Reset_Handler(void);
static volatile uint32_t ticks;
uint32_t SystemCoreClock = 16000000;
uint32_t HAL_GetTick(void) { return ticks; }
static void systick(void) { ++ticks; }
static void fault(void) { boot_fatal(0xE001); }

__attribute__((section(".isr_vector"), aligned(512), used))
const uintptr_t vectors[128] = {
  [0] = (uintptr_t)&_estack,
  [1] = (uintptr_t)Reset_Handler,
  [2 ... 14] = (uintptr_t)fault,
  [15] = (uintptr_t)systick,
  [16 ... 127] = (uintptr_t)fault
};

void Reset_Handler(void) {
  __disable_irq();
  uint32_t* source = &_sidata;
  for (uint32_t* p = &_sdata; p < &_edata;) *p++ = *source++;
  for (uint32_t* p = &_sbss; p < &_ebss;) *p++ = 0;
  SCB->CPACR |= 0xFu << 20;
  __DSB(); __ISB();
  RCC->CR |= RCC_CR_HSION;
  unsigned remaining = 1000000;
  while (!(RCC->CR & RCC_CR_HSIRDY) && --remaining) {}
  if (!remaining) boot_fatal(0xE002);
  RCC->CFGR = 0;
  remaining = 1000000;
  while ((RCC->CFGR & RCC_CFGR_SWS) && --remaining) {}
  if (!remaining) boot_fatal(0xE003);
  RCC->CR &= ~(RCC_CR_PLLON | RCC_CR_HSEON | RCC_CR_CSSON);
  SCB->VTOR = (uint32_t)vectors;
  SysTick->LOAD = 16000 - 1;
  SysTick->VAL = 0;
  SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_TICKINT_Msk | SysTick_CTRL_ENABLE_Msk;
  __enable_irq();
  boot_main();
  boot_fatal(0xE004);
}
