# 稳定性保持系统 API 接口文档

## 接口概述

本文档详细描述了稳定性保持系统提供的所有 RESTful API 接口。系统通过这些接口与信息平台进行交互，实现对工业设备的监控和控制。

所有接口均采用 HTTP 协议，数据交换格式为 JSON。

## 通用格式

### 请求格式

- **请求方法**：GET/POST
- **内容类型**：application/json
- **字符编码**：UTF-8

### 响应格式

所有接口响应均包含以下字段：

- `msg`: 表示请求处理结果，成功为 "success"，失败为 "error"
- `code`: 表示HTTP状态码，成功为 200，失败为对应错误码
- 其他字段：根据具体接口而定

### 错误响应

当请求处理失败时，响应格式如下：

```json
{
    "msg": "error",
    "code": 400,
    "error": "错误描述信息"
}
```

### 状态码说明

| 状态码 | 说明                |
| --- | ----------------- |
| 200 | 请求成功              |
| 400 | 请求参数错误            |
| 401 | 未授权访问             |
| 404 | 请求的资源不存在          |
| 500 | 服务器内部错误           |
| 503 | 服务不可用（如PLC设备未连接时） |

## 接口清单

| 序号  | 接口名称          | 接口路径                                  | 请求方式 | 说明            |
| --- | ------------- | ------------------------------------- | ---- | ------------- |
| 1   | 系统状态检测        | /stability/system/status              | GET  | 检测设备是否在线      |
| 2   | 刚性支撑/柔性复位控制   | /stability/support/control            | POST | 控制刚性支撑或柔性复位   |
| 3   | 作业平台升高/复位控制   | /stability/platformHeight/control     | POST | 控制作业平台升高或复位   |
| 4   | 作业平台调平/调平复位控制 | /stability/platformHorizontal/control | POST | 控制作业平台调平或调平复位 |
| 5   | 实时状态获取        | /stability/device/state               | GET  | 获取系统所有设备状态    |
| 6   | 错误异常上报        | /stability/error/report               | POST | 上报报警信息        |

## 接口详细说明

### 1. 系统状态检测接口

**请求路径**：`{{base_stability_url}}/stability/system/status`

**请求方式**：GET

**响应参数**：

| 参数名   | 类型      | 描述                      |
| ----- | ------- | ----------------------- |
| msg   | string  | 请求结果：`success`或`error`  |
| code  | integer | 状态码：`200`(成功)或`503`(错误) |
| state | string  | 系统状态：`online`或`offline` |

**成功响应示例**：

```json
{
    "msg": "success",
    "code": 200,
    "state": "online"
}
```

**PLC设备未连接时的响应**：

当PLC设备未连接时，接口会返回错误响应：

```json
{
    "msg": "error",
    "code": 503,
    "state": "offline"
}
```

### 2. 刚性支撑/柔性复位控制接口

**请求路径**：`{{base_stability_url}}/stability/support/control`

**请求方式**：POST

**请求参数**：

| 参数名      | 类型      | 必填  | 描述                     |
| -------- | ------- | --- | ---------------------- |
| taskId   | integer | 是   | 任务ID                   |
| defectId | integer | 是   | 缺陷ID                   |
| state    | string  | 是   | 支撑状态：`"刚性支撑"`或`"柔性复位"` |

**请求示例**：

```json
{
    "taskId": 12345,
    "defectId": 67890,
    "state": "刚性支撑"  // 或 "柔性复位"
}
```

**响应示例**：

```json
{
    "msg": "success",
    "code": 200
}
```

**错误响应**：

1. 请求参数错误：

```json
{
    "msg": "error",
    "code": 400,
    "error": "请求参数不完整，需要taskId, defectId和state字段"
}
```

或

```json
{
    "msg": "error",
    "code": 400,
    "error": "无效的state值，必须为'刚性支撑'或'柔性复位'"
}
```

2. PLC设备未连接：

```json
{
    "msg": "error",
    "code": 503,
    "error": "PLC设备未连接，无法执行操作"
}
```

**回调信息**：

- **回调路径**：`{{base_edge_url}}/business/task/stability/support/cback`
- **回调方式**：POST
- **回调参数**：taskId, defectId, state (状态值为`"已刚性支撑"`或`"已柔性复位"`)

**回调请求示例**：

```json
{
    "taskId": 12345,
    "defectId": 67890,
    "state": "已刚性支撑"  // 或 "已柔性复位"
}
```

**回调响应示例**：

```json
{
    "msg": "success",
    "code": 200
}
```

### 3. 作业平台升高/复位控制接口

**请求路径**：`{{base_stability_url}}/stability/platformHeight/control`

**请求方式**：POST

**请求参数**：

| 参数名         | 类型      | 必填  | 描述                 |
| ----------- | ------- | --- | ------------------ |
| taskId      | integer | 是   | 任务ID               |
| defectId    | integer | 是   | 缺陷ID               |
| platformNum | integer | 是   | 平台编号：`1`或`2`       |
| state       | string  | 是   | 控制状态：`"升高"`或`"复位"` |

**请求示例**：

```json
{
    "taskId": 12345,
    "defectId": 67890,
    "platformNum": 1,
    "state": "升高"  // 或 "复位"
}
```

**响应示例**：

```json
{
    "msg": "success",
    "code": 200
}
```

**错误响应**：

1. 请求参数错误：

```json
{
    "msg": "error",
    "code": 400,
    "error": "请求参数不完整，需要taskId, defectId, platformNum和state字段"
}
```

或

```json
{
    "msg": "error",
    "code": 400,
    "error": "无效的platformNum值，必须为1或2"
}
```

或

```json
{
    "msg": "error",
    "code": 400,
    "error": "无效的state值，必须为'升高'或'复位'"
}
```

2. PLC设备未连接：

```json
{
    "msg": "error",
    "code": 503,
    "error": "PLC设备未连接，无法执行操作"
}
```

**回调信息**：

- **回调路径**：`{{base_edge_url}}/business/task/stability/platformHeight/cback`
- **回调方式**：POST
- **回调参数**：taskId, defectId, platformNum, state (状态值为`"已升高"`或`"已复位"`)

**回调请求示例**：

```json
{
    "taskId": 12345,
    "defectId": 67890,
    "platformNum": 1,
    "state": "已升高"  // 或 "已复位"
}
```

**回调响应示例**：

```json
{
    "msg": "success",
    "code": 200
}
```

### 4. 作业平台调平/调平复位控制接口

**请求路径**：`{{base_stability_url}}/stability/platformHorizontal/control`

**请求方式**：POST

**请求参数**：

| 参数名         | 类型      | 必填  | 描述                   |
| ----------- | ------- | --- | -------------------- |
| taskId      | integer | 是   | 任务ID                 |
| defectId    | integer | 是   | 缺陷ID                 |
| platformNum | integer | 是   | 平台编号：`1`或`2`         |
| state       | string  | 是   | 控制状态：`"调平"`或`"调平复位"` |

**请求示例**：

```json
{
    "taskId": 12345,
    "defectId": 67890,
    "platformNum": 1,
    "state": "调平"  // 或 "调平复位"
}
```

**响应示例**：

```json
{
    "msg": "success",
    "code": 200
}
```

**错误响应**：

1. 请求参数错误：

```json
{
    "msg": "error",
    "code": 400,
    "error": "请求参数不完整，需要taskId, defectId, platformNum和state字段"
}
```

或

```json
{
    "msg": "error",
    "code": 400,
    "error": "无效的platformNum值，必须为1或2"
}
```

或

```json
{
    "msg": "error",
    "code": 400,
    "error": "无效的state值，必须为'调平'或'调平复位'"
}
```

2. PLC设备未连接：

```json
{
    "msg": "error",
    "code": 503,
    "error": "PLC设备未连接，无法执行操作"
}
```

**回调信息**：

- **回调路径**：`{{base_edge_url}}/business/task/stability/platformHorizontal/cback`
- **回调方式**：POST
- **回调参数**：taskId, defectId, platformNum, state (状态值为`"已调平"`或`"已调平复位"`)

**回调请求示例**：

```json
{
    "taskId": 12345,
    "defectId": 67890,
    "platformNum": 1,
    "state": "已调平"  // 或 "已调平复位"
}
```

**回调响应示例**：

```json
{
    "msg": "success",
    "code": 200
}
```

### 5. 实时状态获取接口

**请求路径**：`{{base_stability_url}}/stability/device/state`

**请求方式**：GET

**请求参数**：

| 参数名    | 类型     | 必填  | 描述                |
| ------ | ------ | --- | ----------------- |
| fields | string | 否   | 指定返回的字段，多个字段用逗号分隔 |

**请求示例**：

```
GET {{base_stability_url}}/stability/device/state?fields=platform1State,platform2State,platform1Position,platform2Position
```

**响应参数说明**：

| 参数名                | 类型      | 描述                 |
| ------------------ | ------- | ------------------ |
| msg                | string  | 请求结果，成功为 "success" |
| code               | integer | 状态码，成功为 200        |
| operationMode      | string  | 操作模式               |
| emergencyStop      | string  | 急停状态               |
| oilPumpStatus      | string  | 油泵状态               |
| cylinderState      | string  | 刚柔缸状态              |
| platform1State     | string  | 升降平台1状态            |
| platform2State     | string  | 升降平台2状态            |
| heaterStatus       | string  | 电加热状态              |
| coolingStatus      | string  | 风冷状态               |
| leveling1Status    | string  | 1#电动缸调平            |
| leveling2Status    | string  | 2#电动缸调平            |
| cylinderPressure   | float32 | 刚柔缸下降停止压力值         |
| liftPressure       | float32 | 升降平台上升停止压力值        |
| platform1TiltAngle | float32 | 平台1倾斜角度            |
| platform2TiltAngle | float32 | 平台2倾斜角度            |
| platform1Position  | float32 | 平台1位置信息            |
| platform2Position  | float32 | 平台2位置信息            |
| timestamp          | integer | 数据时间戳              |

**响应示例**：

```json
{
    "msg": "success",
    "code": 200,
    "operationMode": "自动",
    "emergencyStop": "正常",
    "oilPumpStatus": "启动",
    "cylinderState": "下降停止",
    "platform1State": "上升停止",
    "platform2State": "下降停止",
    "heaterStatus": "加热",
    "coolingStatus": "启动",
    "leveling1Status": "停止",
    "leveling2Status": "停止",
    "cylinderPressure": 25000,
    "liftPressure": 30000,
    "platform1TiltAngle": 150,
    "platform2TiltAngle": 180,
    "platform1Position": 1500,
    "platform2Position": 1200,
    "timestamp": 1686532271000
}
```

**PLC设备未连接时的响应**：

当PLC设备未连接时，接口返回错误响应：

```json
{
    "msg": "error",
    "code": 503,
    "timestamp": 1686532271000
}
```

**错误响应**：

```json
{
    "msg": "error",
    "code": 500,
    "error": "获取设备状态失败：错误描述"
}
```

**状态参数说明**：

| 参数名                | 类型      | 描述          | 可能的值                   |
| ------------------ | ------- | ----------- | ---------------------- |
| operationMode      | string  | 操作模式        | 自动, 手动                 |
| emergencyStop      | string  | 急停状态        | 正常, 急停                 |
| oilPumpStatus      | string  | 油泵状态        | 启动, 停止                 |
| cylinderState      | string  | 刚柔缸状态       | 下降停止, 下降加压, 上升停止, 上升加压 |
| platform1State     | string  | 升降平台1状态     | 上升, 上升停止, 下降, 下降停止     |
| platform2State     | string  | 升降平台2状态     | 上升, 上升停止, 下降, 下降停止     |
| heaterStatus       | string  | 电加热状态       | 加热, 停止                 |
| coolingStatus      | string  | 风冷状态        | 启动, 停止                 |
| leveling1Status    | string  | 1#电动缸调平     | 停止, 启动                 |
| leveling2Status    | string  | 2#电动缸调平     | 停止, 启动                 |
| cylinderPressure   | float32 | 刚柔缸下降停止压力值  | 四字节浮点型                 |
| liftPressure       | float32 | 升降平台上升停止压力值 | 四字节浮点型                 |
| platform1TiltAngle | float32 | 平台1倾斜角度     | 四字节浮点型                 |
| platform2TiltAngle | float32 | 平台2倾斜角度     | 四字节浮点型                 |
| platform1Position  | float32 | 平台1位置信息     | 四字节浮点型                 |
| platform2Position  | float32 | 平台2位置信息     | 四字节浮点型                 |
| timestamp          | integer | 数据时间戳       | 毫秒级时间戳                 |

### 6. 错误异常上报接口

**请求路径**：`{{base_edge_url}}/stability/error/report`

**请求方式**：POST

**请求参数**：

| 参数名       | 类型      | 必填  | 描述                       |
| --------- | ------- | --- | ------------------------ |
| alarm     | string  | 是   | 报警信息                     |
| state     | string  | 是   | 报警状态:`"error"`或`"clear"` |
| timestamp | integer | 是   | 错误发生时间戳(毫秒)              |

**可能的报警信息**：

- 油温低
- 油温高
- 未知油温报警
- 液位低
- 液位高
- 未知液位报警
- 滤芯堵
- 未知滤芯报警
- PLC连接故障

**可能的清除报警信息**：

- 油温正常
- 液位正常
- 滤芯正常
- PLC连接恢复

**请求示例**：

```json
{
    "alarm": "油温低",
    "state": "error",
    "timestamp": 1686532271000
}
```

**响应示例**：

```json
{
    "msg": "success",
    "code": 200
}
```
