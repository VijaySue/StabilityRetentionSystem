#!/bin/bash

# 定义颜色变量
GREEN='\033[0;32m'
RED='\033[0;31m'
BLUE='\033[0;34m'
YELLOW='\033[0;33m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# 检查是否以root身份运行
if [ "$EUID" -ne 0 ]; then
  echo -e "${RED}请以root身份运行此脚本${NC}"
  exit 1
fi

echo -e "${GREEN}┌────────────────────────────────────────────────┐${NC}"
echo -e "${GREEN}│       稳定性保持系统 - 安装进行中              │${NC}"
echo -e "${GREEN}└────────────────────────────────────────────────┘${NC}"

# 安装目录
INSTALL_DIR="/usr/local/bin"
CONFIG_DIR="/etc/stability-system"
SERVICE_DIR="/lib/systemd/system"
LOG_DIR="/var/log/stability-system"

# 创建目录
echo -e "${BLUE}[信息]${NC} 创建必要目录..."
mkdir -p ${INSTALL_DIR}
mkdir -p ${CONFIG_DIR}
mkdir -p ${SERVICE_DIR}
mkdir -p ${LOG_DIR}
chmod 755 ${LOG_DIR}

# 当前目录（脚本所在目录的上一级）
CURRENT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

# 复制静态编译的可执行文件
echo -e "${BLUE}[信息]${NC} 安装可执行文件..."
cp ${CURRENT_DIR}/bin/stability_server ${INSTALL_DIR}/
chmod +x ${INSTALL_DIR}/stability_server

# 复制卸载脚本到系统路径
echo -e "${BLUE}[信息]${NC} 安装卸载脚本..."
cp ${CURRENT_DIR}/scripts/uninstall.sh /usr/bin/stability-uninstall
chmod +x /usr/bin/stability-uninstall

# 复制配置文件
echo -e "${BLUE}[信息]${NC} 安装配置文件..."
if [ ! -f "${CONFIG_DIR}/config.ini" ]; then
  cp ${CURRENT_DIR}/config/config.ini ${CONFIG_DIR}/
  echo -e "${GREEN}[成功]${NC} 配置文件已安装"
else
  echo -e "${YELLOW}[警告]${NC} 配置文件已存在，跳过安装以避免覆盖现有配置"
fi

# 创建systemd服务文件
echo -e "${BLUE}[信息]${NC} 创建系统服务..."
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
echo -e "${BLUE}[信息]${NC} 更新systemd服务配置..."
systemctl daemon-reload

# 启用并启动服务
echo -e "${BLUE}[信息]${NC} 启用并启动服务..."
systemctl enable stability-system.service
echo -e "${BLUE}[信息]${NC} 服务已启用，服务名: stability-system"

systemctl start stability-system.service
sleep 2

# 检查服务状态
if systemctl is-active --quiet stability-system.service; then
    echo -e "${GREEN}[成功]${NC} 服务已成功启动！"
else
    echo -e "${YELLOW}[警告]${NC} 服务启动可能存在问题，请检查状态"
fi

echo -e "${BLUE}[信息]${NC} 配置文件位置: ${CONFIG_DIR}/config.ini"
echo -e "${BLUE}[信息]${NC} 日志文件位置: ${LOG_DIR}/"

echo -e "${GREEN}┌────────────────────────────────────────────────┐${NC}"
echo -e "${GREEN}│       稳定性保持系统 - 安装完成                │${NC}"
echo -e "${GREEN}└────────────────────────────────────────────────┘${NC}"

echo -e "使用以下命令管理服务："
echo -e "  启动: ${CYAN}systemctl start stability-system.service${NC}"
echo -e "  停止: ${CYAN}systemctl stop stability-system.service${NC}"
echo -e "  重启: ${CYAN}systemctl restart stability-system.service${NC}"
echo -e "  状态: ${CYAN}systemctl status stability-system.service${NC}"
echo -e "  卸载: ${CYAN}sudo stability-uninstall${NC}" 