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
    "status": "online",
    "version": "1.0.0",
    "timestamp": 1686532271000
}
```

**响应参数说明**：

| 参数名 | 类型 | 描述 |
|-------|------|------|
| status | string | 系统状态，值为 "online" |
| version | string | 系统版本号 |
| timestamp | integer | 当前时间戳（毫秒） |

### 2. 刚性支撑/柔性复位控制

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

### 3. 作业平台升高/复位控制

**接口描述**：控制平台升降高度

**请求方式**：POST

**请求路径**：`/stability/platformHeight/control`

**请求参数**：

| 参数名 | 类型 | 必填 | 描述 |
|-------|------|-----|------|
| taskId | integer | 是 | 任务ID |
| defectId | integer | 是 | 缺陷ID |
| platformNum | integer | 是 | 平台编号，1或2 |
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

### 4. 作业平台调平/调平复位控制

**接口描述**：控制平台调平状态

**请求方式**：POST

**请求路径**：`/stability/platformHorizontal/control`

**请求参数**：

| 参数名 | 类型 | 必填 | 描述 |
|-------|------|-----|------|
| taskId | integer | 是 | 任务ID |
| defectId | integer | 是 | 缺陷ID |
| platformNum | integer | 是 | 平台编号，1或2 |
| state | string | 是 | 控制状态，可能的值：`leveling`(调平) 或 `reset`(调平复位) |

**请求示例**：

```json
{
    "taskId": 12345,
    "defectId": 67890,
    "platformNum": 1,
    "state": "leveling"
}
```

**响应示例**：

```json
{
    "msg": "success",
    "taskId": 12345,
    "defectId": 67890,
    "platformNum": 1,
    "state": "leveling",
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

### 5. 实时状态获取

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
    "operationMode": "自动",
    "emergencyStop": "正常",
    "oilPumpStatus": "停止",
    "cylinderState": "下降停止",
    "platform1State": "上升停止",
    "platform2State": "下降停止",
    "heaterStatus": "停止",
    "coolingStatus": "停止",
    "alarmStatus": "正常",
    "levelingStatus": "停止",
    "cylinderPressure": 1.5,
    "platform1Pressure": 2.5,
    "platform2Pressure": 2.3,
    "tiltAngle": 0.05,
    "platformPosition": 120,
    "timestamp": 1686532271000
}
```

**响应参数说明**：

| 参数名 | 类型 | 描述 |
|-------|------|------|
| msg | string | 请求结果，成功为 "success" |
| operationMode | string | 操作模式，可能的值：自动/手动 |
| emergencyStop | string | 急停状态，可能的值：复位/急停 |
| oilPumpStatus | string | 油泵状态，可能的值：停止/启动 |
| cylinderState | string | 刚柔缸状态，可能的值：下降停止/下降加压/上升停止/上升 |
| platform1State | string | 升降平台1状态，可能的值：上升/上升停止/下降/下降停止 |
| platform2State | string | 升降平台2状态，可能的值：上升/上升停止/下降/下降停止 |
| heaterStatus | string | 电加热状态，可能的值：停止/启动 |
| coolingStatus | string | 风冷状态，可能的值：停止/启动 |
| alarmStatus | string | 报警状态，可能的值：油温低/油温高/液位低/液位高/滤芯堵 |
| levelingStatus | string | 电动缸调平状态，可能的值：停止/启动 |
| cylinderPressure | number | 刚柔缸下降停止压力值 (MPa) |
| platform1Pressure | number | 升降平台1上升停止压力值 (MPa) |
| platform2Pressure | number | 升降平台2上升停止压力值 (MPa) |
| tiltAngle | number | 平台倾斜角度 (度) |
| platformPosition | integer | 平台位置信息 (mm) |
| timestamp | integer | 数据时间戳 (毫秒) |

### 6. 错误异常上报

**接口描述**：上报系统错误和异常信息

**请求方式**：POST

**请求路径**：`/stability/error/report`

**请求参数**：

| 参数名 | 类型 | 必填 | 描述 |
|-------|------|-----|------|
| errorCode | integer | 是 | 错误代码 |
| errorMessage | string | 是 | 错误描述信息 |
| timestamp | integer | 是 | 错误发生时间戳（毫秒） |

**请求示例**：

```json
{
    "errorCode": 1001,
    "errorMessage": "油温过高报警",
    "timestamp": 1686532271000
}
```

**响应示例**：

```json
{
    "msg": "success",
    "errorCode": 1001,
    "errorMessage": "油温过高报警",
    "timestamp": 1686532271000
}
```

**响应参数说明**：

| 参数名 | 类型 | 描述 |
|-------|------|------|
| msg | string | 请求结果，成功为 "success" |
| errorCode | integer | 错误代码 |
| errorMessage | string | 错误描述信息 |
| timestamp | integer | 错误发生时间戳（毫秒） |