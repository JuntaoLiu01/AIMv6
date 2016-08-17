
#include <drivers/hd/hd.h>
#include <util.h>

static int __register_func_index = 0;
static int (*__regfuncs[MAX_PARTITION_TABLE_TYPES])(struct hd_device *) = {0};

int register_partition_table(int (*register_func)(struct hd_device *))
{
	__regfuncs[__register_func_index++] = register_func;
	return 0;
}

int detect_hd_partitions(struct hd_device *dev)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(__regfuncs); ++i) {
		if (__regfuncs[i] == NULL)
			continue;
		if (__regfuncs[i](dev) == 0)
			return 0;
	}
	panic("No partition table found\n");
	/* NOTREACHED */
	return -1;
}

