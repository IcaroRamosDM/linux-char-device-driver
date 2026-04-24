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
