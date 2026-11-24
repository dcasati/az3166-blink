#include "stm32f4xx_hal.h"
#include "RecoveryHash.h"

using namespace Recovery;

__attribute__((section(".boot_abi"), used))
const Tag bootTag = BOOT_TAG;

namespace {
QSPI_HandleTypeDef qspi;
volatile uint32_t* const faultLatch = reinterpret_cast<volatile uint32_t*>(0x200001B0);
constexpr uint32_t LATCH_MAGIC = 0x52535450;
bool protectedBoot() { return bootOptionsReady(FLASH->OPTCR); }
void kick() { IWDG->KR = 0xAAAA; }

void consoleInit() {
  RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
  RCC->APB2ENR |= RCC_APB2ENR_USART6EN;
  GPIO_InitTypeDef gpio = {};
  gpio.Pin = GPIO_PIN_11; gpio.Mode = GPIO_MODE_AF_PP;
  gpio.Speed = GPIO_SPEED_HIGH; gpio.Alternate = GPIO_AF8_USART6;
  HAL_GPIO_Init(GPIOA, &gpio);
  USART6->CR1 = 0; USART6->CR2 = 0; USART6->CR3 = 0;
  USART6->BRR = 139;
  USART6->CR1 = USART_CR1_UE | USART_CR1_TE;
}
void print(const char* text) {
  while (*text) {
    unsigned limit = 160000;
    while (!(USART6->SR & USART_SR_TXE) && --limit) {}
    if (!limit) return;
    USART6->DR = uint8_t(*text++);
  }
}
void printHex(uint32_t value) {
  char text[11] = "00000000\r\n";
  for (unsigned i = 0; i < 8; ++i) text[i] = "0123456789ABCDEF"[(value >> (28 - i * 4)) & 15];
  print(text);
}
QSPI_CommandTypeDef command(uint8_t instruction, uint32_t count,
                            bool addressed = false, uint32_t address = 0) {
  QSPI_CommandTypeDef cmd = {};
  cmd.Instruction = instruction;
  cmd.InstructionMode = QSPI_INSTRUCTION_1_LINE;
  cmd.AddressMode = addressed ? QSPI_ADDRESS_1_LINE : QSPI_ADDRESS_NONE;
  cmd.AddressSize = QSPI_ADDRESS_24_BITS;
  cmd.Address = address;
  cmd.DataMode = count ? QSPI_DATA_1_LINE : QSPI_DATA_NONE;
  cmd.NbData = count;
  cmd.SIOOMode = QSPI_SIOO_INST_EVERY_CMD;
  return cmd;
}
bool status(uint8_t& value) {
  QSPI_CommandTypeDef cmd = command(0x05, 1);
  return HAL_QSPI_Command(&qspi, &cmd, 1000) == HAL_OK &&
      HAL_QSPI_Receive(&qspi, &value, 1000) == HAL_OK;
}
bool ready() {
  uint32_t started = HAL_GetTick();
  do {
    uint8_t value;
    if (!status(value)) return false;
    if (!(value & 1)) return true;
  } while (uint32_t(HAL_GetTick() - started) < 5000);
  return false;
}
bool writeEnable() {
  if (!ready()) return false;
  QSPI_CommandTypeDef cmd = command(0x06, 0);
  uint8_t value;
  return HAL_QSPI_Command(&qspi, &cmd, 1000) == HAL_OK &&
      status(value) && (value & 2);
}
bool flashInit() {
  RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN | RCC_AHB1ENR_GPIOBEN | RCC_AHB1ENR_GPIOCEN;
  RCC->AHB3ENR |= RCC_AHB3ENR_QSPIEN;
  RCC->AHB3RSTR |= RCC_AHB3RSTR_QSPIRST;
  RCC->AHB3RSTR &= ~RCC_AHB3RSTR_QSPIRST;
  GPIO_InitTypeDef gpio = {};
  gpio.Mode = GPIO_MODE_AF_PP; gpio.Speed = GPIO_SPEED_HIGH;
  gpio.Pin = GPIO_PIN_1; gpio.Alternate = GPIO_AF9_QSPI;
  HAL_GPIO_Init(GPIOB, &gpio);
  gpio.Pin = GPIO_PIN_11; gpio.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOC, &gpio);
  gpio.Pull = GPIO_NOPULL; gpio.Alternate = GPIO_AF10_QSPI;
  gpio.Pin = GPIO_PIN_6 | GPIO_PIN_7;
  HAL_GPIO_Init(GPIOA, &gpio);
  // Single-bit SPI keeps the unused WP#/HOLD# signals deasserted.
  GPIOC->BSRR = GPIO_PIN_4 | GPIO_PIN_5;
  gpio.Mode = GPIO_MODE_OUTPUT_PP;
  gpio.Pin = GPIO_PIN_4 | GPIO_PIN_5;
  HAL_GPIO_Init(GPIOC, &gpio);
  qspi.Instance = QUADSPI;
  qspi.Init.ClockPrescaler = 3;
  qspi.Init.FifoThreshold = 4;
  qspi.Init.SampleShifting = QSPI_SAMPLE_SHIFTING_NONE;
  qspi.Init.FlashSize = 20;
  qspi.Init.ChipSelectHighTime = QSPI_CS_HIGH_TIME_4_CYCLE;
  qspi.Init.ClockMode = QSPI_CLOCK_MODE_0;
  // AZ3166 data and chip-select pins belong to QSPI bank 2.
  qspi.Init.FlashID = QSPI_FLASH_ID_2;
  qspi.Init.DualFlash = QSPI_DUALFLASH_DISABLE;
  if (HAL_QSPI_Init(&qspi) != HAL_OK) return false;
  QSPI_CommandTypeDef wake = command(0xAB, 0);
  if (HAL_QSPI_Command(&qspi, &wake, 1000) != HAL_OK) return false;
  uint32_t started = HAL_GetTick();
  while (uint32_t(HAL_GetTick() - started) < 1) {}
  if (!ready()) return false;
  uint8_t id[3];
  QSPI_CommandTypeDef cmd = command(0x9F, sizeof(id));
  if (HAL_QSPI_Command(&qspi, &cmd, 1000) != HAL_OK ||
      HAL_QSPI_Receive(&qspi, id, 1000) != HAL_OK) return false;
  uint32_t jedec = uint32_t(id[0]) << 16 | uint32_t(id[1]) << 8 | id[2];
  print("RECOVERY JEDEC "); printHex(jedec);
  // Reject unknown command sets or densities rather than guessing geometry.
  uint8_t protection;
  return (jedec == 0xC22015 || jedec == 0xC22315 || jedec == 0xC22815) &&
      status(protection) && !(protection & 0x3C);
}

uint32_t base(Region region) {
  return region == Region::Staging ? STAGING_BASE :
         region == Region::Fallback ? FALLBACK_BASE : JOURNAL_BASE;
}
struct Ops {
  void progress() { kick(); }
  bool read(Region region, uint32_t offset, void* output, uint32_t size) {
    if (!bounds(region, offset, size)) return false;
    if (region == Region::Internal) {
      memcpy(output, reinterpret_cast<const void*>(APP_BASE + offset), size);
      return true;
    }
    if (!size) return true;
    QSPI_CommandTypeDef cmd = command(0x03, size, true, base(region) + offset);
    return HAL_QSPI_Command(&qspi, &cmd, 1000) == HAL_OK &&
        HAL_QSPI_Receive(&qspi, static_cast<uint8_t*>(output), 1000) == HAL_OK;
  }
  bool write(Region region, uint32_t offset, const void* input, uint32_t size) {
    if (!bounds(region, offset, size) || region == Region::Staging) return false;
    const uint8_t* bytes = static_cast<const uint8_t*>(input);
    if (region == Region::Internal) {
      if (!protectedBoot() || (offset & 3) || (size & 3) ||
          HAL_FLASH_Unlock() != HAL_OK) return false;
      bool ok = true;
      for (uint32_t i = 0; ok && i < size; i += 4) {
        uint32_t word;
        memcpy(&word, bytes + i, sizeof(word));
        ok = HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, APP_BASE + offset + i, word) == HAL_OK;
        if (ok) progress();
      }
      return HAL_FLASH_Lock() == HAL_OK && ok;
    }
    while (size) {
      uint32_t count = 256 - ((base(region) + offset) & 255);
      if (count > size) count = size;
      if (!writeEnable()) return false;
      QSPI_CommandTypeDef cmd = command(0x02, count, true, base(region) + offset);
      if (HAL_QSPI_Command(&qspi, &cmd, 1000) != HAL_OK ||
          HAL_QSPI_Transmit(&qspi, const_cast<uint8_t*>(bytes), 1000) != HAL_OK ||
          !ready()) return false;
      size -= count; offset += count; bytes += count;
      progress();
    }
    return true;
  }
  bool erase(Region region, uint32_t offset, uint32_t size) {
    if (!bounds(region, offset, size) || region == Region::Staging) return false;
    if (region == Region::Internal) {
      if (offset || size != APP_SIZE || !protectedBoot() ||
          HAL_FLASH_Unlock() != HAL_OK) return false;
      bool ok = true;
      for (uint32_t sector = 3; ok && sector <= 11; ++sector) {
        FLASH_EraseInitTypeDef erase = {};
        erase.TypeErase = FLASH_TYPEERASE_SECTORS;
        erase.Sector = sector; erase.NbSectors = 1;
        erase.VoltageRange = FLASH_VOLTAGE_RANGE_3;
        uint32_t failed;
        ok = HAL_FLASHEx_Erase(&erase, &failed) == HAL_OK;
        if (ok) progress();
      }
      return HAL_FLASH_Lock() == HAL_OK && ok;
    }
    if ((offset % SECTOR) || (size % SECTOR)) return false;
    for (uint32_t end = offset + size; offset < end; offset += SECTOR) {
      if (!writeEnable()) return false;
      QSPI_CommandTypeDef cmd = command(0x20, 0, true, base(region) + offset);
      if (HAL_QSPI_Command(&qspi, &cmd, 1000) != HAL_OK || !ready()) return false;
      progress();
    }
    return true;
  }
  bool hash(Region region, uint32_t size, uint8_t output[32]) {
    return Recovery::hash(*this, region, size, output);
  }
};

[[noreturn]] void jump() {
  const uint32_t* vector = reinterpret_cast<const uint32_t*>(APP_BASE);
  uint32_t stack = vector[0], entry = vector[1];
  __disable_irq();
  SysTick->CTRL = 0; SysTick->LOAD = 0; SysTick->VAL = 0;
  SCB->ICSR = SCB_ICSR_PENDSTCLR_Msk | SCB_ICSR_PENDSVCLR_Msk;
  for (unsigned i = 0; i < 8; ++i) {
    NVIC->ICER[i] = 0xFFFFFFFF; NVIC->ICPR[i] = 0xFFFFFFFF;
  }
  SCB->VTOR = APP_BASE;
  __set_CONTROL(0); __set_BASEPRI(0); __set_FAULTMASK(0);
  __DSB(); __ISB();
  kick();
  __asm volatile("msr msp, %0\ncpsie i\nbx %1" :: "r"(stack), "r"(entry) : "memory");
  __builtin_unreachable();
}
} // namespace

extern "C" [[noreturn]] void boot_fatal(uint32_t error) {
  faultLatch[1] = error;
  faultLatch[2] = ~error;
  faultLatch[3] = ~LATCH_MAGIC;
  faultLatch[0] = LATCH_MAGIC;
  __DSB();
  print("RECOVERY STOP "); printHex(error);
  for (;;) __WFI();
}

extern "C" void boot_main() {
  consoleInit();
  print("\r\nAZ3166 source recovery ABI 1\r\n");
  if ((RCC->CSR & RCC_CSR_WDGRSTF) && faultLatch[0] == LATCH_MAGIC &&
      faultLatch[1] == ~faultLatch[2] && faultLatch[3] == ~LATCH_MAGIC) {
    print("RECOVERY latched fault; ST-Link inspection required ");
    printHex(faultLatch[1]);
    for (;;) __WFI();
  }
  faultLatch[0] = 0;
  RCC->CSR |= RCC_CSR_RMVF;
  IWDG_HandleTypeDef watchdog = {};
  watchdog.Instance = IWDG;
  watchdog.Init.Prescaler = IWDG_PRESCALER_256;
  watchdog.Init.Reload = 4095;
  if (HAL_IWDG_Init(&watchdog) != HAL_OK) boot_fatal(0xE010);
  if (!protectedBoot()) boot_fatal(0xE011);
  if (!flashInit()) boot_fatal(0xE012);
  Ops ops;
  Engine<Ops> engine(ops);
  Boot result = engine.boot();
  if (result == Boot::Stop) boot_fatal(0xE100 + uint32_t(engine.error()));
  print(result == Boot::Trial ? "RECOVERY trial jump\r\n" : "RECOVERY confirmed jump\r\n");
  jump();
}
