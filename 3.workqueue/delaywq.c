#include <linux/module.h>
//캐릭터 디바이스 등록에 필요 라이브러리.
#include <linux/fs.h>
#include <linux/cdev.h>
//사용자 공간 버퍼 접근
#include <linux/uaccess.h>
#include <linux/workqueue.h>
#include <linux/printk.h>
#include <linux/delay.h>
//major/minor 번호를 담는 커널 타입
static dev_t dev_num;
//char device 객체
static struct cdev my_cdev;
// work 콜백 함수 선언
static void test_workq_fn(struct work_struct *work);

static struct workqueue_struct *twq;
static struct delayed_work test_dwq;

static DECLARE_WORK(test_workq, test_workq_fn);

static void test_workq_fn(struct work_struct *work)
{
	/*
    #define pr_info(fmt, ...) \
	    printk(KERN_INFO pr_fmt(fmt), ##__VA_ARGS__)
    */
	pr_info("[delay-workqueue] test_workq_fn start (jiffies=%lu) \n",
		jiffies);
	pr_info("[delay-workqueue] test_workq_fn end \n");
}

//사용자 정의 write 함수.
static ssize_t test_write(struct file *file, const char __user *buf,
			  size_t count, loff_t *ppos)
{
	unsigned long delay = msecs_to_jiffies(3000);
	printk(KERN_INFO "[testirpt] top half triggered. count=%zu\n", delay);

	//delayed_wq에 work 삽입
	queue_delayed_work(twq, &test_dwq, delay);
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
//모듈 적재
static int __init testirpt_init(void)
{
	int ret;

	INIT_DELAYED_WORK(&test_dwq, test_workq_fn);
	//전용 wq 생성
	//WQ_UNBOUND : CPU에 묶지 않고 적당한 CPU에서 처리(지역성 희생, 확장성)
	//WQ_MEM_RECLAIM : 최소 1개의 워커를 보장.
	//  메모리 회수 경로에서 사용될 수 있는 WQ라면 무조건 붙어야함.
	//한 CPU 또는 풀에서 동시에 돌아갈 work 개수 상한.
	twq = alloc_workqueue("twq", WQ_UNBOUND | WQ_MEM_RECLAIM, 1);

	if (!twq)
		return -ENOMEM;

	//dev_num의 메이저/마이너 버전 설정
	ret = alloc_chrdev_region(&dev_num, 0, 1, "testirpt");
	if (ret < 0) {
		printk(KERN_ERR "[testirpt] alloc_chrdev_region failed: %d\n",
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
	// 지연 work 취소, 실행 중이면 끝날 때까지 대기.
	cancel_delayed_work_sync(&test_dwq);
	//워크큐 제거.
	destroy_workqueue(twq);

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
MODULE_DESCRIPTION("interrupt + workqueue practice (Linux 6.17.9)");
