#!/bin/bash

# 项目根目录
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "${ROOT_DIR}"

# 设置错误处理
set -e

echo "----------------------------------------------"
echo "开始创建Linux安装包..."
echo "----------------------------------------------"

# 检查依赖
echo "检查安装包构建依赖..."
if ! command -v dpkg-deb &> /dev/null; then
    echo "警告: dpkg-deb 未找到，无法创建 DEB 包"
    CAN_BUILD_DEB=false
else
    CAN_BUILD_DEB=true
fi

if ! command -v rpmbuild &> /dev/null; then
    echo "警告: rpmbuild 未找到，无法创建 RPM 包"
    CAN_BUILD_RPM=false
else
    CAN_BUILD_RPM=true
fi

# 确保 obj 和 bin 目录存在
mkdir -p obj bin

# 执行静态编译 - 使用较少的并行编译数量降低内存使用
echo "执行静态编译..."
make clean
make static -j2  # 改为只使用2个CPU核心进行并行编译

# 检查编译是否成功
if [ ! -f "bin/stability_server" ]; then
    echo "错误: 编译失败！请检查编译错误。"
    exit 1
fi

# 创建发布目录
RELEASE_DIR="release"
mkdir -p "${RELEASE_DIR}"

# 创建自解压安装包
echo "创建自解压安装包..."
SELF_EXTRACT="${RELEASE_DIR}/stability-system-installer.sh"

# 创建自解压安装包头部
cat > "${SELF_EXTRACT}" << 'EOL'
#!/bin/bash

# 自解压安装包

# 临时目录
TEMP_DIR=$(mktemp -d)
ARCHIVE_MARKER="__ARCHIVE_BELOW__"

# 解压文件
tail -n +$(grep -n "${ARCHIVE_MARKER}" "$0" | cut -d ':' -f 1 | head -n 1) "$0" | tar xz -C "${TEMP_DIR}"

# 切换到解压目录并运行安装脚本
cd "${TEMP_DIR}"
if [ "$EUID" -ne 0 ]; then
    echo "需要root权限安装。请使用sudo或以root身份运行。"
    sudo ./install.sh
else
    ./install.sh
fi

# 清理
rm -rf "${TEMP_DIR}"
exit 0

__ARCHIVE_BELOW__
EOL

# 创建临时目录
PACK_DIR=$(mktemp -d)
cp bin/stability_server "${PACK_DIR}/"
cp -r config "${PACK_DIR}/"
cp scripts/install.sh "${PACK_DIR}/"
cp scripts/uninstall.sh "${PACK_DIR}/"
chmod +x "${PACK_DIR}/install.sh"
chmod +x "${PACK_DIR}/uninstall.sh"

# 打包文件到自解压安装包
( cd "${PACK_DIR}" && tar czf - * ) >> "${SELF_EXTRACT}"
chmod +x "${SELF_EXTRACT}"
rm -rf "${PACK_DIR}"

echo "创建的自解压安装包: ${SELF_EXTRACT}"

# 创建DEB包
DEB_SUCCESS=false
if [ "${CAN_BUILD_DEB}" = true ]; then
    echo "创建DEB包..."
    if bash ./scripts/create_deb.sh; then
        if mv *.deb "${RELEASE_DIR}/" 2>/dev/null; then
            echo "DEB包已移动到 ${RELEASE_DIR}/ 目录"
            DEB_SUCCESS=true
        else
            echo "警告：无法找到或移动生成的DEB包"
        fi
    else
        echo "错误：DEB包创建失败"
    fi
fi

# 创建RPM包
RPM_SUCCESS=false
if [ "${CAN_BUILD_RPM}" = true ]; then
    echo "创建RPM包..."
    if bash ./scripts/create_rpm.sh; then
        if mv *.rpm "${RELEASE_DIR}/" 2>/dev/null; then
            echo "RPM包已移动到 ${RELEASE_DIR}/ 目录"
            RPM_SUCCESS=true
        else
            echo "警告：无法找到或移动生成的RPM包"
        fi
    else
        echo "错误：RPM包创建失败"
    fi
fi

echo "----------------------------------------------"
echo "完成！所有安装包已创建在 ${RELEASE_DIR}/ 目录"
if [ "${DEB_SUCCESS}" = true ]; then
    echo "✓ DEB包创建成功"
else
    echo "✗ DEB包创建失败或跳过"
fi
if [ "${RPM_SUCCESS}" = true ]; then
    echo "✓ RPM包创建成功"
else
    echo "✗ RPM包创建失败或跳过"
fi
echo "----------------------------------------------" 