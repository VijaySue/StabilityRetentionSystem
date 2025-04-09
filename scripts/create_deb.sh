#!/bin/bash

# 设置变量
PACKAGE_NAME="stability-retention-system"
VERSION="1.0.0"
ARCH="amd64"
MAINTAINER="VijaySue <your.email@example.com>"
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

# 复制静态编译的可执行文件
cp bin/stability_server ${TEMP_DIR}/usr/local/bin/
chmod +x ${TEMP_DIR}/usr/local/bin/stability_server

# 复制配置文件
cp config/config.ini ${TEMP_DIR}/etc/stability-system/

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
Depends: libssl1.1, libstdc++6
Description: ${DESCRIPTION}
EOL

# 创建安装后脚本
cat > ${TEMP_DIR}/DEBIAN/postinst << EOL
#!/bin/bash
systemctl daemon-reload
systemctl enable stability-system.service
systemctl start stability-system.service
echo "Stability Retention System installed and started successfully!"
exit 0
EOL
chmod +x ${TEMP_DIR}/DEBIAN/postinst

# 创建卸载前脚本
cat > ${TEMP_DIR}/DEBIAN/prerm << EOL
#!/bin/bash
systemctl stop stability-system.service
systemctl disable stability-system.service
echo "Stability Retention System stopped and disabled."
exit 0
EOL
chmod +x ${TEMP_DIR}/DEBIAN/prerm

# 构建DEB包
dpkg-deb --build ${TEMP_DIR} ${PACKAGE_NAME}-${VERSION}-${ARCH}.deb

# 清理临时目录
rm -rf ${TEMP_DIR}

echo "DEB package created: ${PACKAGE_NAME}-${VERSION}-${ARCH}.deb" 