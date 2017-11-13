#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/miscdevice.h>
#include <linux/mm.h>

#include <asm/uaccess.h>

#include "kmod_user.h"

static int kmod_open(struct inode *inode, struct file *filp)
{
	struct page *p;
	printk(KERN_INFO "open syscall is called\n");
	p = alloc_page(GFP_ATOMIC);
	if (p == NULL) {
		return -ENOMEM;
	}
	filp->private_data = page_address(p);
	printk("page struct at %p, page address %p", p, filp->private_data);
	snprintf(filp->private_data, PAGE_SIZE, "Hello our kernel module!");
	printk(KERN_INFO "open() allocates private data\n");
	return 0;
}

static int kmod_release(struct inode *inode, struct file *filp)
{
	if (filp->private_data == NULL)
		printk(KERN_INFO "This file pointer does not have private_data\n");
	else {
		free_page((unsigned long) filp->private_data);
		filp->private_data = NULL;
		printk(KERN_INFO "release() released private data\n");
	}
	return 0;
}

static int kmod_mem_fault(struct vm_area_struct *vma, struct vm_fault *vmf)
{
	char *priv = vma->vm_private_data;
	struct page *page;
	unsigned long off = (vma->vm_pgoff + vmf->pgoff) << PAGE_SHIFT;
	unsigned long pa, pfn;

	printk(KERN_INFO "page fault happens, offset is %lu\n", off);

	pa = virt_to_phys(priv + off);
	if (pa == 0)
		return VM_FAULT_SIGBUS;
	pfn = pa >> PAGE_SHIFT;
	if (!pfn_valid(pfn))
		return VM_FAULT_SIGBUS;
	page = pfn_to_page(pfn);
	get_page(page);
	vmf->page = page;
	return 0;
}

static struct vm_operations_struct kmod_mmap_ops = {
	.fault = kmod_mem_fault,
};

static int kmod_mmap(struct file *filp, struct vm_area_struct *vma)
{
	char *priv = filp->private_data;
	unsigned long off;

	off = vma->vm_pgoff << PAGE_SHIFT;
	if (off > PAGE_SIZE) {
		printk("mmap() is called for mapping %lu bytes, but we only have 4096 bytes\n", off);
		return -EINVAL;
	}

	vma->vm_private_data = priv;
	vma->vm_ops = &kmod_mmap_ops;

	return 0;
}

static long kmod_ioctl(struct file *filp, u_int cmd, u_long data)
{
	struct shared_struct s;
	char *mem = filp->private_data;
	if (copy_from_user(&s, (void *) data, sizeof(struct shared_struct)) != 0) {
		printk(KERN_INFO "copy_from_user failed");
		return -EFAULT;
	}
	if (s.len > PAGE_SIZE) {
		printk(KERN_INFO "specified length is bigger than 4096");
		return -EINVAL;
	}
	mem[s.len] = '\0';

	printk("Message from application : %s\n", mem);

	return 0;
}

static u_int kmod_poll(struct file * filp, struct poll_table_struct *pwait)
{
	return 0;
}

static struct file_operations kmod_fops = {
	.owner = THIS_MODULE,
	.open = kmod_open,
	.mmap = kmod_mmap,
	.unlocked_ioctl = kmod_ioctl,
	.poll = kmod_poll,
	.release = kmod_release,
};

struct miscdevice kmod_cdevsw = {
	MISC_DYNAMIC_MINOR,
	"kmod",
	&kmod_fops,
};

static int __init kmod_module_init(void)
{
	misc_register(&kmod_cdevsw);
	printk(KERN_INFO "Linux character device driver is loaded\n");
	return 0;
}

static void __exit kmod_module_exit(void)
{
	misc_deregister(&kmod_cdevsw);
	printk(KERN_INFO "Linux character device driver is unloaded\n");
	return;
}

module_init(kmod_module_init);
module_exit(kmod_module_exit);

MODULE_DESCRIPTION("Kernel Module Template");
MODULE_LICENSE("Dual BSD/GPL");
