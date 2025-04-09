#!/bin/bash

# 检查是否以root身份运行
if [ "$EUID" -ne 0 ]; then
  echo "请以root身份运行此脚本"
  exit 1
fi

# 停止并禁用服务
echo "停止并禁用服务..."
systemctl stop stability-system.service
systemctl disable stability-system.service

# 删除服务文件
echo "删除服务文件..."
rm -f /lib/systemd/system/stability-system.service
systemctl daemon-reload

# 询问是否删除配置文件
read -p "是否删除配置文件? [y/N] " -n 1 -r
echo
if [[ $REPLY =~ ^[Yy]$ ]]; then
  echo "删除配置文件..."
  rm -rf /etc/stability-system
fi

# 删除可执行文件
echo "删除可执行文件..."
rm -f /usr/local/bin/stability_server

echo "卸载完成！" 