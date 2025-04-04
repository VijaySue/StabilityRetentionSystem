#!/bin/bash
# scripts/install_service.sh - 稳定性保持系统服务安装脚本

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

echo -e "${GREEN}==== 稳定性保持系统服务安装 ====${NC}"

# 安装目录
INSTALL_DIR="/opt/StabilityRetentionSystem"

# 安装依赖项
install_dependencies() {
    echo -e "${YELLOW}安装依赖项...${NC}"
    
    # 安装 OpenSSL
    apt-get update
    apt-get install -y libssl-dev
    
    # 安装 Boost
    apt-get install -y libboost-all-dev
    
    # 安装 C++ REST SDK 和 JSON 库
    apt-get install -y libcpprest-dev
    apt-get install -y nlohmann-json3-dev
    
    # 安装 spdlog 和 fmt
    if [ ! -f "/usr/local/lib/libspdlog.so" ]; then
        echo "安装 spdlog 1.9.2..."
        # 临时目录
        TMP_DIR=$(mktemp -d)
        cd $TMP_DIR
        
        git clone https://github.com/gabime/spdlog.git
        cd spdlog
        git checkout v1.9.2
        mkdir build && cd build
        cmake .. && make -j
        make install
        cd $TMP_DIR
        rm -rf spdlog
    fi
    
    if [ ! -f "/usr/local/lib/libfmt.so" ]; then
        echo "安装 fmt 9.1.0..."
        # 临时目录
        TMP_DIR=$(mktemp -d)
        cd $TMP_DIR
        
        git clone https://github.com/fmtlib/fmt.git
        cd fmt
        git checkout 9.1.0
        mkdir build && cd build
        cmake .. && make -j
        make install
        cd $TMP_DIR
        rm -rf fmt
        cd -
    fi
    
    # 检查 Snap7 是否已安装
    if [ ! -f "/usr/lib/libsnap7.so" ]; then
        echo "安装 Snap7 1.4.0..."
        # 临时目录
        TMP_DIR=$(mktemp -d)
        cd $TMP_DIR
        
        # 下载Snap7
        git clone https://github.com/SCADACS/snap7.git
        cd snap7/build/unix
        make -f x86_64_linux.mk
        cp lib/libsnap7.so /usr/lib/
        cp ../../src/sys/snap7.h /usr/include/
        cd $TMP_DIR
        rm -rf snap7
        cd -
    fi
    
    # 刷新动态库缓存
    ldconfig
    
    echo -e "${GREEN}依赖项安装完成${NC}"
}

# 创建系统目录结构
create_directories() {
    echo -e "${YELLOW}创建系统目录...${NC}"
    
    mkdir -p ${INSTALL_DIR}
    mkdir -p ${INSTALL_DIR}/bin
    mkdir -p ${INSTALL_DIR}/config
    mkdir -p ${INSTALL_DIR}/scripts
    
    echo -e "${GREEN}目录创建完成${NC}"
}

# 安装可执行文件和配置文件
install_files() {
    echo -e "${YELLOW}安装系统文件...${NC}"
    
    # 检查执行路径
    CURRENT_DIR=$(pwd)
    
    # 如果当前目录有bin目录且包含可执行文件，则复制
    if [ -f "${CURRENT_DIR}/bin/stability_server" ]; then
        cp ${CURRENT_DIR}/bin/stability_server ${INSTALL_DIR}/bin/
    else
        echo -e "${RED}错误: 找不到可执行文件 ${CURRENT_DIR}/bin/stability_server${NC}"
        echo "请先编译项目或确保在正确的目录中运行此脚本"
        exit 1
    fi
    
    # 复制配置文件
    if [ -d "${CURRENT_DIR}/config" ]; then
        cp -r ${CURRENT_DIR}/config/* ${INSTALL_DIR}/config/
    else
        # 创建默认配置文件
        echo -e "${YELLOW}未找到配置文件目录，创建默认配置...${NC}"
        cat > ${INSTALL_DIR}/config/config.ini << EOF
[server]
port = 8080               # API 服务监听端口
host = 0.0.0.0            # API 服务监听地址，0.0.0.0表示监听所有接口

[plc]
ip = 192.168.28.57        # PLC设备IP地址,不需要http前缀
port = 102                # PLC设备Modbus TCP端口

[logging]
level = info              # 日志级别：trace, debug, info, warning, error, critical

[edge_system]
address = http://192.168.28.57      # 边缘系统服务器地址
port = 8080                         # 边缘系统服务端口
EOF
    fi
    
    # 复制脚本文件
    cp ${CURRENT_DIR}/scripts/install_service.sh ${INSTALL_DIR}/scripts/
    cp ${CURRENT_DIR}/scripts/uninstall_service.sh ${INSTALL_DIR}/scripts/
    chmod +x ${INSTALL_DIR}/scripts/*.sh
    
    echo -e "${GREEN}文件安装完成${NC}"
}

# 创建systemd服务
create_service() {
    echo -e "${YELLOW}创建systemd服务...${NC}"
    
    cat > /etc/systemd/system/stability-server.service << EOF
[Unit]
Description=Stability Retention System
After=network.target

[Service]
ExecStart=${INSTALL_DIR}/bin/stability_server
WorkingDirectory=${INSTALL_DIR}
Restart=always
User=root
Group=root

[Install]
WantedBy=multi-user.target
EOF
    
    # 重新加载systemd配置
    systemctl daemon-reload
    
    # 启用服务（设置开机自启）
    systemctl enable stability-server
    
    echo -e "${GREEN}服务安装完成并已设置开机自启${NC}"
}

# 启动服务
start_service() {
    echo -e "${YELLOW}启动服务...${NC}"
    
    systemctl start stability-server
    
    # 检查服务状态
    sleep 2
    if systemctl is-active --quiet stability-server; then
        echo -e "${GREEN}服务已成功启动${NC}"
    else
        echo -e "${RED}服务启动失败，请检查状态${NC}"
        systemctl status stability-server --no-pager
    fi
}

# 主流程
install_dependencies
create_directories
install_files
create_service
start_service

echo -e "${GREEN}==== 稳定性保持系统安装完成 ====${NC}"
echo "系统已安装到 ${INSTALL_DIR}"
echo "服务名称: stability-server"
echo "服务管理命令:"
echo "  启动:   sudo systemctl start stability-server"
echo "  停止:   sudo systemctl stop stability-server"
echo "  重启:   sudo systemctl restart stability-server"
echo "  查看状态: sudo systemctl status stability-server"
echo "服务已设置为开机自动启动"

exit 0 