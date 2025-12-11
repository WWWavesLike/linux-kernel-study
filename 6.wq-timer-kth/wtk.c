#include "linux/workqueue_types.h"
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/wait.h>
//TASK_INTERRUPTIBLE
#include <linux/sched.h>
#include <linux/workqueue.h>
#include <linux/timer.h>
#include <linux/jiffies.h>
#include <linux/atomic.h>

//kthread 핸들
static struct task_struct *worker_thread;
//kthread를 재우고 깨우기 위한 waitqueue
static wait_queue_head_t worker_wq;
//0이면 이벤트 없음, 1이면 있음
static atomic_t event_flag;
//몇번째 이벤트인지 카운트
static atomic_t event_count;

//전용 워크큐
static struct workqueue_struct *test_wq;
//실제 work 구조체
static struct work_struct test_work;

//주기적으로 work를 올리기 위한 타이머
static struct timer_list test_timer;

static int worker_thread_fn(void *data);
static void test_work_fn(struct work_struct *work);
static void test_timer_fn(struct timer_list *t);

//모듈 적재
static int __init wtk_init(void)
{
	printk(KERN_INFO "[wtk] module loaded.\n");

	int ret;

	//wait큐, 아토믹 초기화
	init_waitqueue_head(&worker_wq);
	atomic_set(&event_flag, 0);
	atomic_set(&event_count, 0);

	//kthread 생성
	//사용할 함수, 데이터(void *), 이름
	worker_thread = kthread_run(worker_thread_fn, NULL, "wtk_worker");
	if (IS_ERR(worker_thread)) {
		ret = PTR_ERR(worker_thread);
		pr_err("failed to create kthread : %d\n", ret);
		worker_thread = NULL;
		return ret;
	}

	//워크큐 동적 생성
	//3.workqueue 참조.
	test_wq = alloc_workqueue("wtk_wq", WQ_UNBOUND, 0);
	if (!test_wq) {
		pr_err("failed to create workqueue\n");
		kthread_stop(worker_thread);
		worker_thread = NULL;
		return -ENOMEM;
	}

	//work 초기화
	INIT_WORK(&test_work, test_work_fn);
	pr_info("[wtk] workqueue created\n");

	//test_timer(timer_list 구조체) 초기화
	//만료되면 콜백함수를 실행
	timer_setup(&test_timer, test_timer_fn, 0);
	test_timer.expires = jiffies + HZ; //틱주기 : 1000으로 설정되어있음.
	//타이머 등록 1초 후 실행됨
	add_timer(&test_timer);
	pr_info("[wtk] timer started\n");

	return 0;
}

static void test_work_fn(struct work_struct *work)
{
	//event_flag 세팅하고 kthread 깨움
	atomic_set(&event_flag, 1);
	//잠자고 있을 스레드를 깨움.
	wake_up_interruptible(&worker_wq);
}
static void test_timer_fn(struct timer_list *t)
{
	//workqueue에 work를 삽입.
	if (test_wq) {
		//queue_work : work가 이미 올라가 있는지 확인.
		//1이면 work가 큐에 새로 올라감
		//0이면 새로 넣지 못함.
		if (!queue_work(test_wq, &test_work)) {
			pr_debug("[wtk] timer: work already pending\n");
		} else {
			pr_info("[wtk] timer: queued work\n");
		}
	}
	//타이머 재등록
	mod_timer(&test_timer, jiffies + HZ);
}
//모듈 언로드
static void __exit wtk_exit(void)
{
	timer_delete_sync(&test_timer);
	pr_info("[wtk] timer deleted\n");
	//워크큐 정리
	if (test_wq) {
		flush_workqueue(test_wq); //남아있는 워크 모두 처리
		destroy_workqueue(test_wq); // workqueue 구조체 해제
		pr_info("[wtk] workqueue destroyed\n");
	}
	if (worker_thread) {
		//잠자고 있을 스레드를 깨움.
		wake_up_interruptible(&worker_wq);
		//kthread_should_stop을 작동시킴.
		kthread_stop(worker_thread);
		pr_info("[wtk] worker_thread stopped\n");
	}
	printk(KERN_INFO "[wtk] module unloaded.\n");
}

static int worker_thread_fn(void *data)
{
	pr_info("[wtk] worker_thread started\n");

	//unload같은 상황이 발생했을 때 중단.
	while (!kthread_should_stop()) {
		//조건이 true가 될 때까지 수면.
		//TASK_INTERRUPTIBLE 상태(중단 가능)로 수면.
		//event_flag != 0 -> 이벤트 발생
		wait_event_interruptible(worker_wq,
					 atomic_read(&event_flag) != 0 ||
						 kthread_should_stop());
		if (kthread_should_stop())
			break;

		if (atomic_read(&event_flag)) {
			int cnt = atomic_inc_return(&event_count);
			atomic_set(&event_flag, 0);

			pr_info("[wtk] worker_thread : handle event #%d\n",
				cnt);
		}
	}

	pr_info("[wtk] worker_thread stopping\n");
	return 0;
}

module_init(wtk_init);
module_exit(wtk_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("test");
MODULE_DESCRIPTION("workqueue + timer + kthread");
