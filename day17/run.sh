#!/bin/bash

# 默认不带调试选项
DEBUG_MODE=0

# 解析命令行参数
while getopts "d" opt; do
  case $opt in
    d) DEBUG_MODE=1 ;;  # 如果传入 -d，启用调试模式
    *) echo "用法: $0 [-d]" && exit 1 ;;
  esac
done

# 清理
make clean || exit 1

# 编译和运行
if [ "$DEBUG_MODE" -eq 1 ]; then
  echo "===== 带调试选项编译和运行 ====="
  make DEBUG=1 all || exit 1
  make kernel.elf
  #make app/a.elf
  #make app/start.elf
  make DEBUG=1 qemu
else
  echo "===== 普通模式编译和运行 ====="
  make all || exit 1
  make qemu
fi

