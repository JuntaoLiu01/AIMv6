
#ifndef _DRIVERS_CONSOLE_CONS_H
#define _DRIVERS_CONSOLE_CONS_H

#include <aim/device.h>
#include <proc.h>

int cons_open(struct chr_device *, int, struct proc *);
int cons_close(struct chr_device *, int, struct proc *);
int cons_intr(int, struct chr_device *, unsigned char (*)(struct chr_device *));
int cons_getc(struct chr_device *);
int cons_putc(struct chr_device *, int,
    int (*)(struct chr_device *, unsigned char));
int cons_write(struct chr_device *, struct uio *, int,
    int (*)(struct chr_device *, unsigned char));

#endif
