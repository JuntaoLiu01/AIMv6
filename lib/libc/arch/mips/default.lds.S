
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifndef __LP64__
#define TARGET	"elf32-tradlittlemips"
#else
#define TARGET	"elf64-tradlittlemips"
#endif

#define USER_START	0x400000

OUTPUT_FORMAT(TARGET)

ENTRY(__start)

SECTIONS {
	. = USER_START;
	.text : {
		*(.text*);
	}
	. = ALIGN(32);
	.rodata : {
		*(.rodata*);
	}
	HIDDEN(_gp = ALIGN(16) + 0x7ff0);
	.got : {
		*(.got*);
	}
	.data : {
		*(.data*);
	}
	.bss : {
		*(.bss*);
	}
}

