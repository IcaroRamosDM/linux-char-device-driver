# Linux Character Device Driver

A simple Linux Kernel Module implementing a character device driver.

## Overview

This project implements a basic character device driver that allows user-space applications to write data to the kernel and read it back.

The driver:
- Registers a character device
- Creates a device node automatically in `/dev`
- Supports `open`, `read`, `write`, and `release` operations
- Stores data in a fixed-size internal buffer

---

## Features

- Dynamic allocation of major/minor numbers
- Automatic creation of `/dev/char_device`
- Kernel-space buffer for storing data
- Safe data transfer using `copy_to_user` and `copy_from_user`
- Basic error handling and logging via `printk`

---

## How it works internally

This driver implements a simple character device backed by a fixed-size kernel buffer.

### Device registration

During module initialization, the driver dynamically allocates a device number using `alloc_chrdev_region`. This provides a major and a minor number for the character device.

The driver then initializes a `struct cdev` with the supported file operations and registers it with the kernel using `cdev_add`.

To make the device accessible from user space, the module creates a device class with `class_create` and a device node with `device_create`. This automatically exposes the device as:

```bash
/dev/char_device
```

### File operations

The driver provides four basic file operations:
- `open`: called when the device is opened
- `read`: called when user space reads from the device
- `write`: called when the user space writes to the device
- `release`: called when the device is closed

These operations are registered through the `file_operations` structure.

### Write path

When user space writes to `/dev/char_device`, the kernel calls the driver's `write` function.

The driver copies data from user space into an internal kernel buffer using `copy_from_user`.

The buffer has a fixed size of 1024 bytes. If the input is larger than the available buffer space, only the bytes that fir are stored and the remaining bytes are discarded.

A new write sequence starting at offset `0` clears the previous buffer contents. This means the device stores the latest written content instead of appending new data forever.

### Read path

When user space reads from `/dev/char_device`, the kernel calls the driver's `read` function.

The driver copies data from the internal kernel buffer back to user space using `copy_to_user`.

The read operation respects the file offset. Once the offset reaches the amount of valid data stored in the buffer, the function returns `0`, which signals end-of-file to user-soace programs such as `cat`.

### Cleanup

When the module is removed, the driver destroys the device node, destroys the device class, removes the character device, and releases the allocated device number.

The cleanup happens in the reverse order of initialiation to avoid leaving stale kernel resources behind.

---

## Build Instructions

Make sure you have the kernel headers installed.

```bash
make
```

This will generate the kernel module:

```bash
char_device.ko
```
---

## Load and Unload Module

### Load the module

```bash
sudo insmod char_device.ko
```

### Check kernel logs

```bash
sudo dmesg | tail
```

### Unload the module

```bash
sudo rmmod char_device
```

---

## Device File

After loading the module, the device will be available at:

```bash
/dev/char_device
```

---

## Usage

### Write to the device

```bash
echo "hello" | sudo tee /dev/char_device
```

### Read from the device

```bash
sudo cat /dev/char_device
```

---

## Example

```bash
echo "abcdef" | sudo tee /dev/char_device > /dev/null
sudo cat /dev/char_device
```

Output:

```text
abcdef
```

---

## Buffer Behavior

- The driver uses a fixed-size buffer of 1024 bytes.
- Each write operation overwrites the previous content.
- If more than 1024 bytes are written, the input is truncated.

Example:
- Input: 1100 bytes
- Stored: first 1024 bytes
- Remaining bytes are discarded

---

## Limitations

- Single fixed-size buffer (no dynamic allocation)
- No synchronization (not safe for concurrent access)
- Write operation does not support append
- Offset handling is simplified
- No support for multiple devices

---

## Future Improvements

- Add synchronization (mutex/spinlock)
- Implement circular buffer
- Support multiple devices
- Improve error handling
- Add ioctl support

---

## Author

Ícaro Ramos
icaroelt@gmail.com
