#!/bin/bash

# 检查是否以root身份运行
if [ "$EUID" -ne 0 ]; then
  echo "请以root身份运行此脚本"
  exit 1
fi

# 安装目录
INSTALL_DIR="/usr/local/bin"
CONFIG_DIR="/etc/stability-system"
SERVICE_DIR="/lib/systemd/system"

# 创建目录
mkdir -p ${INSTALL_DIR}
mkdir -p ${CONFIG_DIR}
mkdir -p ${SERVICE_DIR}

# 当前目录（脚本所在目录的上一级）
CURRENT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

# 复制静态编译的可执行文件
echo "安装可执行文件..."
cp ${CURRENT_DIR}/bin/stability_server ${INSTALL_DIR}/
chmod +x ${INSTALL_DIR}/stability_server

# 复制卸载脚本到系统路径
echo "安装卸载脚本..."
cp ${CURRENT_DIR}/scripts/uninstall.sh ${INSTALL_DIR}/stability-uninstall
chmod +x ${INSTALL_DIR}/stability-uninstall

# 复制配置文件
echo "安装配置文件..."
if [ ! -f "${CONFIG_DIR}/config.ini" ]; then
  cp ${CURRENT_DIR}/config/config.ini ${CONFIG_DIR}/
else
  echo "配置文件已存在，跳过安装以避免覆盖现有配置..."
fi

# 创建systemd服务文件
echo "创建系统服务..."
cat > ${SERVICE_DIR}/stability-system.service << EOL
[Unit]
Description=Stability Retention System Service
After=network.target

[Service]
Type=simple
User=root
ExecStart=${INSTALL_DIR}/stability_server
WorkingDirectory=${INSTALL_DIR}
Restart=on-failure
RestartSec=5

[Install]
WantedBy=multi-user.target
EOL

# 重新加载systemd
systemctl daemon-reload

# 启用并启动服务
echo "启用并启动服务..."
systemctl enable stability-system.service
systemctl start stability-system.service

echo "安装完成！"
echo "服务状态："
systemctl status stability-system.service --no-pager

echo "----------------------------------------------"
echo "使用以下命令管理服务："
echo "启动: systemctl start stability-system.service"
echo "停止: systemctl stop stability-system.service"
echo "重启: systemctl restart stability-system.service"
echo "状态: systemctl status stability-system.service"
echo "卸载: 运行 stability-uninstall 命令" 