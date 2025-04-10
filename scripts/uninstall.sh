#!/bin/bash

# 定义颜色变量
GREEN='\033[0;32m'
NC='\033[0m' # No Color

# 记录初始状态
INITIAL_STATE_LOG="/tmp/stability_uninstall_$(date +%Y%m%d%H%M%S).log"
echo "记录初始卸载状态..." > ${INITIAL_STATE_LOG}

# 检查是否以root身份运行
if [ "$EUID" -ne 0 ]; then
  echo "请以root身份运行此脚本"
  exit 1
fi

echo "开始卸载稳定性保持系统..."

# 停止并禁用服务
echo "停止并禁用服务..."
systemctl stop stability-system.service 2>/dev/null || true
systemctl disable stability-system.service 2>/dev/null || true

# 删除服务文件
echo "删除服务文件..."
rm -f /lib/systemd/system/stability-system.service
rm -f /usr/lib/systemd/system/stability-system.service
systemctl daemon-reload

# 强制删除配置文件（不询问）
echo "删除配置文件..."
rm -rf /etc/stability-system

# 删除可执行文件
echo "删除可执行文件..."
rm -f /usr/local/bin/stability_server

# 删除日志文件（如果有）
echo "删除日志文件..."
rm -f /var/log/stability-system*.log

# 检查是否还有残留文件
rpm -q stability-retention-system &>/dev/null
if [ $? -eq 0 ]; then
  echo "尝试使用RPM卸载包..."
  rpm -e stability-retention-system --nodeps || true
fi

# 确保所有文件都被删除，即使RPM卸载失败
echo "确保所有文件已删除..."
find /usr/local/bin -name "stability*" -delete
find /etc -name "stability*" -delete
find /lib/systemd/system -name "stability*" -delete
find /usr/lib/systemd/system -name "stability*" -delete

echo -e "${GREEN}卸载完成！系统已清理。${NC}"
echo -e "${GREEN}现在可以重新安装软件包了。${NC}"
echo -e "\n初始卸载状态信息已保存到: ${INITIAL_STATE_LOG}" 