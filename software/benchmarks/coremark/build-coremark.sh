#!/bin/bash

set -e

BASEDIR="$(cd "$(dirname "$0")" && pwd)"
CM_FOLDER=coremark
ITERATIONS="${1:-2000}"
PORT="${2:-/dev/ttyUSB0}"

echo "Building CoreMark for Z-Core, ITERATIONS=${ITERATIONS} ..."

cd "${BASEDIR}/${CM_FOLDER}"

make PORT_DIR=../z_core compile ITERATIONS="${ITERATIONS}"

riscv32-unknown-elf-objcopy -O binary coremark.elf "${BASEDIR}/coremark.bin"
riscv32-unknown-elf-objdump -d -S coremark.elf > "${BASEDIR}/coremark.lst"
cp coremark.elf "${BASEDIR}/coremark.elf"

echo ""
echo "Build complete."
echo "  ${BASEDIR}/coremark.elf"
echo "  ${BASEDIR}/coremark.bin"
echo "  ${BASEDIR}/coremark.lst  (annotated disassembly)"
echo ""
echo "To upload to Z-Core via bootloader:"
echo "  cd ${BASEDIR}/../.."
echo "  python3 ../scripts/upload.py ${PORT} benchmarks/coremark/coremark.bin --reset --base 0x10000000 --entry 0x10000000"
echo ""
echo "Expected UART output:"
echo "  2K performance run parameters for coremark."
echo "  CoreMark Size    : 666"
echo "  Total ticks      : <cycles>"
echo "  Total time (secs): <seconds>"
echo "  Iterations/Sec   : <score>"
echo "  Iterations       : ${ITERATIONS}"
