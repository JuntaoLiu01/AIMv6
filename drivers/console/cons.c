
#include <aim/device.h>
#include <aim/sync.h>
#include <drivers/console/cons.h>
#include <libc/string.h>
#include <libc/stddef.h>
#include <sched.h>
#include <fs/uio.h>

int cons_open(struct chr_device *dev, int mode, struct proc *p)
{
	/* Initialize character buffer */
	memset(dev->cbuf.buf, 0, sizeof(dev->cbuf.buf));
	dev->cbuf.head = dev->cbuf.tail = 0;
	spinlock_init(&dev->cbuf.lock);
	return 0;
}

int cons_close(struct chr_device *dev, int mode, struct proc *p)
{
	/* currently we do nothing */
	return 0;
}

int cons_intr(int irq, struct chr_device *dev,
    unsigned char (*getc)(struct chr_device *))
{
	unsigned char c;
	unsigned long flags;
	struct cbuf *cbuf = &dev->cbuf;

	c = getc(dev);
	/* XXX Currently we are assuming that there is only one
	 * keyboard/console.  Usually, for each console we need to
	 * maintain a separate character buffer (cbuf). */
	spin_lock_irq_save(&cbuf->lock, flags);
	if (cbuf->head == (cbuf->tail + 1) % BUFSIZ) {
		spin_unlock_irq_restore(&cbuf->lock, flags);
		return 0;
	}
	cbuf->buf[cbuf->tail++] = c;
	cbuf->tail %= BUFSIZ;
	wakeup(cbuf);
	spin_unlock_irq_restore(&cbuf->lock, flags);
	return 0;
}

int cons_getc(struct chr_device *dev)
{
	int c;
	unsigned long flags;
	struct cbuf *cbuf = &dev->cbuf;

	spin_lock_irq_save(&cbuf->lock, flags);
	while (cbuf->head == cbuf->tail)
		sleep_with_lock(cbuf, &cbuf->lock);
	c = cbuf->buf[cbuf->head++];
	cbuf->head %= BUFSIZ;
	spin_unlock_irq_restore(&cbuf->lock, flags);
	return c;
}

int cons_putc(struct chr_device *dev, int c,
    int (*putc)(struct chr_device *, unsigned char))
{
	return putc(dev, c);
}

int cons_write(struct chr_device *dev, struct uio *uio, int ioflags,
    int (*putc)(struct chr_device *, unsigned char))
{
	char buf[BUFSIZ];
	size_t len;
	int err, i;

	while (uio->resid > 0) {
		len = min2(uio->resid, BUFSIZ);
		err = uiomove(buf, len, uio);
		if (err)
			return err;
		for (i = 0; i < len; ++i)
			putc(dev, buf[i]);
	}

	return 0;
}

