obj-m += char_device.o

KDIR ?= /lib/modules/$(shell uname -r)/build
PYTHON ?= python3

.PHONY: all clean test

all:
	$(MAKE) -C "$(KDIR)" M="$(CURDIR)" W=1 modules

clean:
	$(MAKE) -C "$(KDIR)" M="$(CURDIR)" clean

test:
	$(PYTHON) tests/check_device.py
