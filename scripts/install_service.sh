#!/bin/bash

# 检查是否以root权限运行
if [ "$EUID" -ne 0 ]; then 
    echo "请使用root权限运行此脚本"
    exit 1
fi

# 创建系统服务文件
cat > /etc/systemd/system/stability-server.service << 'EOL'
[Unit]
Description=Stability Retention System Server
After=network.target

[Service]
Type=simple
User=root
Group=root
WorkingDirectory=/opt/StabilityRetentionSystem
ExecStart=/opt/StabilityRetentionSystem/bin/stability_server
Restart=always
RestartSec=3

[Install]
WantedBy=multi-user.target
EOL

# 重新加载systemd配置
systemctl daemon-reload

# 启用服务
systemctl enable stability-server

echo "系统服务安装完成。"
echo "使用以下命令管理服务："
echo "启动服务：sudo systemctl start stability-server"
echo "停止服务：sudo systemctl stop stability-server"
echo "重启服务：sudo systemctl restart stability-server"
echo "查看状态：sudo systemctl status stability-server" 