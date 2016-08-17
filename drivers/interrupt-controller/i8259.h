
#ifndef _DRIVERS_INTRCTRL_I8259_H
#define _DRIVERS_INTRCTRL_I8259_H

#define PIC_MASTER_CMD		0x20
#define PIC_MASTER_IMR		0x21
#define PIC_MASTER_ISR		PIC_MASTER_CMD
#define PIC_MASTER_POLL		PIC_MASTER_ISR
#define PIC_MASTER_OCW3		PIC_MASTER_ISR
#define PIC_SLAVE_CMD		0xa0
#define PIC_SLAVE_IMR		0xa1

#define PIC_CASCADE_IR		2
#define MASTER_ICW4_DEFAULT	0x01
#define SLAVE_ICW4_DEFAULT	0x01
#define PIC_ICW4_AEOI		2

#define PIC_SPEC_EOI		0x60

void i8259_init(bool auto_eoi);
void i8259_eoi(int irq);

#endif
