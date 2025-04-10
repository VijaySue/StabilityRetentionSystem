#!/bin/bash

# 设置变量
PACKAGE_NAME="stability-retention-system"
VERSION="1.0.0"
RELEASE="1"
ARCH="x86_64"
SUMMARY="Stability Retention System for edge control"
LICENSE="Proprietary"
VENDOR="vijaysue@yeah.net"

# 项目根目录（脚本所在目录的上一级）
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "${ROOT_DIR}"

# 创建RPM构建目录
mkdir -p ~/rpmbuild/{BUILD,RPMS,SOURCES,SPECS,SRPMS}

# 创建临时目录结构
TEMP_DIR="./rpm_build"
PKG_DIR="${TEMP_DIR}/${PACKAGE_NAME}-${VERSION}"
mkdir -p ${PKG_DIR}/{usr/local/bin,etc/stability-system,lib/systemd/system,usr/bin}

# 复制静态编译的可执行文件
cp bin/stability_server ${PKG_DIR}/usr/local/bin/
chmod +x ${PKG_DIR}/usr/local/bin/stability_server

# 复制配置文件
cp config/config.ini ${PKG_DIR}/etc/stability-system/

# 复制卸载脚本
cp scripts/uninstall.sh ${PKG_DIR}/usr/bin/stability-uninstall
chmod +x ${PKG_DIR}/usr/bin/stability-uninstall

# 创建systemd服务文件
cat > ${PKG_DIR}/lib/systemd/system/stability-system.service << EOL
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

# 创建tar源包
echo "Creating source tarball..."
tar -C ${TEMP_DIR} -czf ~/rpmbuild/SOURCES/${PACKAGE_NAME}-${VERSION}.tar.gz ${PACKAGE_NAME}-${VERSION}

# 创建spec文件
cat > ~/rpmbuild/SPECS/${PACKAGE_NAME}.spec << EOL
Name:           ${PACKAGE_NAME}
Version:        ${VERSION}
Release:        ${RELEASE}%{?dist}
Summary:        ${SUMMARY}
License:        ${LICENSE}
URL:            http://example.com
Source0:        %{name}-%{version}.tar.gz
BuildArch:      ${ARCH}
Requires:       openssl, libstdc++

%description
Stability Retention System for monitoring and control of edge computing systems.

%prep
%setup -q

%install
mkdir -p %{buildroot}/usr/local/bin
mkdir -p %{buildroot}/etc/stability-system
mkdir -p %{buildroot}/lib/systemd/system
mkdir -p %{buildroot}/usr/bin

cp usr/local/bin/stability_server %{buildroot}/usr/local/bin/
cp etc/stability-system/config.ini %{buildroot}/etc/stability-system/
cp lib/systemd/system/stability-system.service %{buildroot}/lib/systemd/system/
cp usr/bin/stability-uninstall %{buildroot}/usr/bin/

%files
%attr(755, root, root) /usr/local/bin/stability_server
%attr(755, root, root) /usr/bin/stability-uninstall
%config /etc/stability-system/config.ini
/lib/systemd/system/stability-system.service

%post
systemctl daemon-reload
systemctl enable stability-system.service
systemctl start stability-system.service

%preun
if [ $1 -eq 0 ]; then
    # 仅在完全卸载时执行
    systemctl stop stability-system.service
    systemctl disable stability-system.service
fi

%postun
if [ $1 -eq 0 ]; then
    # 仅在完全卸载时执行
    # 删除可能残留的配置目录
    rm -rf /etc/stability-system
    # 确保服务文件被删除
    rm -f /lib/systemd/system/stability-system.service
    rm -f /usr/lib/systemd/system/stability-system.service
    systemctl daemon-reload
    # 删除日志文件
    rm -f /var/log/stability-system*.log
fi

%changelog
* $(date "+%a %b %d %Y") ${VENDOR} <your.email@example.com> - ${VERSION}-${RELEASE}
- Initial package release
EOL

# 构建RPM包
echo "Building RPM package..."
rpmbuild -bb ~/rpmbuild/SPECS/${PACKAGE_NAME}.spec
BUILD_RESULT=$?

# 检查构建结果
if [ $BUILD_RESULT -ne 0 ]; then
    echo "Error: RPM build failed with exit code $BUILD_RESULT"
    exit $BUILD_RESULT
fi

# 复制生成的RPM包到当前目录
echo "Copying RPM package to current directory..."
find ~/rpmbuild/RPMS -name "${PACKAGE_NAME}*.rpm" -exec cp {} ./ \;
COPY_RESULT=$?

if [ $COPY_RESULT -ne 0 ]; then
    echo "Warning: Could not find or copy the built RPM package"
else
    echo "RPM package created successfully!"
fi

# 清理临时目录
rm -rf ${TEMP_DIR} 