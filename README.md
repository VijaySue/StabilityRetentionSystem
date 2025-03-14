# 稳定性保持系统

## 项目概述

稳定性保持系统是一套用于控制稳定平台的自动化系统，通过与PLC设备集成，实现对平台升降、水平调整和刚柔支撑等功能的精确控制。本系统提供基于HTTP的REST API接口，允许上层应用通过网络远程控制和监控平台状态。

本系统使用C++语言开发，基于C++ REST SDK提供Web服务，使用libmodbus库实现与PLC设备的通信，在Linux操作系统上运行。

## 主要功能

- **设备状态监控**：实时监控平台工作状态、压力参数、位置信息和报警状态
- **支撑控制**：实现刚性支撑和柔性复位功能
- **平台高度控制**：对两个独立的升降平台进行上升和下降控制
- **平台水平调整**：自动调平功能，保持平台水平
- **电源控制**：远程控制系统电源开关
- **电机控制**：远程启动和停止油泵电机
- **操作模式切换**：支持自动和手动模式切换
- **异步任务管理**：长时间运行的操作通过异步任务方式管理
- **错误上报**：提供错误上报接口和完善的异常处理机制

## 系统架构

系统采用模块化设计，主要包含以下组件：

- **HTTP服务器**：基于C++ REST SDK实现，提供RESTful API接口
- **API处理器**：处理HTTP请求，验证参数，调用相应的服务
- **PLC通信模块**：基于libmodbus实现与PLC设备的通信
- **设备状态管理器**：实时获取和解析设备状态数据
- **任务管理器**：管理异步任务的创建、执行、状态跟踪
- **配置管理器**：管理系统配置参数
- **日志系统**：基于spdlog实现的多级日志记录系统

## 技术栈

- **编程语言**：C++17
- **Web框架**：C++ REST SDK (cpprestsdk) 2.10.18
- **PLC通信**：libmodbus 3.1.6
- **日志系统**：spdlog 1.10.0
- **JSON处理**：nlohmann/json 3.11.2
- **配置解析**：自定义INI解析器
- **编译器要求**：GCC 9.0+ 或 Clang 10.0+

## 快速开始

### 系统要求

- **操作系统**：Linux (Ubuntu 20.04/22.04)
- **硬件要求**：
  - 最低配置：双核CPU，4GB内存，100MB磁盘空间
  - 推荐配置：四核CPU，8GB内存，1GB磁盘空间
- **依赖项**：
  - libmodbus-dev
  - OpenSSL
  - Boost (cpprestsdk依赖)

### 安装依赖项

```bash
# Ubuntu/Debian
sudo apt update
sudo apt install -y libmodbus-dev libssl-dev libboost-all-dev libcpprest-dev spdlog-dev g++

# CentOS/RHEL
sudo yum install -y epel-release
sudo yum install -y libmodbus-devel openssl-devel boost-devel gcc-c++
# 对于CentOS，可能需要从源码编译cpprestsdk和spdlog
```

### 编译构建

使用Makefile进行编译构建：

```bash
# 编译项目
make

# 清理项目
make clean

# 重新构建
make clean all
```

### 运行系统

1. 配置 `config.ini` 文件，根据实际环境修改参数：

```ini
[server]
port=8080
threads=4

[plc]
ip=192.168.1.10
port=502
polling_interval_ms=100

[logging]
level=info
file=logs/stability_system.log
```

2. 运行系统：

```bash
# 如果已安装
./bin/stability_server

# 或直接从项目目录运行
./stability_server
```

### 配置文件说明

系统通过 `config.ini` 文件进行配置，主要参数包括：

| 段落 | 参数 | 说明 | 默认值 |
|------|------|------|---------|
| server | port | HTTP服务器端口 | 8080 |
| server | threads | 服务器线程数 | 4 |
| plc | ip | PLC设备IP地址 | 192.168.1.10 |
| plc | port | PLC设备端口 | 502 |
| plc | polling_interval_ms | PLC数据轮询间隔(毫秒) | 100 |
| logging | level | 日志级别(debug/info/warn/error) | info |
| logging | file | 日志文件路径 | logs/stability_system.log |

### API使用示例

1. 获取系统状态：

```bash
curl -X GET http://localhost:8080/api/v1/system/status
```

2. 发送支撑控制命令：

```bash
curl -X POST http://localhost:8080/api/v1/control/support \
  -H "Content-Type: application/json" \
  -d '{"taskId": 12345, "defectId": 67890, "state": "rigid"}'
```

3. 控制平台高度：

```bash
curl -X POST http://localhost:8080/api/v1/control/platform/height \
  -H "Content-Type: application/json" \
  -d '{"taskId": 12345, "defectId": 67890, "platformNum": 1, "state": "up"}'
```

## 故障排除

### 常见问题

1. **无法连接到PLC设备**
   - 检查PLC设备IP和端口是否正确
   - 确认网络连接是否正常
   - 检查防火墙设置是否阻止了通信

2. **系统启动失败**
   - 查看日志文件中的错误信息
   - 确认配置文件格式正确
   - 检查服务器端口是否被占用

3. **API响应错误**
   - 检查请求参数格式是否正确
   - 确认API访问路径是否正确
   - 查看服务器日志了解详细错误原因

## 项目文档

详细的项目文档包括：

- [需求分析文档](需求分析文档.md)：系统需求分析和功能规格
- [API接口文档](API接口文档.md)：详细的API接口说明
- [技术实现方案](技术实现方案.md)：系统设计和技术实现详情
- [部署指南](部署指南.md)：详细的安装和部署说明
- [交付清单](交付清单.md)：系统交付项目清单
- [数据结构说明](数据结构说明.md)：系统数据结构和PLC地址映射

## 开发指南

### 代码结构

项目主要代码如下：

- main.cpp：程序入口点
- server.h/server.cpp：HTTP服务器模块
- plc_manager.h/plc_manager.cpp：PLC通信模块
- task_manager.h/task_manager.cpp：任务管理模块
- common.h/common.cpp：公共功能和工具
- callback_client.h/callback_client.cpp：回调客户端

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

| 版本 | 日期 | 变更内容 |
|------|------|---------|
| 1.0.0 | 2024-3-3 | 初始版本发布 |
| 1.1.0 | 2024-3-6 | 增加平台2控制功能 |
| 1.2.0 | 2024-3-9 | 优化任务管理，增强错误处理 |
| 2.0.0 | 2024-3-11 | 重构API接口，提升性能 |