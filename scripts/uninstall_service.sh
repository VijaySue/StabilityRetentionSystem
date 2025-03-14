#!/bin/bash

# 检查是否以root权限运行
if [ "$EUID" -ne 0 ]; then 
    echo "请使用root权限运行此脚本"
    exit 1
fi

# 定义服务名称
SERVICE_NAME="stability"

# 停止服务
systemctl stop $SERVICE_NAME

# 禁用服务
systemctl disable $SERVICE_NAME

# 删除服务文件
rm -f /etc/systemd/system/$SERVICE_NAME.service

# 重新加载systemd配置
systemctl daemon-reload

echo "稳定性保持系统服务已卸载" 