#!/usr/bin/env python3
"""Exercise an already loaded character device; no module is loaded here."""

import argparse
import errno
import os
from pathlib import Path


def require(condition, message):
    if not condition:
        raise AssertionError(message)


def read_all(path, chunk_size):
    chunks = []
    with path.open("rb", buffering=0) as stream:
        while chunk := stream.read(chunk_size):
            chunks.append(chunk)
    return b"".join(chunks)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--device", type=Path, default=Path("/dev/char_device"))
    args = parser.parse_args()
    if not args.device.exists():
        parser.exit(2, "Load the module first; see README.md.\n")

    message = b"partial reads preserve every byte\n"
    with args.device.open("wb", buffering=0) as stream:
        written = stream.write(message)
    require(written == len(message), "Unexpected partial message write")
    require(read_all(args.device, 1) == message, "One-byte reads lost data")
    require(read_all(args.device, 7) == message, "Partial reads lost data")

    descriptor = os.open(args.device, os.O_WRONLY)
    try:
        require(os.write(descriptor, b"") == 0, "An empty write changed the byte count")
    finally:
        os.close(descriptor)
    require(
        read_all(args.device, 1024) == message, "An empty write changed the message"
    )

    payload = bytes(range(256)) * 5
    descriptor = os.open(args.device, os.O_WRONLY)
    try:
        require(
            os.write(descriptor, payload) == 1024,
            "The buffer boundary was not enforced",
        )
        try:
            os.write(descriptor, b"x")
        except OSError as error:
            if error.errno != errno.ENOSPC:
                raise
        else:
            raise AssertionError("A full buffer must reject another nonempty write")
    finally:
        os.close(descriptor)
    require(read_all(args.device, 31) == payload[:1024], "The stored payload differs")
    print("PASS: partial reads, EOF, zero-length writes and the 1024-byte boundary")


if __name__ == "__main__":
    main()
