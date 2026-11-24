ENTRY(Reset_Handler)
MEMORY {
  FLASH (rx) : ORIGIN = 0x08000000, LENGTH = 48K
  RAM (rwx) : ORIGIN = 0x200001C4, LENGTH = 256K - 0x1C4
}
_estack = ORIGIN(RAM) + LENGTH(RAM);
SECTIONS {
  .text : {
    KEEP(*(.isr_vector))
    . = ORIGIN(FLASH) + 512;
    KEEP(*(.boot_abi))
    *(.text*) *(.rodata*)
    . = ALIGN(4);
  } > FLASH
  .ARM.extab : { *(.ARM.extab*) } > FLASH
  .ARM.exidx : { *(.ARM.exidx*) } > FLASH
  .data : {
    . = ALIGN(4); _sdata = .; *(.data*); . = ALIGN(4); _edata = .;
  } > RAM AT> FLASH
  _sidata = LOADADDR(.data);
  .bss (NOLOAD) : {
    . = ALIGN(4); _sbss = .; *(.bss*) *(COMMON); . = ALIGN(4); _ebss = .;
  } > RAM
  end = .; _end = .; __end__ = .;
  /DISCARD/ : { *(.comment*) *(.note*) }
}
ASSERT(_ebss + 8192 <= _estack, "Recovery stack exceeds RAM")
ASSERT(LOADADDR(.data) + SIZEOF(.data) <= 0x0800C000, "Recovery overlaps application")
