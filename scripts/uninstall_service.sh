#!/bin/bash
# scripts/uninstall_service.sh - 稳定性保持系统服务卸载脚本

# 设置颜色输出
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
NC='\033[0m' # No Color

# 检查是否为root用户
if [ "$(id -u)" != "0" ]; then
   echo -e "${RED}此脚本需要root权限，请使用sudo运行${NC}" 1>&2
   exit 1
fi

echo -e "${GREEN}==== 稳定性保持系统服务卸载 ====${NC}"

# 安装目录
INSTALL_DIR="/opt/StabilityRetentionSystem"

# 停止和禁用服务
stop_service() {
    echo -e "${YELLOW}停止服务...${NC}"
    
    # 检查服务是否存在
    if systemctl list-unit-files | grep -q stability-server; then
        # 检查服务是否在运行
        if systemctl is-active --quiet stability-server; then
            # 停止服务
            systemctl stop stability-server
            echo "服务已停止"
        else
            echo "服务未运行"
        fi
        
        # 禁用服务（取消开机自启）
        systemctl disable stability-server
        echo "服务已禁用"
    else
        echo "服务不存在，跳过停止步骤"
    fi
}

# 删除服务文件
remove_service() {
    echo -e "${YELLOW}删除服务文件...${NC}"
    
    if [ -f "/etc/systemd/system/stability-server.service" ]; then
        rm /etc/systemd/system/stability-server.service
        systemctl daemon-reload
        echo "服务文件已删除"
    else
        echo "服务文件不存在，跳过删除步骤"
    fi
}

# 删除安装目录
remove_files() {
    echo -e "${YELLOW}删除安装目录...${NC}"
    
    if [ -d "$INSTALL_DIR" ]; then
        read -p "是否要删除所有系统文件(包括配置文件)? (y/n): " confirm
        if [[ "$confirm" == [yY] || "$confirm" == [yY][eE][sS] ]]; then
            rm -rf $INSTALL_DIR
            echo "安装目录已删除"
        else
            echo "保留安装目录: $INSTALL_DIR"
        fi
    else
        echo "安装目录不存在，跳过删除步骤"
    fi
}

# 主流程
stop_service
remove_service
remove_files

echo -e "${GREEN}==== 稳定性保持系统服务卸载完成 ====${NC}"
echo "服务已从系统中移除"
echo "如果需要完全删除，还应考虑："
echo "1. 移除配置文件（如果选择保留）"
echo "2. 如不再需要，可以卸载依赖库：libcpprest-dev, libspdlog-dev, libfmt-dev 等"

exit 0 