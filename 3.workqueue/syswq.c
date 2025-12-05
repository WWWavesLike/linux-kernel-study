#include "linux/mutex.h"
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

//work_struct 정의 & 초기화 (정적)
/*
struct work_struct {
	atomic_long_t data;
	struct list_head entry;
	work_func_t func;
#ifdef CONFIG_LOCKDEP
	struct lockdep_map lockdep_map;
#endif
};
*/
static DECLARE_WORK(test_workq, test_workq_fn);

static DEFINE_MUTEX(test_lock);

static void test_workq_fn(struct work_struct *work)
{
	/*
    #define pr_info(fmt, ...) \
	    printk(KERN_INFO pr_fmt(fmt), ##__VA_ARGS__)
    */
	pr_info("[workqueue] test_workq_fn start (current=%s pid=%d) \n",
		current->comm, current->pid);

	//워크큐는 프로세스 컨텍스트에서 실행되므로 수면 가능함.
	msleep(1000);

	//뮤택스 역시 가능함.
	mutex_lock(&test_lock);
	pr_info("[workqueue] LOCK!!! \n");
	mutex_unlock(&test_lock);

	pr_info("[workqueue] test_workq_fn end \n");
}

//사용자 정의 write 함수.
static ssize_t test_write(struct file *file, const char __user *buf,
			  size_t count, loff_t *ppos)
{
	printk(KERN_INFO "[testirpt] top half triggered. count=%zu\n", count);

	//system_wq에 work를 넣는 헬퍼 함수.
	/*
    static inline bool schedule_work(struct work_struct *work)
    {
	    return queue_work(system_wq, work);
    }
    */
	schedule_work(&test_workq);
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
	mutex_init(&test_lock);
	//dev_num의 메이저/마이너 버전 설정
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
	cancel_work_sync(&test_workq);

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
