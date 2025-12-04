#include <linux/module.h>
//캐릭터 디바이스 등록에 필요 라이브러리.
#include <linux/fs.h>
#include <linux/cdev.h>
//사용자 공간 버퍼 접근
#include <linux/uaccess.h>
//태스크릿
#include <linux/interrupt.h>
#include <linux/printk.h>

//major/minor 번호를 담는 커널 타입
static dev_t dev_num;
//char device 객체
static struct cdev my_cdev;

//태스크릿에서 사용할 콜백함수
static void test_tasklet_fn(struct tasklet_struct *t);
//태스크릿 선언 매크로
//6.8부터 변경됨.
/*
#define DECLARE_TASKLET(name, _callback)		\
struct tasklet_struct name = {				\
	.count = ATOMIC_INIT(0),			\
	.callback = _callback,				\
	.use_callback = true,				\
}
*/
static DECLARE_TASKLET(my_tasklet, test_tasklet_fn);

//dev/testirpt에 쓰기를 하면 커널이 VFS(가상파일시스템)을 통해 이 함수로 진입.
//file : 열려있는 파일을 나타내는 커널 객체
//buf : 유저 공간 버퍼
//count : 유저가 쓰려고 한 바이트 수
//ppos : 파일 오프셋 포인터(seek 가능한 일반 파일에서 사용, 보통 안씀)
static ssize_t test_write(struct file *file, const char __user *buf,
			  size_t count, loff_t *ppos)
{
	printk(KERN_INFO "[testirpt] top half triggered. count=%zu\n", count);
	//my_taskelt 예약
	//tasklet은 내부적으로 softirq로 동작함.
	tasklet_schedule(&my_tasklet);
	//count 바이트만큼 성공적으로 썼다고 통보.
	return count;
}

//파일 시스템의 연산자(함수) 테이블.
// write 연산자를 직접 만든 test_write로 지정.
// 쓰기 작업 시 호출됨.
//인자 미지정시 기본 연산자가 적용됨.
static const struct file_operations test_fops = {
	.owner = THIS_MODULE,
	.write = test_write,
};

//후반기 처리 콜백 함수
//softirq 컨텍스트에서 실행됨. 수면상태로 들어가면 절대 안됨.
static void test_tasklet_fn(struct tasklet_struct *t)
{
	printk(KERN_INFO "[testirpt] bottom half executed.\n");
}

//모듈 적재
static int __init testirpt_init(void)
{
	int ret;

	//dev_num의 메이저/마이너 버전 설정
	//2번째 인자 : 시작 마이너 번호
	//3번째 인자 : 드라이버가 관리할 디바이스 수
	ret = alloc_chrdev_region(&dev_num, 0, 1, "testirpt");
	if (ret < 0) {
		printk(KERN_ERR "[myirqdev] alloc_chrdev_region failed: %d\n",
		       ret);
		return ret;
	}

	//my_cdev 구조체에 test_fops를 연결
	cdev_init(&my_cdev, &test_fops);
	my_cdev.owner = THIS_MODULE;

	//my_cdev를 커널에 등록.
	ret = cdev_add(&my_cdev, dev_num, 1);
	if (ret < 0) {
		printk(KERN_ERR "[testirpt] cdev_add failed: %d\n", ret);
		unregister_chrdev_region(dev_num, 1);
		return ret;
	}

	printk(KERN_INFO "[testirpt] module loaded. major=%d minor=%d\n",
	       MAJOR(dev_num), MINOR(dev_num));

	return 0;
}

//모듈 언로드
static void __exit testirpt_exit(void)
{
	//태스크릿 해제
	tasklet_kill(&my_tasklet);
	//cdev 해제
	cdev_del(&my_cdev);
	//메이저/마이너 버전 반납
	unregister_chrdev_region(dev_num, 1);

	printk(KERN_INFO "[testirpt] module unloaded.\n");
}
module_init(testirpt_init);
module_exit(testirpt_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("test");
MODULE_DESCRIPTION("interrupt + tasklet practice (Linux 6.17.9)");
