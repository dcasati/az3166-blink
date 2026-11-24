#!/usr/bin/env python3
"""Run the production recovery engine against NOR storage under host sanitizers."""
import os
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[2]
with tempfile.TemporaryDirectory(prefix="az3166-recovery-") as directory:
    output = Path(directory)
    flags = ["-O1", "-g", "-fsanitize=address,undefined", "-fno-omit-frame-pointer",
             '-DMBEDTLS_CONFIG_FILE="config_mbedtls.h"',
             "-I" + str(root / "include"), "-I" + str(root / "bootloader"),
             "-I" + str(root / "bootloader/third_party")]
    objects = []
    for name in ("source-sha256.c", "source-platform_util.c"):
        obj = output / (name + ".o")
        subprocess.run([os.environ.get("CC", "cc"), "-std=c99", *flags, "-c",
                        str(root / "bootloader/third_party" / name), "-o", str(obj)], check=True)
        objects.append(str(obj))
    binary = output / "test-recovery"
    subprocess.run([os.environ.get("CXX", "c++"), "-std=c++11", "-Wall", "-Wextra", "-Werror",
                    *flags, str(root / "test/test_ota/test_recovery.cpp"), *objects,
                    "-o", str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
