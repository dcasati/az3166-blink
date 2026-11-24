MEMORY {
  FLASH (rx) : ORIGIN = 0x0800C000, LENGTH = 0x98000
  RAM (rwx) : ORIGIN = 0x200001C4, LENGTH = 256K - 0x1C4
}
ENTRY(Reset_Handler)
SECTIONS {
  .text : {
    KEEP(*(.isr_vector))
    . = ORIGIN(FLASH) + 512;
    KEEP(*(.recovery_abi))
    *(.text*)
    KEEP(*(.init)) KEEP(*(.fini))
    *crtbegin.o(.ctors) *crtbegin?.o(.ctors)
    *(EXCLUDE_FILE(*crtend?.o *crtend.o) .ctors)
    *(SORT(.ctors.*)) *(.ctors)
    *crtbegin.o(.dtors) *crtbegin?.o(.dtors)
    *(EXCLUDE_FILE(*crtend?.o *crtend.o) .dtors)
    *(SORT(.dtors.*)) *(.dtors)
    *(.rodata*)
    __start_riot_core = .; *(.riot_core) __stop_riot_core = .;
    __start_riot_fw = .; *(.riot_fw) __stop_riot_fw = .;
    KEEP(*(.eh_frame*))
  } > FLASH
  .ARM.extab : { *(.ARM.extab* .gnu.linkonce.armextab.*) } > FLASH
  __exidx_start = .;
  .ARM.exidx : { *(.ARM.exidx* .gnu.linkonce.armexidx.*) } > FLASH
  __exidx_end = .;
  __etext = .; _sidata = .;
  .data : AT (__etext) {
    __data_start__ = .; _sdata = .;
    *(vtable) *(.data*)
    . = ALIGN(4);
    PROVIDE_HIDDEN (__preinit_array_start = .);
    KEEP(*(.preinit_array)) PROVIDE_HIDDEN (__preinit_array_end = .);
    . = ALIGN(4);
    PROVIDE_HIDDEN (__init_array_start = .);
    KEEP(*(SORT(.init_array.*))) KEEP(*(.init_array))
    PROVIDE_HIDDEN (__init_array_end = .);
    . = ALIGN(4);
    PROVIDE_HIDDEN (__fini_array_start = .);
    KEEP(*(SORT(.fini_array.*))) KEEP(*(.fini_array))
    PROVIDE_HIDDEN (__fini_array_end = .);
    KEEP(*(.jcr*)) . = ALIGN(4);
    __data_end__ = .; _edata = .;
  } > RAM
  __recovery_image_end = LOADADDR(.data) + SIZEOF(.data);
  .bss : {
    . = ALIGN(4); __bss_start__ = .; _sbss = .;
    *(.bss*) *(COMMON) . = ALIGN(4); __bss_end__ = .; _ebss = .;
  } > RAM
  .heap (COPY) : { __end__ = .; end = .; *(.heap*) __HeapLimit = .; } > RAM
  .stack_dummy (COPY) : { *(.stack*) } > RAM
  __StackTop = ORIGIN(RAM) + LENGTH(RAM); _estack = __StackTop;
  __StackLimit = __StackTop - SIZEOF(.stack_dummy);
  PROVIDE(__stack = __StackTop);
  ASSERT(__StackLimit >= __HeapLimit, "region RAM overflowed with stack")
  ASSERT(__recovery_image_end <= ORIGIN(FLASH) + LENGTH(FLASH), "Image exceeds fallback capacity")
}
