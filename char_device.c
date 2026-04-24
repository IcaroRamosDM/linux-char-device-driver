#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/uaccess.h>
#include <linux/kernel.h>
#include <linux/string.h>

#define BUFFER_SIZE 1024

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Icaro Ramos");
MODULE_DESCRIPTION("Simple Character Device Driver");

/* Device number containing the dinamically allocated major and minor numbers */
static dev_t dev_num;

/* Kernel representation of this character device */
static struct cdev char_cdev;

/* Device class and device object use to create /dev/char_device automatically */
static struct class *char_class;
static struct device *char_device;

/* Fixed-size kernel buffer used to store data written from user space */
static char device_buffer[BUFFER_SIZE];

/* Number of valid bytes currently stored in device_buffer */
static size_t buffer_size = 0;

static int char_open(struct inode *inode, struct file *file) {
    printk(KERN_INFO "char_device: device opened\n");
    return 0;
}

static int char_release(struct inode *inode, struct file *file) {
    printk(KERN_INFO "char_device: device closed\n");
    return 0;
}

static ssize_t char_read(struct file *file, char __user *buf, size_t count, loff_t *offset) {
    
    size_t bytes_to_read;

    /* Return EOF when the current file offset reaches the stored data size */
    if (*offset >= buffer_size) {
        return 0;
    }

    /* Read only the amount requessted by user space or the remaining data available */
    bytes_to_read = min(count, buffer_size - (size_t)*offset);

    /* Copy data safely from kernel to user space */
    if (copy_to_user(buf, device_buffer + *offset, bytes_to_read)) {
        printk(KERN_ERR "char_device: failed to copy data to user\n");
        return -EFAULT;
    }

    /* Advance the file offset by the number of bytes successfully read */
    *offset += bytes_to_read;

    printk(KERN_INFO "char_device: read %zu bytes\n", bytes_to_read);

    return bytes_to_read;
}

static ssize_t char_write(struct file *file, const char __user *buf, size_t count, loff_t *offset) {

    size_t bytes_to_write;
    size_t space_left;

    /* A new write sequence starts at offset 0.
       Clear the previous content so the device stores only the latest write */
    if (*offset == 0) {
        memset(device_buffer, 0, BUFFER_SIZE);
        buffer_size = 0;
    }

    /* Reject writes when the file offset has already reached the buffer limit */
    if (*offset >= BUFFER_SIZE) {
        printk(KERN_WARNING "char_device: no space left in buffer\n");
        return -ENOSPC;
    }

    /* Calculate how much space remains in the fixed-size buffer */
    space_left = BUFFER_SIZE - (size_t)*offset;

    /* If the user space send more data than available space, only part of it is stored */
    if  (count > space_left) {
        printk(KERN_WARNING "char device: input too large, truncating from %zu to %zu bytes\n",
               count, space_left);
    }

    bytes_to_write = min(count, space_left);

    /* Copy data safely from user space into the kernel buffer */
    if (copy_from_user(device_buffer + *offset, buf, bytes_to_write)) {
        printk(KERN_ERR "char_device: failed to copy data from user\n");
        return -EFAULT;
    }

    /* Advance the file offset by the number of bytes successfully written */
    *offset += bytes_to_write;

    /* Track the amount of valid data stored in the buffer */
    if (buffer_size < (size_t)*offset)
        buffer_size = (size_t)*offset;

    printk(KERN_INFO "char_device: wrote %zu bytes\n", bytes_to_write);

    return bytes_to_write;
}

/* File operations exposed by this character device */
static struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = char_open,
    .release = char_release,
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
    printk(KERN_INFO "char_device: registered with major %d and minor %d\n",
           MAJOR(dev_num), MINOR(dev_num));
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
