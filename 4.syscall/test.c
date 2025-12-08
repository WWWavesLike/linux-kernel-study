#define _GNU_SOURCE
#include <unistd.h>
#include <sys/syscall.h>
#include <stdio.h>

#ifndef __NR_test_syscall
#define __NR_test_syscall 548
#endif

int main(void)
{
	long foo = syscall(__NR_test_syscall, 555, 548);

	printf("syscall return : foo(%ld)\n", foo);
	return 0;
}
