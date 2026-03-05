/* MatryoshkaOS Kernel Linker Script */
/* x86_64 ELF64 format */

ENTRY(_start)

SECTIONS
{
    /* Kernel starts at 1MB mark (standard for x86_64) */
    . = 1M;

    /* Multiboot2 header must be in first 32KB */
    .multiboot ALIGN(8) : {
        *(.multiboot)
    }

    /* Boot code immediately after multiboot header */
    .text.boot ALIGN(16) : {
        *(.text.boot)     /* Entry point (_start) */
    }

    /* Rest of code sections */
    .text ALIGN(4K) : {
        *(.text)          /* All other code */
    }

    .rodata ALIGN(4K) : {
        *(.rodata)        /* Read-only data */
        *(.rodata.*)
    }

    /* Read-write sections */
    .data ALIGN(4K) : {
        *(.data)          /* Initialized data */
    }

    .bss ALIGN(4K) : {
        *(COMMON)
        *(.bss)           /* Uninitialized data */
        *(.bss.*)
    }

    /* End of kernel marker */
    . = ALIGN(4K);
    _kernel_end = .;

    /* Stack section */
    .stack ALIGN(4K) : {
        . = . + 16K;      /* 16KB initial stack */
        _stack_top = .;
    }

    /* Discard unwanted sections */
    /DISCARD/ : {
        *(.comment)
        *(.eh_frame)
        *(.note.*)
    }
}

