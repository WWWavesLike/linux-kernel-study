#include <linux/printk.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
//proc_fs.h : /proc 파일 시스템 관련 API
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
//sysfs용 kobject, attribute 관련 API
#include <linux/kobject.h>
#include <linux/sysfs.h>

//파일 이름
#define PROC_NAME "test"
#define PROC_BUF_SZ 128

static struct proc_dir_entry *proc_entry;
//유저가 쓴 /proc/test/에 쓴 문자열을 저장하는 버퍼
static char proc_buf[PROC_BUF_SZ];
//유효한 문자열 길이
static size_t proc_len;
//sys/kernel/test_kobj에 해당하는 kobj 포인터
static struct kobject *test_kobj;
//sys/kernel/test_kobj/value에 연결된 int 값.
static int sysfs_value = 0;

//read 연산.
//기본 파일 시스템의 read 대신 작동한다.
// *ppos : 파일의 현재 오프셋
static ssize_t test_read(struct file *file, char __user *ubuf, size_t count,
			 loff_t *ppos)
{
	int ret;

	//procfs는 반복 호출될 수 있다. (cat 같은걸로)
	//한번도 안 읽었다면 (ppos가 0이면) 데이터 보내기

	//이미 한 번 읽었으면 EOF
	if (*ppos > 0 || proc_len == 0)
		return 0;

	//유저가 준 버퍼 크기가 proc_len보다 작으면 에러.
	//한 번에 전체를 다 읽어야함.
	if (count < proc_len)
		return -EINVAL;
	//커널 버퍼 -> 유저 버퍼 복사
	//실패시 에러처리.
	if (copy_to_user(ubuf, proc_buf, proc_len))
		return -EFAULT;

	//*ppos를 proc_len만큼 뒤로 밀어놓음.
	//다음 read에서 *ppos > 0 조건이 걸려 EOF.
	//실제 유저에게 전달한 바이트 수를 리턴
	*ppos = proc_len;
	ret = proc_len;

	pr_info("proc_sysfs : /proc/%s read %zu bytes\n", PROC_NAME, proc_len);
	return ret;
}

//쓰기 연산 정의.
//사용자가 쓰기 연산 호출시 해당 함수가 작동함.
static ssize_t test_write(struct file *file, const char __user *ubuf,
			  size_t count, loff_t *ppos)
{
	size_t len;
	//범위를 넘어갈 경우를 방지.
	if (count >= PROC_BUF_SZ)
		len = PROC_BUF_SZ - 1;
	else
		len = count;

	//이전 내용이 들어가 있을 수 있으므로 비움.
	memset(proc_buf, 0, PROC_BUF_SZ);

	//유저 버퍼에서 커널 버퍼로 데이터 복사.
	if (copy_from_user(proc_buf, ubuf, len))
		return -EFAULT;

	//개행 정리. echo로 문자열 넘길 때 실제론 개행 문자도 딸려들어옴.
	if (len > 0 && proc_buf[len - 1] == '\n')
		proc_buf[len - 1] = '\0';

	//실제 문자열 길이를 계산하여 저장.
	proc_len = strnlen(proc_buf, PROC_BUF_SZ);

	pr_info("myproc_sysfs: /proc/%s write: \"%s\" (len=%zu)\n", PROC_NAME,
		proc_buf, proc_len);

	//POSIX 규정상 write는 성공적으로 처리한 바이트 수를 리턴해야함.
	return count;
}

//연산자 등록.
//등록 안하면 기본 연산자로 동작함.
static const struct proc_ops test_proc_ops = {
	.proc_read = test_read,
	.proc_write = test_write,
};

//test_kobj 구현
//읽기 동작
//sysfs_value를 문자열로 바꿔서 buf에 쓰고, 길이 리턴.
static ssize_t value_show(struct kobject *kobj, struct kobj_attribute *attr,
			  char *buf)
{
	return scnprintf(buf, PAGE_SIZE, "%d\n", sysfs_value);
}

//쓰기 동작
static ssize_t value_store(struct kobject *kobj, struct kobj_attribute *attr,
			   const char *buf, size_t count)
{
	int tmp;
	int ret;

	//유저가 쓴 문자열 데이터를 숫자로 변환
	ret = kstrtoint(buf, 10, &tmp);
	if (ret)
		return ret;

	sysfs_value = tmp;
	pr_info("myproc_sysfs: sysfs value set to %d\n", sysfs_value);

	return count;
}

//sysfs attribut 하나를 정의하는 매크로.
/*
매크로 구현 :
#define __ATTR(_name, _mode, _show, _store) {				\
	.attr = {.name = __stringify(_name),				\
		 .mode = VERIFY_OCTAL_PERMISSIONS(_mode) },		\
	.show	= _show,						\
	.store	= _store,						\
}

구조체 정의 :
struct kobj_attribute {
	struct attribute attr;
	ssize_t (*show)(struct kobject *kobj, struct kobj_attribute *attr,
			char *buf);
	ssize_t (*store)(struct kobject *kobj, struct kobj_attribute *attr,
			 const char *buf, size_t count);
};
*/
//이름, 권한, show 함수(읽기), store 함수(쓰기)
static struct kobj_attribute value_attribute =
	__ATTR(value, 0664, value_show, value_store);
//모듈 적재
static int __init proc_sysfs_init(void)
{
	int ret;

	printk(KERN_INFO "[proc_sysfs] module loaded.\n");

	// /proc/test를 처음 읽었을 때 나올 기본 메세지 설정.
	snprintf(proc_buf, PROC_BUF_SZ, "sysfs_value=%d\n", sysfs_value);
	proc_len = strlen(proc_buf);

	// /proc/test 엔트리 생성.
	// 이름, 권한, 부모, ops(연산)
	proc_entry = proc_create(PROC_NAME, 0666, NULL, &test_proc_ops);
	if (!proc_entry) {
		pr_err("proc_sysfs failed to create\n");
		return -ENOMEM;
	}

	//kobj 생성 /sys/kernel/ 아래에 생성.
	test_kobj = kobject_create_and_add("test_kobj", kernel_kobj);
	if (!test_kobj) {
		pr_err("proc_sysfs : faile to create kobject\n");
		remove_proc_entry(PROC_NAME, NULL);
		return -ENOMEM;
	}

	// /sys/kernel/test_kobj/value 파일 생성
	// 생성 실패시 에러처리.
	ret = sysfs_create_file(test_kobj, &value_attribute.attr);
	if (ret) {
		pr_err("proc_sysfs : faile to create sysfs file\n");
		kobject_put(test_kobj);
		remove_proc_entry(PROC_NAME, NULL);
		return ret;
	}
	pr_info("proc_sysfs: created /proc/%s and /sys/kernel/test_kobj/value\n",
		PROC_NAME);

	return 0;
}

//모듈 언로드
static void __exit proc_sysfs_exit(void)
{
	//kobj 정리.
	if (test_kobj) {
		sysfs_remove_file(test_kobj, &value_attribute.attr);
		kobject_put(test_kobj);
	}
	// proc 정리.
	if (proc_entry)
		remove_proc_entry(PROC_NAME, NULL);

	printk(KERN_INFO "[proc_sysfs] module unloaded.\n");
}
module_init(proc_sysfs_init);
module_exit(proc_sysfs_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("test");
MODULE_DESCRIPTION("procfs + sysfs");
