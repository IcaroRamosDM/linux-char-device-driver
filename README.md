# Linux Character Device Driver

[![Checks](https://github.com/IcaroRamosDM/linux-char-device-driver/actions/workflows/checks.yml/badge.svg)](https://github.com/IcaroRamosDM/linux-char-device-driver/actions/workflows/checks.yml)

An educational Linux kernel module exposing a synchronized 1024-byte message
buffer at /dev/char_device.

## Behavior

Reads respect the requested byte count and file offset and return EOF after
the stored message. Each write sequence beginning at offset zero replaces the
previous message. Larger input is written only up to the remaining capacity;
a further nonempty write returns ENOSPC. A zero-length write preserves the message.
A failed copy from user space preserves the previous data.

A mutex protects the buffer and size. Individual read/write operations are
serialized; multiple partial reads can still observe another writer's later
message between calls. The device is nonseekable and does not provide ioctl,
poll, per-client messages or multiple device instances.

## Build

~~~sh
make
# Or select headers explicitly:
make KDIR=/usr/src/linux-headers-YOUR-VERSION
~~~

The source uses the class_create API provided by Linux 6.4 and newer.
Build against headers matching the intended kernel.
W=1 enables extra kernel build warnings. Build products are not distribution
packages and are not loaded by make.

## Load, use and remove

~~~sh
sudo insmod char_device.ko
printf 'hello\n' | sudo tee /dev/char_device
sudo cat /dev/char_device
sudo rmmod char_device
~~~

Use the host's module-loading policy. Compilation alone does not verify
that a module can be loaded, and a kernel module should be tested in a disposable
development system or VM.

## Integration checks

With this demonstration module already loaded:

~~~sh
sudo python3 tests/check_device.py
~~~

The script changes the demo buffer and checks one-byte reads, EOF, empty writes
and capacity. It never loads or removes a module.

## License

[MIT](LICENSE).

## Author

**Icaro Ramos Rodrigues dos Santos**<br>
Electronic Engineer

Embedded Systems | Firmware | C | C++ | Python | Dart | Assembly | Hardware Design | PCB Design | STM32 | ARM Cortex-M | Linux | Windows | RISC-V | Verilog | SystemVerilog | UVM | Design Verification | Instrumentation | Signal Processing | Qt | FLTK

GitHub: [IcaroRamosDM](https://github.com/IcaroRamosDM)<br>
LinkedIn: [icaro-ramos-r](https://www.linkedin.com/in/icaro-ramos-r/)<br>
[icaroelt@gmail.com](mailto:icaroelt@gmail.com)
