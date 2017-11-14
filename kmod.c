#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/miscdevice.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/poll.h>

#include <asm/uaccess.h>

#include "kmod_user.h"

struct kmod_priv {
	wait_queue_head_t wq;
	unsigned int num_pages;
	char **page_ptr;
};

static int kmod_open(struct inode *inode, struct file *filp)
{
	struct kmod_priv *priv = (struct kmod_priv *) kzalloc(sizeof(struct kmod_priv), GFP_KERNEL);
	if (priv == NULL)
		return -ENOMEM;
	init_waitqueue_head(&priv->wq);
	filp->private_data = (void *) priv;
	printk(KERN_INFO "open() allocated private data\n");
	return 0;
}

static int kmod_release(struct inode *inode, struct file *filp)
{
	struct kmod_priv *priv = (struct kmod_priv *) filp->private_data;
	unsigned int i;
	for (i = 0; i < priv->num_pages; i++)
		free_page((unsigned long) priv->page_ptr[i]);
	if (priv->page_ptr)
		kfree(priv->page_ptr);
	kfree(priv);
	printk(KERN_INFO "release() released private data\n");
	return 0;
}

static int kmod_mem_fault(struct vm_area_struct *vma, struct vm_fault *vmf)
{
	struct kmod_priv *priv = vma->vm_private_data;
	struct page *page;
	unsigned long off = (vma->vm_pgoff + vmf->pgoff) << PAGE_SHIFT;
	unsigned long pa, pfn;

	printk(KERN_INFO "page fault offset %lu, page %lu\n", off, off / PAGE_SIZE);

	pa = virt_to_phys(priv->page_ptr[off / PAGE_SIZE]);
	if (pa == 0) {
		printk(KERN_INFO "wrong pa\n");
		return VM_FAULT_SIGBUS;
	}

	pfn = pa >> PAGE_SHIFT;
	if (!pfn_valid(pfn)) {
		printk(KERN_INFO "Invalid pfn\n");
		return VM_FAULT_SIGBUS;
	}

	page = pfn_to_page(pfn);
	get_page(page);
	vmf->page = page;

	printk(KERN_INFO "mmap is done\n");

	return 0;
}

static struct vm_operations_struct kmod_mmap_ops = {
	.fault = kmod_mem_fault,
};

static int kmod_mmap(struct file *filp, struct vm_area_struct *vma)
{
	struct kmod_priv *priv = filp->private_data;
	unsigned long off = vma->vm_pgoff << PAGE_SHIFT;

	if (off + (vma->vm_end - vma->vm_start) > (priv->num_pages * PAGE_SIZE))
		return -EINVAL;

	vma->vm_private_data = priv;
	vma->vm_ops = &kmod_mmap_ops;

	return 0;
}

static long kmod_ioctl(struct file *filp, unsigned int cmd, unsigned long data)
{
	struct kmod_priv *priv = (struct kmod_priv *) filp->private_data;
	struct shared_struct s;

	if (copy_from_user(&s, (void *) data, sizeof(struct shared_struct)) != 0) {
		printk(KERN_INFO "copy_from_user failed\n");
		return -EFAULT;
	}

	switch (cmd) {
	case IOCREGMEM:
		{
			unsigned int i, j, num_pages;
			num_pages = (s.len % PAGE_SIZE == 0) ? (s.len / PAGE_SIZE) : (s.len / PAGE_SIZE) + 1;
			priv->page_ptr = kzalloc(sizeof(char **) * num_pages, GFP_KERNEL);
			if (priv->page_ptr == NULL)
				return -ENOMEM;
			for (i = 0; i < num_pages; i++) {
				struct page *page = alloc_page(GFP_ATOMIC | __GFP_ZERO);
				if (page == NULL) {
					for (j = 0; j < i; j++) {
						free_page((unsigned long) priv->page_ptr[j]);
						priv->page_ptr[j] = NULL;
					}
					kfree(priv->page_ptr);
					return -ENOMEM;
				}
				priv->page_ptr[i] = page_address(page);
				printk(KERN_INFO "page %d at %p", i, priv->page_ptr[i]);
			}
			priv->num_pages = num_pages;
			printk(KERN_INFO "%u pages are allocated\n", num_pages);
		}
		break;
	case IOCPRINTK:
		{
			char *buf;
			printk(KERN_INFO "off %lu, len %lu\n", s.off, s.len);
			if (priv->num_pages == 0 || s.off > (priv->num_pages * PAGE_SIZE))
				return -EINVAL;
			buf = priv->page_ptr[s.off / PAGE_SIZE] + (s.off % PAGE_SIZE);
			buf[s.len] = '\0';
			printk(KERN_INFO "%s\n", buf);
		}
		break;
	default:
		printk(KERN_INFO "ioctl(): cmd is not implemented\n");
		break;
	}

	return 0;
}

static u_int kmod_poll(struct file * filp, struct poll_table_struct *pwait)
{
	struct kmod_priv *priv = filp->private_data;
	poll_wait(filp, &priv->wq, pwait);
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
}

module_init(kmod_module_init);
module_exit(kmod_module_exit);

MODULE_LICENSE("Dual BSD/GPL");
