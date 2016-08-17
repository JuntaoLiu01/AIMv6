
/*
 * This is the REAL entry for all user programs.
 * It invokes the user-defined main() function, and calls exit(2) with the
 * return code.
 */
void __start(int argc, char *argv[], char *envp[])
{
	int retcode;

	extern int main(int argc, char *argv[], char *envp[]);
	retcode = main(argc, argv, envp);
	/* exit(2) is not implemented yet */
	for (;;)
		/* nothing */;
}
