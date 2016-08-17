
/*
 * Temporary trap test, will be removed.
 */

#include <aim/console.h>
#include <panic.h>

void empty_routine(void)
{
}

static void __test_skip(void)
{
	int a = 2, b = 2, c = -2, d;
	kprintf("**********test_skip**********\n");
	kprintf("Direct breakpoint\n");
	asm volatile (
		"break	0"
	);
	kprintf("Jump\n");
	asm volatile (
		"	.set	push;"
		"	.set	noreorder;"
		"	j	1f;"
		"	break	0;"
		"1:	.set	pop"
	);
	kprintf("Jump Register\n");
	asm volatile (
		"	.set	push;"
		"	.set	noreorder;"
		"	la	$8, 1f;"
		"	jr	$8;"
		"	break	0;"
		"1:	.set	pop;"
		: /* no output */ : /* no input */ : "$8"
	);
	/* should enter "empty_routine" once */
	kprintf("Jump And Link\n");
	asm volatile (
		".set	push;"
		".set	noreorder;"
		"jal	empty_routine;"
		"break	0;"
		".set	pop;"
	);
	/* should enter "empty_routine" once */
	kprintf("Jump And Link Register\n");
	asm volatile (
		".set	push;"
		".set	noreorder;"
		"la	$8, empty_routine;"
		"jalr	$8;"
		"break	0;"
		".set	pop;"
		: /* no output */ : /* no input */ : "$8"
	);
	kprintf("Branch if Equal (Success)\n");
	asm volatile (
		"	.set	push;"
		"	.set	noreorder;"
		"	li	%0, 1;"
		"	beq	%1, %2, 1f;"
		"	break	0;"
		"	move	%0, $0;"
		"1:	.set	pop"
		: "+r"(d) : "r"(a), "r"(b)
	);
	assert(d);
	kprintf("Branch if Equal (Fail)\n");
	asm volatile (
		"	.set	push;"
		"	.set	noreorder;"
		"	move	%0, $0;"
		"	beq	%1, %2, 1f;"
		"	break	0;"
		"	li	%0, 1;"
		"1:	.set	pop"
		: "+r"(d) : "r"(a), "r"(c)
	);
	assert(d);
	/* should enter "empty_routine" once */
	kprintf("Branch if >= 0 And Link (Success)\n");
	asm volatile (
		".set	push;"
		".set	noreorder;"
		"bgezal	%0, empty_routine;"
		"break	0;"
		".set	pop"
		: /* no output */ : "r"(a)
	);
	/* should enter "empty_routine" once */
	kprintf("Branch if >= 0 And Link (Fail)\n");
	asm volatile (
		".set	push;"
		".set	noreorder;"
		"bgezal	%0, empty_routine;"
		"break	0;"
		".set	pop"
		: /* no output */ : "r"(c)
	);
	/* should trigger the exception twice */
	kprintf("Branch back if not equal (Success and fail)\n");
	asm volatile (
		"	.set	push;"
		"	.set	noreorder;"
		"	b	2f;"
		"	move	$8, $0;"
		"1:	li	$8, 1;"
		"2:	beqz	$8, 1b;"
		"	break	0;"
		"	.set	pop"
		: /* no output */ : /* no input */ : "$8"
	);
	kprintf("*******test_skip succeeded********\n");
}

void trap_test(void)
{
#if 0
	__test_skip();
#endif
}

