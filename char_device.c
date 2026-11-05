#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/uaccess.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/mutex.h>
#include <linux/slab.h>

#define BUFFER_SIZE 1024

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Icaro Ramos");
MODULE_DESCRIPTION("Simple Character Device Driver");

/* Device number allocated when the module is loaded. */
static dev_t dev_num;

/* Kernel representation of this character device */
static struct cdev char_cdev;

/* Objects used by udev to expose /dev/char_device. */
static struct class *char_class;
static struct device *char_device;

/* Fixed-size kernel buffer used to store data written from user space */
static char device_buffer[BUFFER_SIZE];

/* Number of valid bytes currently stored in device_buffer */
static size_t buffer_size;
static DEFINE_MUTEX(device_mutex);

static int char_open(struct inode *inode, struct file *file) {
    return nonseekable_open(inode, file);
}

static ssize_t char_read(struct file *file, char __user *buffer, size_t count, loff_t *offset) {
    ssize_t result;

    (void)file;
    if (mutex_lock_interruptible(&device_mutex)) {
        return -ERESTARTSYS;
    }
    result = simple_read_from_buffer(buffer, count, offset, device_buffer, buffer_size);
    mutex_unlock(&device_mutex);
    return result;
}

static ssize_t char_write(struct file *file, const char __user *buffer, size_t count,
                          loff_t *offset) {
    ssize_t result;
    size_t bytes_to_write;
    void *incoming;

    (void)file;
    if (mutex_lock_interruptible(&device_mutex)) {
        return -ERESTARTSYS;
    }
    if (*offset < 0) {
        result = -EINVAL;
        goto unlock;
    }
    if (count == 0) {
        result = 0;
        goto unlock;
    }
    if (*offset >= BUFFER_SIZE) {
        result = -ENOSPC;
        goto unlock;
    }
    bytes_to_write = min(count, BUFFER_SIZE - (size_t)*offset);

    /* A failed user copy must leave the previous message intact. */
    incoming = memdup_user(buffer, bytes_to_write);
    if (IS_ERR(incoming)) {
        result = PTR_ERR(incoming);
        goto unlock;
    }
    if (*offset == 0) {
        memset(device_buffer, 0, sizeof device_buffer);
        buffer_size = 0;
    }
    memcpy(device_buffer + (size_t)*offset, incoming, bytes_to_write);
    kfree(incoming);
    *offset += bytes_to_write;
    buffer_size = max(buffer_size, (size_t)*offset);
    result = (ssize_t)bytes_to_write;

unlock:
    mutex_unlock(&device_mutex);
    return result;
}

/* File operations exposed by this character device */
static struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = char_open,
    .read = char_read,
    .write = char_write,
};

static int __init char_device_init(void) {
    int ret;

    /* Dynamically allocate one character device number */
    ret = alloc_chrdev_region(&dev_num, 0, 1, "char_device");

    if (ret < 0) {
        printk(KERN_ERR "char_device: failed to allocate device number\n");
        return ret;
    }

    /* Initialize and register the character device with the kernel */
    cdev_init(&char_cdev, &fops);
    char_cdev.owner = THIS_MODULE;

    ret = cdev_add(&char_cdev, dev_num, 1);

    if (ret < 0) {
        printk(KERN_ERR "char_device: failed to add cdev\n");
        unregister_chrdev_region(dev_num, 1);
        return ret;
    }

    /* Create a device class used by udev to expose the device under /dev */
    char_class = class_create("char_device_class");
    if (IS_ERR(char_class)) {
        printk(KERN_ERR "char_device: failed to create class\n");
        cdev_del(&char_cdev);
        unregister_chrdev_region(dev_num, 1);
        return PTR_ERR(char_class);
    }

    /* Create /dev/char_device automatically */
    char_device = device_create(char_class, NULL, dev_num, NULL, "char_device");
    if (IS_ERR(char_device)) {
        printk(KERN_ERR "char_device: failed to create device\n");
        class_destroy(char_class);
        cdev_del(&char_cdev);
        unregister_chrdev_region(dev_num, 1);
        return PTR_ERR(char_device);
    }

    printk(KERN_INFO "char_device: module loaded\n");
    printk(KERN_INFO "char_device: registered with major %d and minor %d\n", MAJOR(dev_num),
           MINOR(dev_num));
    printk(KERN_INFO "char_device: device created at /dev/char_device\n");

    return 0;
}

static void __exit char_device_exit(void) {

    /* Destroy resources in the reverse order of initialization */
    device_destroy(char_class, dev_num);
    class_destroy(char_class);
    cdev_del(&char_cdev);
    unregister_chrdev_region(dev_num, 1);

    printk(KERN_INFO "char_device: module unloaded\n");
}

module_init(char_device_init);
module_exit(char_device_exit);
