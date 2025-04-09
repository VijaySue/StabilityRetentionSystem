# StabilityRetentionSystem 稳定性保持系统

## 项目概述

稳定性保持系统是一套用于控制稳定平台的自动化系统，通过与PLC设备集成，实现对平台升降、水平调整和刚柔支撑等功能的精确控制。本系统提供基于HTTP的REST API接口，允许上层应用通过网络远程控制和监控平台状态。

本系统使用C++语言开发，基于C++ REST SDK提供Web服务，使用Snap7库实现与西门子S7 PLC设备的通信，在Linux操作系统上运行。

## 主要功能

- **设备状态监控**：实时监控平台工作状态、压力参数、位置信息和报警状态
- **支撑控制**：实现刚性支撑和柔性复位功能
- **平台高度控制**：对两个独立的升降平台进行上升和下降控制
- **平台水平调整**：自动调平功能，保持平台水平
- **异步任务管理**：长时间运行的操作通过异步任务方式管理
- **错误上报**：提供错误上报接口和完善的异常处理机制

## 系统架构

系统采用模块化设计，主要包含以下组件：

- **HTTP服务器**：基于C++ REST SDK实现，提供RESTful API接口
- **API处理器**：处理HTTP请求，验证参数，调用相应的服务
- **PLC通信模块**：基于Snap7实现与西门子S7 PLC设备的通信
- **设备状态管理器**：实时获取和解析设备状态数据
- **任务管理器**：管理异步任务的创建、执行、状态跟踪
- **配置管理器**：管理系统配置参数
  - 采用单例模式设计，确保全局唯一配置实例
  - 支持从INI格式的配置文件加载配置
  - 提供服务器、PLC等核心配置项
  - 支持配置项默认值，确保系统在配置文件缺失时仍能正常运行
  - 支持运行时动态加载配置

## 技术栈

- **编程语言**：C++17
- **Web框架**：C++ REST SDK (cpprestsdk) 2.10.19
- **PLC通信**：Snap7 1.4.0
- **日志系统**：spdlog 1.9.2
- **JSON处理**：nlohmann/json 3.11.3
- **格式化库**：fmt 9.1.0
- **编译器要求**：GCC 11+ 或 Clang 14.0+

## 快速开始

### 系统要求

- **开发环境**：
  
  - 操作系统：Windows 11
  - IDE：Visual Studio 2022
  - 构建工具：Make
  - 包管理器：vcpkg
  - 版本控制：Git

- **运行环境**：
  
  - 操作系统：Ubuntu Server 24.04
  - 编译器：GCC 11+
  - 构建工具：Make
  - 包管理器：vcpkg

- **硬件要求**：
  
  - 最低配置：双核CPU，4GB内存，100MB磁盘空间
  - 推荐配置：四核CPU，8GB内存，1GB磁盘空间

### 安装依赖项

```bash
# 安装 OpenSSL
sudo apt-get update
sudo apt-get install -y libssl-dev

# 安装 Boost
sudo apt-get install -y libboost-all-dev

# 安装 C++ REST SDK 和 JSON 库
# 使用vcpkg安装
vcpkg install cpprestsdk:x64-linux
vcpkg install nlohmann-json:x64-linux

# 安装 spdlog
git clone https://github.com/gabime/spdlog.git
cd spdlog
git checkout v1.9.2
mkdir build && cd build
cmake .. && make -j
sudo make install

# 安装fmt
git clone https://github.com/fmtlib/fmt.git
cd fmt
git checkout 9.1.0
mkdir build && cd build
cmake .. && make -j
sudo make install

# 安装 Snap7 西门子通信库
git clone https://github.com/SCADACS/snap7.git
cd snap7/build/unix
make -f x86_64_linux.mk
sudo cp lib/libsnap7.so /usr/lib/
sudo cp ../../src/sys/snap7.h /usr/include/

# 刷新动态库缓存
sudo ldconfig
```

### 编译构建

使用Makefile进行项目构建：

```bash
# 编译项目
make

# 清理项目
make clean

# 安装到系统目录
sudo make install
```

### 配置文件说明

配置 `config/config.ini` 文件，根据实际环境修改参数：

```ini
[server]
port = 8080               # API 服务监听端口
host = 0.0.0.0            # API 服务监听地址，0.0.0.0表示监听所有接口

[plc]
ip = 127.0.0.1        # PLC设备IP地址,不需要http前缀
port = 102                # PLC设备Modbus TCP端口

[logging]
level = info              # 日志级别：trace, debug, info, warning, error, critical

[edge_system]
url = http://127.0.0.1:8080/    # 边缘系统完整URL（包含协议、地址和端口）
```

系统通过 `config/config.ini` 文件进行配置，主要参数包括：

| 段落          | 参数    | 说明                | 默认值                   |
| ----------- | ----- | ----------------- | --------------------- |
| server      | port  | HTTP服务器端口         | 8080                  |
| server      | host  | 服务器监听地址           | 0.0.0.0               |
| plc         | ip    | PLC设备IP地址         | 192.168.28.57         |
| plc         | port  | PLC设备Modbus TCP端口 | 102                   |
| logging     | level | 日志级别              | info                  |
| edge_system | url   | 边缘系统完整URL         | http://127.0.0.1:8080 |

### 常见问题

1. **无法连接到PLC设备**
   
   - 检查PLC设备IP、机架号和槽号是否正确
   - 确认网络连接是否正常
   - 检查防火墙设置是否阻止了通信

2. **系统启动失败**
   
   - 确认配置文件格式正确
   - 检查服务器端口是否被占用

3. **API响应错误**
   
   - 检查请求参数格式是否正确
   - 确认API访问路径是否正确

## 项目文档

详细的项目文档包括：

- [API接口文档](docs/API接口文档.md)：详细的API接口说明
- [部署指南](docs/部署指南.md)：详细的安装和部署说明
- [交付清单](docs/交付清单.md)：系统交付项目清单

## 开发指南

### 代码结构

项目目录结构如下：

```
StabilityRetentionSystem/
├── bin/                 # 编译后的可执行文件目录
│   └── x64/
│       ├── Debug/       # 调试版本可执行文件
│       └── Release/     # 发布版本可执行文件
├── config/              # 配置文件目录
│   └── config.ini       # 主配置文件
├── docs/                # 文档目录
│   ├── API接口文档.md     # API接口详细文档
│   ├── 部署指南.md        # 部署和安装指南
│   ├── 交付清单.md        # 系统交付项目清单
│   ├── 控制系统变量对应表.xlsx # 控制系统变量映射表
│   └── 信息平台对稳定性保持系统的需求.docx # 需求文档
├── include/             # 头文件目录
│   ├── alarm_monitor.h   # 报警监控模块
│   ├── callback_client.h # 回调客户端
│   ├── common.h          # 公共功能和工具
│   ├── config_manager.h  # 配置管理模块
│   ├── plc_manager.h     # PLC通信模块
│   ├── server.h          # HTTP服务器模块
│   ├── snap7.h           # Snap7库头文件
│   └── task_manager.h    # 任务管理模块
├── obj/                 # 编译中间文件目录
│   └── x64/             # x64平台编译中间文件
├── scripts/             # 脚本文件目录
│   ├── create_deb.sh     # 创建DEB包脚本
│   ├── create_packages.sh # 创建安装包脚本
│   ├── create_rpm.sh     # 创建RPM包脚本
│   ├── install.sh        # 安装脚本
│   └── uninstall.sh      # 卸载脚本
├── src/                 # 源代码目录
│   ├── alarm_monitor.cpp  # 报警监控模块实现
│   ├── callback_client.cpp # 回调客户端实现
│   ├── common.cpp          # 公共功能实现
│   ├── config_manager.cpp  # 配置管理实现
│   ├── main.cpp            # 程序入口点
│   ├── plc_manager.cpp     # PLC通信实现
│   ├── server.cpp          # HTTP服务器实现
│   ├── snap7.cpp           # Snap7库实现
│   └── task_manager.cpp    # 任务管理实现
├── tools/               # 工具目录
│   ├── TestTcpClient/    # TCP测试客户端工具
│   │   ├── client_test.py  # 客户端测试脚本
│   │   ├── server_test.py  # 服务器测试脚本
│   │   ├── .idea/          # PyCharm项目配置
│   │   ├── .venv/          # Python虚拟环境
│   │   ├── docs/           # 测试工具文档
│   │   └── TestTcpClient/  # 测试工具模块
│   └── HslCommunicationDemo/ # HSL通信演示工具
│       ├── HslCommunicationDemo.exe # 西门子通信测试工具
│       ├── HslCommunication.dll     # 通信库
│       ├── HslCommunication.xml     # 库文档
│       ├── HslControls.dll          # 控件库
│       ├── DynamicExpresso.Core.dll # 依赖库
│       ├── libcrypto-3-x64.dll      # 加密库
│       ├── libssl-3-x64.dll         # SSL库
│       ├── Newtonsoft.Json.dll      # JSON处理库
│       ├── Upgrade.exe              # 升级工具
│       ├── PCOMM.DLL                # 通信库
│       ├── WeifenLuo.WinFormsUI.Docking.dll       # UI库
│       ├── WeifenLuo.WinFormsUI.Docking.ThemeVS2015.dll # UI主题库
│       └── newVersionIngored.txt    # 版本忽略配置
├── .git/                # Git版本控制目录
├── .vs/                 # Visual Studio配置目录
├── .gitattributes       # Git属性文件
├── .gitignore           # Git忽略文件
├── LICENSE.txt          # 许可证文件
├── makefile             # 项目构建文件
├── README.md            # 项目说明文档
├── StabilityRetentionSystem.sln        # Visual Studio解决方案文件
├── StabilityRetentionSystem.vcxproj    # Visual Studio项目文件
├── StabilityRetentionSystem.vcxproj.filters  # Visual Studio项目筛选器文件
└── StabilityRetentionSystem.vcxproj.user     # Visual Studio用户配置文件
```

主要模块说明：

- **HTTP服务器模块**：基于C++ REST SDK实现，提供RESTful API接口
- **PLC通信模块**：基于Snap7实现与西门子S7 PLC设备的通信
- **报警监控模块**：监控设备报警状态并进行处理
- **任务管理模块**：管理异步任务的创建、执行和状态跟踪
- **配置管理模块**：管理系统配置参数，支持从INI文件加载配置
- **公共功能模块**：提供错误处理等通用功能
- **回调客户端模块**：实现与上层系统的回调通信

### 编译构建

使用Makefile进行项目构建：

```bash
# 编译项目
make

# 清理项目
make clean
```

### 贡献指南

如果您想为项目做出贡献，请遵循以下步骤：

1. Fork本仓库
2. 创建您的特性分支 (`git checkout -b feature/amazing-feature`)
3. 提交您的更改 (`git commit -m 'Add some amazing feature'`)
4. 推送到分支 (`git push origin feature/amazing-feature`)
5. 开启一个Pull Request

## 许可证

本项目采用MIT许可证 - 查看 [LICENSE](LICENSE) 文件了解详情。

## 联系与支持

如有问题或需要技术支持，请联系：

- 技术支持团队：vijaysue@yeah.net
- 项目主页：[VijaySue/StabilityRetentionSystem](https://github.com/VijaySue/StabilityRetentionSystem)

## 版本历史

| 版本    | 日期        | 变更内容                               |
| ----- | --------- | ---------------------------------- |
| 1.0.0 | 2024-3-3  | 初始版本发布                             |
| 1.1.0 | 2024-3-6  | 增加平台2控制功能                          |
| 1.2.0 | 2024-3-9  | 优化任务管理，增强错误处理                      |
| 2.0.0 | 2024-3-28 | 重构API接口，提升性能，更新开发环境，使用Snap7实现PLC通信 |
