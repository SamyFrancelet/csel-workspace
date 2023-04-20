#include <linux/cdev.h>     // char device driver
#include <linux/fs.h>       // device drivers
#include <linux/init.h>     // macros
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/uaccess.h>  // copy data to/from user
#include <linux/slab.h>     // dynamic memory management

static int instances = 3;
module_param(instances, int, 0);

#define BUF_SIZE 10000
static char** dev_bufs = 0;

static dev_t mydriver_dev;
static struct cdev mydriver_cdev;

static int mydriver_open(struct inode* i, struct file* f)
{
    pr_info("mydriver: open operation, major:%d, minor:%d\n",
            imajor(i),
            iminor(i));

    if (iminor(i) >= instances) {
        return -EFAULT;
    }

    if ((f->f_flags & (O_APPEND)) != 0) {
        pr_info("mydriver: opened for append\n");
    }

    if ((f->f_mode & (FMODE_READ | FMODE_WRITE)) != 0) {
        pr_info("mydriver: opened for read & write\n");
    } else if ((f->f_mode & FMODE_READ) != 0) {
        pr_info("mydriver: opened for read\n");
    } else if ((f->f_mode & FMODE_WRITE) != 0) {
        pr_info("mydriver: opened for write\n");
    }

    // sets the global device buffer to the file
    f->private_data = dev_bufs[iminor(i)];
    pr_info("mydriver: private_data=%p\n", f->private_data);

    return 0;
}

static int mydriver_release(struct inode* i, struct file* f)
{
    pr_info("mydriver: release operation\n");

    return 0;
}

static ssize_t mydriver_read(struct file* f,
                             char __user* buf,
                             size_t count,
                             loff_t* off)
{
    // update remaining bytes, count and pointers
    ssize_t remaining = BUF_SIZE - (ssize_t)(*off);
    char* ptr         = (char*)f->private_data + *off;
    if (count > remaining) count = remaining;
    *off += count;

    // copy to user space the required number of bytes
    if (copy_to_user(buf, ptr, count) != 0) count = -EFAULT;

    pr_info("mydriver: read operation, read=%ld\n", count);

    return count;
}

static ssize_t mydriver_write(struct file* f,
                              const char __user* buf,
                              size_t count,
                              loff_t* off)
{
    // update remaining bytes
    ssize_t remaining = BUF_SIZE - (ssize_t)(*off);

    pr_info("mydriver: at%ld\n", (unsigned long)(*off));

    // check if we can write
    if (count >= remaining) count = -ENOSPC;

    // copy from user space the required number of bytes
    if (count > 0) {
        char* ptr = (char*)f->private_data + *off;
        *off += count;
        ptr[count] = '\0'; // null terminate the string
        if (copy_from_user(ptr, buf, count) != 0) count = -EFAULT;
    }

    pr_info("mydriver: write operation, wrote=%ld\n", count);

    return count;
}

static struct file_operations mydriver_fops = {
    .owner   = THIS_MODULE,
    .open    = mydriver_open,
    .release = mydriver_release,
    .read    = mydriver_read,
    .write   = mydriver_write,
};

static int __init mydriver_init(void)
{
    int i;
    int err;

    if (instances <= 0) {
        pr_err("mydriver: invalid number of instances\n");
        return -EINVAL;
    }

    err = alloc_chrdev_region(&mydriver_dev, 0, instances, "mydriver");

    if (!err) {
        cdev_init(&mydriver_cdev, &mydriver_fops);
        mydriver_cdev.owner = THIS_MODULE;
        err = cdev_add(&mydriver_cdev, mydriver_dev, instances);
    }

    if (!err) {
        dev_bufs = kzalloc(sizeof(char*) * instances, GFP_KERNEL);
        for (i = 0; i < instances; i++) {
            dev_bufs[i] = kzalloc(BUF_SIZE, GFP_KERNEL);
        }
    }

    if (err) {
        pr_err("mydriver: failed to register device\n");
        return err;
    }

    pr_info("mydriver: registered %d dev with major %d and minor %d\n",
            instances,
            MAJOR(mydriver_dev),
            MINOR(mydriver_dev));

    pr_info("mydriver: loaded module\n");

    return 0;
}

static void __exit mydriver_exit(void)
{
    int i;

    cdev_del(&mydriver_cdev);
    unregister_chrdev_region(mydriver_dev, instances);
    pr_info("mydriver: unregistered device\n");

    for (i = 0; i < instances; i++) {
        kfree(dev_bufs[i]);
    }

    kfree(dev_bufs);    

    pr_info("mydriver: freed memory\n");
    pr_info("mydriver: unloaded module\n");
}

module_init(mydriver_init);
module_exit(mydriver_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Samy Francelet <samy.francelet@ik.me>");
MODULE_DESCRIPTION("mydriver csel module");