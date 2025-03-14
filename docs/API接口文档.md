# 稳定性保持系统 API 接口文档

## 接口概述

本文档详细描述了稳定性保持系统提供的所有 RESTful API 接口。系统通过这些接口与信息平台进行交互，实现对工业设备的监控和控制。

所有接口均采用 HTTP 协议，数据交换格式为 JSON。接口基础路径为 `/stability`。

## 通用格式

### 请求格式

- **请求方法**：GET/POST
- **内容类型**：application/json
- **字符编码**：UTF-8

### 响应格式

所有接口响应均包含以下字段：

- `msg`: 表示请求处理结果，成功为 "success"，失败为 "error"
- 其他字段：根据具体接口而定

### 错误响应

当请求处理失败时，响应格式如下：

```json
{
    "msg": "error",
    "error": "错误描述信息"
}
```

### 状态码说明

| 状态码 | 说明 |
|-------|------|
| 200 | 请求成功 |
| 400 | 请求参数错误 |
| 401 | 未授权访问 |
| 404 | 请求的资源不存在 |
| 500 | 服务器内部错误 |

## 接口列表

### 1. 系统状态检测

**接口描述**：检测系统是否正常运行

**请求方式**：GET

**请求路径**：`/stability/health`

**请求参数**：无

**响应示例**：

```json
{
    "msg": "success",
    "status": "online",
    "version": "2.0",
    "timestamp": 1686532271000
}
```

**响应参数说明**：

| 参数名 | 类型 | 描述 |
|-------|------|------|
| msg | string | 请求结果，成功为 "success" |
| status | string | 系统状态，值为 "online" |
| version | string | 系统版本号 |
| timestamp | integer | 当前时间戳（毫秒） |

### 2. 系统信息查询

**接口描述**：获取系统基本信息

**请求方式**：GET

**请求路径**：`/stability/system/info`

**请求参数**：无

**响应示例**：

```json
{
    "msg": "success",
    "version": "2.0",
    "buildTime": "2024-03-11 12:00:00",
    "platform": "Ubuntu Server 24.04",
    "dependencies": {
        "cpprestsdk": "2.10.19",
        "nlohmann-json": "3.11.3",
        "spdlog": "1.9.2",
        "fmt": "9.1.0",
        "libmodbus": "3.1.10"
    },
    "plcHost": "192.168.1.10",
    "plcPort": 502
}
```

**响应参数说明**：

| 参数名 | 类型 | 描述 |
|-------|------|------|
| msg | string | 请求结果，成功为 "success" |
| version | string | 系统版本号 |
| buildTime | string | 构建时间 |
| platform | string | 运行平台 |
| dependencies | object | 依赖库版本信息 |
| plcHost | string | PLC 设备主机地址 |
| plcPort | integer | PLC 设备端口号 |

### 3. 设备状态查询

**接口描述**：获取当前设备状态

**请求方式**：GET

**请求路径**：`/stability/device/state`

**请求参数**：

| 参数名 | 类型 | 必填 | 描述 |
|-------|------|-----|------|
| fields | string | 否 | 指定返回的字段，多个字段用逗号分隔 |

**请求示例**：

```
GET /stability/device/state?fields=liftState,liftPressure,operationMode
```

**响应示例**：

```json
{
    "msg": "success",
    "liftState": "停止",
    "liftPressure": 2.5,
    "operationMode": "自动",
    "emergencyStop": "正常",
    "cylinderState": "下降停止",
    "platform1State": "上升停止",
    "platform2State": "下降停止",
    "alarmStatus": "正常",
    "levelingStatus": "停止",
    "heaterStatus": "停止",
    "coolingStatus": "停止",
    "cylinderPressure": 1.5,
    "platform1Pressure": 2.5,
    "platform2Pressure": 2.3,
    "tiltAngle": 0.05,
    "platformPosition": 120,
    "timestamp": 1686532271000,
    "raw_data": {
        "vb_1000": 2,
        "vb_1001": 1
        // ... 更多原始数据
    }
}
```

**响应参数说明**：

| 参数名 | 类型 | 描述 |
|-------|------|------|
| msg | string | 请求结果，成功为 "success" |
| liftState | string | 升降平台状态，可能的值：上升/上升停止/下降/下降停止/停止 |
| liftPressure | number | 升降平台压力值 (MPa) |
| operationMode | string | 操作模式，可能的值：自动/手动 |
| emergencyStop | string | 急停状态，可能的值：复位/急停 |
| cylinderState | string | 刚柔缸状态，可能的值：下降停止/下降加压/上升停止/上升 |
| platform1State | string | 升降平台1状态，可能的值：上升/上升停止/下降/下降停止 |
| platform2State | string | 升降平台2状态，可能的值：上升/上升停止/下降/下降停止 |
| alarmStatus | string | 报警状态，可能的值：油温低/油温高/液位低/液位高/滤芯堵 |
| levelingStatus | string | 电动缸调平状态，可能的值：停止/启动 |
| heaterStatus | string | 电加热状态，可能的值：停止/启动 |
| coolingStatus | string | 风冷状态，可能的值：停止/启动 |
| cylinderPressure | number | 刚柔缸下降停止压力值 (MPa) |
| platform1Pressure | number | 升降平台1上升停止压力值 (MPa) |
| platform2Pressure | number | 升降平台2上升停止压力值 (MPa) |
| tiltAngle | number | 平台倾斜角度 (度) |
| platformPosition | integer | 平台位置信息 (mm) |
| timestamp | integer | 数据时间戳 (毫秒) |
| raw_data | object | PLC原始数据，包含VB和VW地址的原始值 |

### 4. 支撑控制

**接口描述**：控制支撑系统状态

**请求方式**：POST

**请求路径**：`/stability/support/control`

**请求参数**：

| 参数名 | 类型 | 必填 | 描述 |
|-------|------|-----|------|
| taskId | integer | 是 | 任务ID |
| defectId | integer | 是 | 缺陷ID |
| state | string | 是 | 支撑状态，可能的值：`rigid`(刚性支撑) 或 `flexible`(柔性复位) |

**请求示例**：

```json
{
    "taskId": 12345,
    "defectId": 67890,
    "state": "rigid"
}
```

**响应示例**：

```json
{
    "msg": "success",
    "taskId": 12345,
    "defectId": 67890,
    "state": "rigid",
    "status": "processing"
}
```

**响应参数说明**：

| 参数名 | 类型 | 描述 |
|-------|------|------|
| msg | string | 请求结果，成功为 "success" |
| taskId | integer | 任务ID |
| defectId | integer | 缺陷ID |
| state | string | 支撑状态 |
| status | string | 任务状态，值为 "processing" |

**回调说明**：

当任务完成后，系统会向信息平台发送回调请求：

- 请求地址：`/stability/support/cback`（由信息平台提供）
- 请求方式：POST
- 请求参数：

```json
{
    "taskId": 12345,
    "defectId": 67890,
    "state": "success"
}
```

### 5. 平台高度控制

**接口描述**：控制升降平台高度

**请求方式**：POST

**请求路径**：`/stability/platformHeight/control`

**请求参数**：

| 参数名 | 类型 | 必填 | 描述 |
|-------|------|-----|------|
| taskId | integer | 是 | 任务ID |
| defectId | integer | 是 | 缺陷ID |
| platformNum | integer | 是 | 平台编号，值为 1 或 2 |
| state | string | 是 | 控制状态，可能的值：`up`(上升) 或 `down`(下降) |

**请求示例**：

```json
{
    "taskId": 12345,
    "defectId": 67890,
    "platformNum": 1,
    "state": "up"
}
```

**响应示例**：

```json
{
    "msg": "success",
    "taskId": 12345,
    "defectId": 67890,
    "platformNum": 1,
    "state": "up",
    "status": "processing"
}
```

**响应参数说明**：

| 参数名 | 类型 | 描述 |
|-------|------|------|
| msg | string | 请求结果，成功为 "success" |
| taskId | integer | 任务ID |
| defectId | integer | 缺陷ID |
| platformNum | integer | 平台编号 |
| state | string | 控制状态 |
| status | string | 任务状态，值为 "processing" |

**回调说明**：

当任务完成后，系统会向信息平台发送回调请求：

- 请求地址：`/stability/platformHeight/cback`（由信息平台提供）
- 请求方式：POST
- 请求参数：

```json
{
    "taskId": 12345,
    "defectId": 67890,
    "platformNum": 1,
    "state": "success"
}
```

### 6. 平台水平控制

**接口描述**：控制平台水平调平

**请求方式**：POST

**请求路径**：`/stability/platformHorizontal/control`

**请求参数**：

| 参数名 | 类型 | 必填 | 描述 |
|-------|------|-----|------|
| taskId | integer | 是 | 任务ID |
| defectId | integer | 是 | 缺陷ID |
| platformNum | integer | 是 | 平台编号，值为 1 或 2 |
| state | string | 是 | 控制状态，可能的值：`level`(调平) 或 `level_reset`(调平复位) |

**请求示例**：

```json
{
    "taskId": 12345,
    "defectId": 67890,
    "platformNum": 1,
    "state": "level"
}
```

**响应示例**：

```json
{
    "msg": "success",
    "taskId": 12345,
    "defectId": 67890,
    "platformNum": 1,
    "state": "level",
    "status": "processing"
}
```

**响应参数说明**：

| 参数名 | 类型 | 描述 |
|-------|------|------|
| msg | string | 请求结果，成功为 "success" |
| taskId | integer | 任务ID |
| defectId | integer | 缺陷ID |
| platformNum | integer | 平台编号 |
| state | string | 控制状态 |
| status | string | 任务状态，值为 "processing" |

**回调说明**：

当任务完成后，系统会向信息平台发送回调请求：

- 请求地址：`/stability/platformHorizontal/cback`（由信息平台提供）
- 请求方式：POST
- 请求参数：

```json
{
    "taskId": 12345,
    "defectId": 67890,
    "platformNum": 1,
    "state": "success"
}
```

### 7. 电源控制

**接口描述**：控制设备电源

**请求方式**：POST

**请求路径**：`/stability/power/control`

**请求参数**：

| 参数名 | 类型 | 必填 | 描述 |
|-------|------|-----|------|
| state | string | 是 | 电源状态，可能的值：`on`(开启) 或 `off`(关闭) |

**请求示例**：

```json
{
    "state": "on"
}
```

**响应示例**：

```json
{
    "msg": "success",
    "state": "on"
}
```

**响应参数说明**：

| 参数名 | 类型 | 描述 |
|-------|------|------|
| msg | string | 请求结果，成功为 "success" |
| state | string | 电源状态 |

### 8. 电机控制

**接口描述**：控制电机状态

**请求方式**：POST

**请求路径**：`/stability/motor/control`

**请求参数**：

| 参数名 | 类型 | 必填 | 描述 |
|-------|------|-----|------|
| state | string | 是 | 电机状态，可能的值：`start`(启动) 或 `stop`(停止) |

**请求示例**：

```json
{
    "state": "start"
}
```

**响应示例**：

```json
{
    "msg": "success",
    "state": "start"
}
```

**响应参数说明**：

| 参数名 | 类型 | 描述 |
|-------|------|------|
| msg | string | 请求结果，成功为 "success" |
| state | string | 电机状态 |

### 9. 操作模式控制

**接口描述**：控制设备操作模式

**请求方式**：POST

**请求路径**：`/stability/operation/mode`

**请求参数**：

| 参数名 | 类型 | 必填 | 描述 |
|-------|------|-----|------|
| mode | string | 是 | 操作模式，可能的值：`auto`(自动) 或 `manual`(手动) |

**请求示例**：

```json
{
    "mode": "auto"
}
```

**响应示例**：

```json
{
    "msg": "success",
    "mode": "auto"
}
```

**响应参数说明**：

| 参数名 | 类型 | 描述 |
|-------|------|------|
| msg | string | 请求结果，成功为 "success" |
| mode | string | 操作模式 |

### 10. 错误上报

**接口描述**：上报错误信息

**请求方式**：POST

**请求路径**：`/stability/error/report`

**请求参数**：

| 参数名 | 类型 | 必填 | 描述 |
|-------|------|-----|------|
| alarm | string | 是 | 报警信息 |
| source | string | 否 | 报警来源 |
| level | string | 否 | 报警级别，可能的值：info/warning/error/critical |

**请求示例**：

```json
{
    "alarm": "平台1压力过高",
    "source": "platform_control",
    "level": "warning"
}
```

**响应示例**：

```json
{
    "msg": "success",
    "alarm": "平台1压力过高",
    "timestamp": "1686532271000"
}
```

**响应参数说明**：

| 参数名 | 类型 | 描述 |
|-------|------|------|
| msg | string | 请求结果，成功为 "success" |
| alarm | string | 报警信息 |
| timestamp | string | 时间戳 |

## 错误码说明

当请求处理失败时，系统会返回包含错误信息的JSON响应：

```json
{
    "msg": "error",
    "error": "错误描述信息"
}
```

常见错误描述如下：

| 错误描述 | 说明 |
|---------|------|
| 请求参数不完整 | 缺少必要的请求参数 |
| 无效的参数值 | 参数值不符合要求 |
| PLC连接失败 | 与PLC设备的连接失败 |
| 操作超时 | 操作执行超时 |
| 系统内部错误 | 系统内部处理错误 |

## 版本历史

| 版本号 | 更新日期 | 更新内容 |
|-------|---------|---------|
| 1.0 | 2024-3-3 | 初始版本 |
| 1.1 | 2024-3-6 | 更新错误处理机制，增强参数验证 |
| 1.2 | 2024-3-10 | 添加系统信息查询接口，优化设备状态响应格式 |