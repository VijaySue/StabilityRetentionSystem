#!/bin/bash

# 设置变量
PACKAGE_NAME="stability-retention-system"
VERSION="1.0.0"
ARCH="amd64"
MAINTAINER="VijaySue <vijaysue@yeah.net>"
DESCRIPTION="Stability Retention System for edge control"

# 项目根目录（脚本所在目录的上一级）
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "${ROOT_DIR}"

# 创建临时目录结构
TEMP_DIR="./debian_package"
mkdir -p ${TEMP_DIR}/DEBIAN
mkdir -p ${TEMP_DIR}/usr/local/bin
mkdir -p ${TEMP_DIR}/etc/stability-system
mkdir -p ${TEMP_DIR}/lib/systemd/system
mkdir -p ${TEMP_DIR}/usr/bin
mkdir -p ${TEMP_DIR}/var/log/stability-system

# 复制静态编译的可执行文件
cp bin/stability_server ${TEMP_DIR}/usr/local/bin/
chmod +x ${TEMP_DIR}/usr/local/bin/stability_server

# 复制配置文件
cp config/config.ini ${TEMP_DIR}/etc/stability-system/

# 复制卸载脚本
cp scripts/uninstall.sh ${TEMP_DIR}/usr/bin/stability-uninstall
chmod +x ${TEMP_DIR}/usr/bin/stability-uninstall

# 创建systemd服务文件
cat > ${TEMP_DIR}/lib/systemd/system/stability-system.service << EOL
[Unit]
Description=Stability Retention System Service
After=network.target

[Service]
Type=simple
User=root
ExecStart=/usr/local/bin/stability_server
WorkingDirectory=/usr/local/bin
Restart=on-failure
RestartSec=5

[Install]
WantedBy=multi-user.target
EOL

# 创建控制文件
cat > ${TEMP_DIR}/DEBIAN/control << EOL
Package: ${PACKAGE_NAME}
Version: ${VERSION}
Section: utils
Priority: optional
Architecture: ${ARCH}
Maintainer: ${MAINTAINER}
Depends: libssl | libssl1.1 | libssl3, libstdc++6
Provides: ${PACKAGE_NAME}
Replaces: ${PACKAGE_NAME}
Description: ${DESCRIPTION}
 Stability Retention System for monitoring and control of edge computing systems.
 This package provides a complete solution for maintaining stability 
 in edge computing environments.
EOL

# 创建安装后脚本
cat > ${TEMP_DIR}/DEBIAN/postinst << EOL
#!/bin/bash

# 定义颜色变量
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[0;33m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

echo -e "\${GREEN}┌────────────────────────────────────────────────┐\${NC}"
echo -e "\${GREEN}│       稳定性保持系统 - 安装进行中              │\${NC}"
echo -e "\${GREEN}└────────────────────────────────────────────────┘\${NC}"

echo -e "\${BLUE}[信息]\${NC} 正在配置系统服务..."
systemctl daemon-reload
systemctl enable stability-system.service
echo -e "\${BLUE}[信息]\${NC} 服务已启用，服务名: stability-system"

echo -e "\${BLUE}[信息]\${NC} 正在启动服务..."
systemctl start stability-system.service
sleep 2

# 检查服务状态
if systemctl is-active --quiet stability-system.service; then
    echo -e "\${GREEN}[成功]\${NC} 服务已成功启动！"
else
    echo -e "\${YELLOW}[警告]\${NC} 服务启动可能存在问题，请检查状态。"
fi

# 确保日志目录存在并设置权限
mkdir -p /var/log/stability-system
chmod 755 /var/log/stability-system

echo -e "\${BLUE}[信息]\${NC} 配置文件位置: /etc/stability-system/config.ini"
echo -e "\${BLUE}[信息]\${NC} 日志文件位置: /var/log/stability-system/"

echo -e "\${GREEN}┌────────────────────────────────────────────────┐\${NC}"
echo -e "\${GREEN}│       稳定性保持系统 - 安装完成                │\${NC}"
echo -e "\${GREEN}└────────────────────────────────────────────────┘\${NC}"

echo -e "使用以下命令管理服务："
echo -e "  启动: \${CYAN}systemctl start stability-system.service\${NC}"
echo -e "  停止: \${CYAN}systemctl stop stability-system.service\${NC}"
echo -e "  重启: \${CYAN}systemctl restart stability-system.service\${NC}"
echo -e "  状态: \${CYAN}systemctl status stability-system.service\${NC}"
echo -e "  卸载: \${CYAN}sudo /usr/bin/stability-uninstall\${NC}"

exit 0
EOL
chmod +x ${TEMP_DIR}/DEBIAN/postinst

# 创建卸载前脚本
cat > ${TEMP_DIR}/DEBIAN/prerm << EOL
#!/bin/bash

# 定义颜色变量
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo -e "\${BLUE}[信息]\${NC} 停止并禁用服务..."
systemctl stop stability-system.service
systemctl disable stability-system.service
echo -e "\${BLUE}[信息]\${NC} 稳定性保持系统服务已停止和禁用。"

exit 0
EOL
chmod +x ${TEMP_DIR}/DEBIAN/prerm

# 创建卸载后脚本
cat > ${TEMP_DIR}/DEBIAN/postrm << EOL
#!/bin/bash

# 定义颜色变量
GREEN='\033[0;32m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# 删除可能残留的配置目录和日志
if [ "\$1" = "purge" ]; then
    echo -e "\${BLUE}[信息]\${NC} 清理配置文件和日志..."
    rm -rf /etc/stability-system
    rm -rf /var/log/stability-system
fi

# 重新加载systemd以移除服务
systemctl daemon-reload

echo -e "\${GREEN}[信息]\${NC} 稳定性保持系统已完全卸载"

exit 0
EOL
chmod +x ${TEMP_DIR}/DEBIAN/postrm

# 创建conffiles文件以标记配置文件
cat > ${TEMP_DIR}/DEBIAN/conffiles << EOL
/etc/stability-system/config.ini
EOL

# 设置目录权限
chmod 755 ${TEMP_DIR}/var/log/stability-system

# 构建DEB包
dpkg-deb --build ${TEMP_DIR} ${PACKAGE_NAME}-${VERSION}-${ARCH}.deb

# 清理临时目录
rm -rf ${TEMP_DIR}

echo -e "\033[0;32m[成功]\033[0m DEB包已创建: ${PACKAGE_NAME}-${VERSION}-${ARCH}.deb"
echo -e "\033[0;34m[信息]\033[0m 使用以下命令安装: \033[0;36msudo dpkg -i ${PACKAGE_NAME}-${VERSION}-${ARCH}.deb\033[0m"
echo -e "\033[0;34m[信息]\033[0m 如果有依赖问题，请运行: \033[0;36msudo apt-get -f install\033[0m" 