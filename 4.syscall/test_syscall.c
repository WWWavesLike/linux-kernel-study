#include <linux/kernel.h>
#include <linux/syscalls.h>

//SYSCALL_DEFINE1 : 인자 1개짜리 syscall 생성 매크로
//첫번째 인자 syscall 이름, 두번째 이후는 매개변수 타입과 이름.
SYSCALL_DEFINE1(test_syscall, int, value)
{
	printk(KERN_INFO "test_syscall called with %d\n", value);
	return value + 100;
}
