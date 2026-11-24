#!/usr/bin/env python3
"""Build only. This tool deliberately has no board programming operation."""
import argparse
import ast
import hashlib
import json
import os
from pathlib import Path
import struct
import subprocess


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--core-dir", type=Path, default=Path.home() / ".platformio")
    parser.add_argument("--output", type=Path, default=Path(".pio/build/recovery_boot"))
    args = parser.parse_args()
    root = Path(__file__).resolve().parent
    sdk = args.core_dir / "packages/framework-arduinostm32mxchip"
    tool = args.core_dir / "packages/toolchain-gccarmnoneeabi/bin/arm-none-eabi"
    builder = args.core_dir / "platforms/ststm32/builder/frameworks/arduino/mxchip.py"
    if json.loads((sdk / "package.json").read_text())["version"] != "2.0.0":
        parser.error("Use framework-arduinostm32mxchip 2.0.0 target headers")
    version = subprocess.check_output([str(tool) + "-gcc", "-dumpversion"], text=True).strip()
    if version != "6.3.1":
        parser.error("Use the existing GCC 6.3.1 toolchain, not a newlib-incompatible replacement")
    defines = next(ast.literal_eval(node.value) for node in ast.walk(ast.parse(builder.read_text()))
                   if isinstance(node, ast.keyword) and node.arg == "CPPDEFINES")
    flags = ["-mcpu=cortex-m4", "-mthumb", "-mfloat-abi=softfp", "-mfpu=fpv4-sp-d16",
             "-Os", "-g", "-ffunction-sections", "-fdata-sections", "-fno-unwind-tables",
             "-fno-asynchronous-unwind-tables", "-Wall", "-Wextra",
             "-include", "mbed_config.h", '-DMBEDTLS_CONFIG_FILE="config_mbedtls.h"',
             "-I" + str(root), "-I" + str(root / "third_party"), "-I" + str(root.parent / "include")]
    for define in defines:
        flags.append("-D" + (define if isinstance(define, str)
                             else define[0] + "=" + str(define[1])))
    for directory, _, files in sorted(os.walk(sdk / "system")):
        if any(name.endswith(".h") for name in files) or "inc" in directory:
            flags.append("-I" + directory)
    for directory in ("system", "system/mbed-os/features", "system/mbed-os/features/mbedtls",
                      "system/az3166-driver/mico/platform"):
        flags.append("-I" + str(sdk / directory))
    args.output.mkdir(parents=True, exist_ok=True)
    sources = [root / "startup.c", root / "boot.cpp", *sorted((root / "third_party").glob("*.c"))]
    objects = []
    for source in sources:
        obj = args.output / (source.stem + ".o")
        cpp = source.suffix == ".cpp"
        language = ["-std=gnu++11", "-fno-exceptions", "-fno-rtti", "-fno-threadsafe-statics"] if cpp else ["-std=gnu99"]
        subprocess.run([str(tool) + ("-g++" if cpp else "-gcc"), *flags, *language,
                        "-c", str(source), "-o", str(obj)], check=True)
        objects.append(str(obj))
    elf, binary = args.output / "boot.elf", args.output / "boot.bin"
    subprocess.run([str(tool) + "-g++", "-mcpu=cortex-m4", "-mthumb", "-mfloat-abi=softfp",
                    "-mfpu=fpv4-sp-d16", "-nostartfiles", "--specs=nano.specs", "--specs=nosys.specs",
                    "-Wl,--gc-sections", "-T", str(root / "boot.ld"),
                    "-Wl,-Map=" + str(args.output / "boot.map"), *objects,
                    "-Wl,--start-group", "-lc", "-lm", "-lgcc", "-lnosys", "-Wl,--end-group",
                    "-o", str(elf)], check=True)
    subprocess.run([str(tool) + "-objcopy", "-O", "binary", str(elf), str(binary)], check=True)
    data = binary.read_bytes()
    expected = (0x415A4231, 1, 0x0800C000, 0x98000, 1, 1, 0, 0)
    stack, reset = struct.unpack_from("<II", data)
    if (len(data) > 0xC000 or struct.unpack_from("<8I", data, 512) != expected or
            stack != 0x20040000 or not reset & 1 or not 0x08000220 <= reset < 0x0800C000):
        raise SystemExit("Boot image layout/ABI validation failed")
    undefined = subprocess.check_output([str(tool) + "-nm", "-u", str(elf)], text=True)
    symbols = subprocess.check_output([str(tool) + "-nm", "-C", str(elf)], text=True)
    if undefined.strip() or any(name in symbols for name in
                              ("mico_rtos_", "osKernel", "malloc", "operator new",
                               "HAL_FLASH_OB_", "HAL_FLASHEx_OBProgram")):
        raise SystemExit("Unexpected unresolved, OS, or option-byte programming dependency")
    subprocess.run([str(tool) + "-size", str(elf)], check=True)
    inputs = [*sources, *root.glob("*.ld"), *root.glob("*.h"),
              *(root / "third_party").rglob("*.h"),
              *(root.parent / "include" / name for name in
                ("Recovery.h", "RecoveryHash.h", "DurableRecord.h"))]
    result = {"bytes": len(data), "sha256": hashlib.sha256(data).hexdigest(),
              "origin": "0x08000000", "reset": hex(reset), "application": "0x0800C000",
              "hardware_verified": False, "compiler": version, "framework": "2.0.0",
              "sources": {str(p.relative_to(root)): hashlib.sha256(p.read_bytes()).hexdigest()
                          for p in sources},
              "inputs": {str(p.relative_to(root.parent)): hashlib.sha256(p.read_bytes()).hexdigest()
                         for p in sorted(inputs)}}
    (args.output / "build.json").write_text(json.dumps(result, indent=2) + "\n")
    print("Built and checked", binary, "-- NOT FLASHED")


if __name__ == "__main__":
    main()
