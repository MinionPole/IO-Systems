#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/version.h>
#include <linux/types.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/cdev.h>
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Aleksei Lapin");
MODULE_DESCRIPTION("Simple module to start");

#define DEVICE_NAME "io_lab"

#define BUF_SIZE;

char ibuf[BUF_SIZE];
static u64 coma_count = 0; 

static dev_t major = 0; /* The major number assigned to the device driver */
static struct class *dev_class;
static struct cdev io_dev;

static int __init chdrv_init(void);
static void __exit chdrv_exit(void);

static ssize_t my_read(struct file *filp, char __user *buf, size_t len, loff_t *off);
static ssize_t my_write(struct file *filp, const char __user *buf, size_t len, loff_t *off);
static int my_open(struct inode *inode, struct file *file);
static int my_release(struct inode *inode, struct file *file);
static long my_ioctl(struct file *file, unsigned int cmd, unsigned long arg);

static struct file_operations fops = {
    .owner          = THIS_MODULE,
    .read           = my_read,
    .write          = my_write,
    .open           = my_open,
    .release        = my_release,
    .unlocked_ioctl = my_ioctl,
};

static ssize_t my_read(struct file *filp, char __user *buf, size_t len, loff_t *off){
    int count = strlen(ibuf);
    pr_info("Driver: read()\n");
  
    if (*off > 0 || len < count) {
        return 0;
    }
    sprintf(ibuf, "%d", coma_count);
    
    if (copy_to_user(buf, ibuf, count) != 0) {
        return -EFAULT;
    }
    
    *off = count;
    
    return count;
}

static ssize_t my_write(struct file *filp, const char __user *buf, size_t len, loff_t *off){
    printk(KERN_INFO "Driver: write()\n");
  
    if(len > BUF_SIZE)
      return 0;
  
    if (copy_from_user(ibuf, buf, len) != 0) {
      return -EFAULT;
    }

    for(size_t i = 0; i < len; i++){
        if (ibuf[i] == ','){
            coma_count++;
            if(coma_count <= 0){
                pr_err("Integer overflow");
                return -EFAULT;
            }
        }
    }
    return len;
}

static int my_open(struct inode *inode, struct file *file){
    pr_info("Device opened.\n");
    return 0;
}

static int my_release(struct inode *inode, struct file *file){
    pr_info("Device closed.\n");
    return 0;
}

static long my_ioctl(struct file *file, unsigned int cmd, unsigned long arg){
    pr_info("Device ioctl.\n");
    return 0;
}

static int __init chdrv_init(void){
    pr_info("cpu_stat: Module loaded\n");

    /* Allocating major numbers */
    if(alloc_chrdev_region(&major, 0, 1, DEVICE_NAME) < 0){
        pr_err("Cannot allocate major numbers.\n");
        return -1;
    }

    /* cdev structure initialization */
    cdev_init(&cs_dev, &fops);

    /* Adding device to the system */
    if(cdev_add(&cs_dev, major, 1) < 0){
        pr_err("Cannot add the device to the system.\n");
        goto rm_major;
    }

    /* Creating structure class */
    if((dev_class = class_create(THIS_MODULE, DEVICE_NAME)) == NULL) {
        pr_err("Cannot create the structure class.\n");
        goto rm_major;
    }

    if(device_create(dev_class, NULL, major, NULL, DEVICE_NAME) < 0){
        pr_err("Cannot create the device");
        goto rm_class;
    }

    pr_info("Device created on /dev/%s\n", DEVICE_NAME);

    return 0;

rm_class:
    class_destroy(dev_class);
rm_major:
    unregister_chrdev_region(major, 1);
    return -1;
}

static void __exit chdrv_exit(void){
    device_destroy(dev_class, major);
    class_destroy(dev_class);
    cdev_del(&io_dev);
    unregister_chrdev_region(major, 1);
    pr_info("cpu_stat: Module unloaded.\n");
}