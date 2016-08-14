
#include <pipe.h>
#include <vmm.h>
#include <pmm.h>
#include <panic.h>
#include <aim/sync.h>
#include <sched.h>
#include <file.h>
#include <asm-generic/funcs.h>
#include <fcntl.h>
#include <percpu.h>
#include <proc.h>
#include <errno.h>

struct pipe *
PIPENEW(void)
{
	struct pipe *pipe;
	struct pages pages = {
		.size = ALIGN_ABOVE(PIPE_SIZE, PAGE_SIZE),
		.flags = 0,
	};
	if ((pipe = kmalloc(sizeof(*pipe), GFP_ZERO)) == NULL)
		return NULL;
	if (alloc_pages(&pages) < 0) {
		kfree(pipe);
		return NULL;
	}

	spinlock_init(&pipe->lock);
	pipe->data = pa2kva(pages.paddr);
	pipe->nread = pipe->nwritten = 0;
	pipe->nreaders = pipe->nwriters = 1;

	return pipe;
}

void
pipe_free(struct pipe *pipe)
{
	struct pages pages;

	pages.paddr = kva2pa(pipe->data);
	pages.size = ALIGN_ABOVE(PIPE_SIZE, PAGE_SIZE);
	pages.flags = 0;

	free_pages(&pages);
	kfree(pipe);
}

int
pipe_close(struct file *file, struct proc *p)
{
	struct pipe *pipe = file->pipe;
	assert(file->type == FPIPE);
	spin_lock(&pipe->lock);

	if (file->openflags & FWRITE) {
		if (--pipe->nwriters == 0)
			wakeup(&pipe->nread);
	}
	if (file->openflags & FREAD) {
		if (--pipe->nreaders == 0)
			wakeup(&pipe->nwritten);
	}
	if (pipe->nreaders == 0 && pipe->nwriters == 0){
		spin_unlock(&pipe->lock);
		pipe_free(pipe);
	} else {
		spin_unlock(&pipe->lock);
	}

	return 0;
}

void
pipe_ref(struct file *file)
{
	assert(file->type == FPIPE);
	if (file->openflags & FREAD)
		file->pipe->nreaders++;
	if (file->openflags & FWRITE)
		file->pipe->nwriters++;
}

int
pipe_read(struct file *file, void *buf, size_t len, loff_t *off)
{
	struct pipe *pipe = file->pipe;
	unsigned long flags;
	size_t waitfor;
	char *data = pipe->data;
	assert(file->type == FPIPE);

	spin_lock_irq_save(&pipe->lock, flags);
	for (; len > 0; ) {
		waitfor = min2(len, PIPE_SIZE);
		while (pipe->nwritten - pipe->nread < waitfor &&
		    pipe->nwriters > 0) {
			if (current_proc->flags & PF_SIGNALED) {
				spin_unlock_irq_restore(&pipe->lock, flags);
				return -EINTR;
			}
			sleep_with_lock(&pipe->nread, &pipe->lock);
		}
		for (int i = 0; i < waitfor; ++i)
			*(char *)(buf++) = data[pipe->nread++ % PIPE_SIZE];
		len -= waitfor;
		*off += waitfor;
		wakeup(&pipe->nwritten);
	}
	spin_unlock_irq_restore(&pipe->lock, flags);
	return 0;
}

int
pipe_write(struct file *file, void *buf, size_t len, loff_t *off)
{
	struct pipe *pipe = file->pipe;
	unsigned long flags;
	size_t waitfor;
	char *data = pipe->data;
	assert(file->type == FPIPE);

	spin_lock_irq_save(&pipe->lock, flags);
	for (; len > 0; ) {
		waitfor = min2(len, PIPE_SIZE);
		while (pipe->nwritten - pipe->nread > PIPE_SIZE - waitfor) {
			if ((current_proc->flags & PF_SIGNALED) ||
			    pipe->nreaders == 0) {
				/*
				 * As per POSIX, we need to send a SIGPIPE
				 * signal prior to returning error code
				 * EPIPE.
				 */
				spin_unlock_irq_restore(&pipe->lock, flags);
				return -EPIPE;
			}
			sleep_with_lock(&pipe->nwritten, &pipe->lock);
		}
		for (int i = 0; i < waitfor; ++i)
			data[pipe->nwritten++ % PIPE_SIZE] = *(char *)(buf++);
		len -= waitfor;
		*off += waitfor;
		wakeup(&pipe->nread);
	}
	spin_unlock_irq_restore(&pipe->lock, flags);
	return 0;
}

struct file_ops pipeops = {
	.close = pipe_close,
	.read = pipe_read,
	.write = pipe_write,
	.ref = pipe_ref,
};
