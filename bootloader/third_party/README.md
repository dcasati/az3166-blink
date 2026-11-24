# Recovery build dependencies

The `source-` prefix does not indicate modifications. These source files are
copied unchanged from the following tagged upstream releases:

- `source-stm32f4xx_hal_{flash,flash_ex,gpio,qspi,dma,iwdg}.c`:
  [ST HAL V1.5.0 in mbed-os-5.3.0](https://github.com/ARMmbed/mbed-os/tree/mbed-os-5.3.0/targets/TARGET_STM/TARGET_STM32F4/device).
  Each file retains its ST copyright and three-condition BSD-style license.
- `source-sha256.c`, `source-platform_util.c`:
  [mbedtls-2.15.0](https://github.com/Mbed-TLS/mbedtls/tree/mbedtls-2.15.0).
  Apache-2.0; see `mbedtls-LICENSE.txt` and retained notices.

The compatible `mbedtls/*.h` headers were copied unchanged from the installed
`framework-arduinostm32mxchip` 2.0.0 package, retaining their Apache-2.0 notices.

The HAL uses the existing `framework-arduinostm32mxchip` target/CMSIS headers
and GCC 6.3.1 runtime. The recovery image does **not** link the vendor SDK archive,
RTOS, WiFi, Azure, allocation, or option-byte programming routines.
`build.py` records source SHA-256 values with each build.
