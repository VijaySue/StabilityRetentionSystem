#!/bin/bash

# 检查是否以root权限运行
if [ "$EUID" -ne 0 ]; then 
    echo "请使用root权限运行此脚本"
    exit 1
fi

# 定义服务名称和安装路径
SERVICE_NAME="stability"
INSTALL_PATH="/opt/stability"

# 创建服务文件
cat > /etc/systemd/system/$SERVICE_NAME.service << EOL
[Unit]
Description=Stability Retention System Service
After=network.target

[Service]
Type=simple
User=root
WorkingDirectory=$INSTALL_PATH
ExecStart=$INSTALL_PATH/bin/stability_server
Restart=always
RestartSec=3

[Install]
WantedBy=multi-user.target
EOL

# 重新加载systemd配置
systemctl daemon-reload

# 启用服务
systemctl enable $SERVICE_NAME

# 启动服务
systemctl start $SERVICE_NAME

echo "稳定性保持系统服务已安装并启动"
echo "使用以下命令查看服务状态："
echo "systemctl status $SERVICE_NAME" 