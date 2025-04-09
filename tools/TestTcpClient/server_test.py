from flask import Flask, request, jsonify
import logging
from datetime import datetime

# 修改日志配置
logging.basicConfig(
    level=logging.INFO,
    format='[%(asctime)s] [%(levelname)s] %(message)s',
    datefmt='%Y-%m-%d %H:%M:%S'  # 修改这里，移除了毫秒部分
)

app = Flask(__name__)


@app.route('/business/task/stability/platformHeight/cback', methods=['POST'])
def platform_height_callback():
    try:
        data = request.get_json()
        task_id = data.get('taskId')
        defect_id = data.get('defectId')
        platform_num = data.get('platformNum')
        state = data.get('state')

        logging.info(f"收到平台高度回调 - 任务ID: {task_id}, 缺陷ID: {defect_id}, "
                     f"平台号: {platform_num}, 状态: {state}")

        return jsonify({
            "code": 200,
            "msg": "平台高度回调处理成功",
            "data": {
                "taskId": task_id,
                "defectId": defect_id,
                "platformNum": platform_num,
                "state": state,
                "timestamp": datetime.now().isoformat()
            }
        })
    except Exception as e:
        logging.error(f"处理平台高度回调时发生错误: {str(e)}")
        return jsonify({
            "code": 500,
            "msg": f"处理回调时发生错误: {str(e)}"
        }), 500


@app.route('/business/task/stability/support/cback', methods=['POST'])
def support_callback():
    try:
        data = request.get_json()
        task_id = data.get('taskId')
        defect_id = data.get('defectId')
        state = data.get('state')

        logging.info(f"收到支撑回调 - 任务ID: {task_id}, 缺陷ID: {defect_id}, 状态: {state}")

        return jsonify({
            "code": 200,
            "msg": "支撑回调处理成功",
            "data": {
                "taskId": task_id,
                "defectId": defect_id,
                "state": state,
                "timestamp": datetime.now().isoformat()
            }
        })
    except Exception as e:
        logging.error(f"处理支撑回调时发生错误: {str(e)}")
        return jsonify({
            "code": 500,
            "msg": f"处理回调时发生错误: {str(e)}"
        }), 500


@app.route('/business/task/stability/platformHorizontal/cback', methods=['POST'])
def platform_horizontal_callback():
    try:
        data = request.get_json()
        task_id = data.get('taskId')
        defect_id = data.get('defectId')
        platform_num = data.get('platformNum')
        state = data.get('state')

        logging.info(f"收到平台水平回调 - 任务ID: {task_id}, 缺陷ID: {defect_id}, "
                     f"平台号: {platform_num}, 状态: {state}")

        return jsonify({
            "code": 200,
            "msg": "平台水平回调处理成功",
            "data": {
                "taskId": task_id,
                "defectId": defect_id,
                "platformNum": platform_num,
                "state": state,
                "timestamp": datetime.now().isoformat()
            }
        })
    except Exception as e:
        logging.error(f"处理平台水平回调时发生错误: {str(e)}")
        return jsonify({
            "code": 500,
            "msg": f"处理回调时发生错误: {str(e)}"
        }), 500

@app.route('/stability/device/state', methods=['GET'])
def device_state():
    # 获取请求中的fields参数
    fields = request.args.get('fields', '')
    logging.info(f"收到设备状态请求, 过滤字段: {fields}")

    # 创建一个模拟的设备状态数据，字段名与C++接口保持一致
    device_data = {
        "msg": "success",
        "code": 200,
        "operationMode": "手动",
        "emergencyStop": "正常",
        "oilPumpStatus": "停止",
        "cylinderState": "上升停止",
        "platform1State": "上升停止",
        "platform2State": "下降停止",
        "heaterStatus": "停止",
        "coolingStatus": "停止",
        "leveling1Status": "停止",
        "leveling2Status": "停止",
        "cylinderPressure": 0.0,
        "liftPressure": 0.0,
        "platform1TiltAngle": 0.0,
        "platform2TiltAngle": 0.0,
        "platform1Position": 0.0,
        "platform2Position": 0.0,
        "timestamp": int(datetime.now().timestamp() * 1000)
    }

    # 如果有fields参数，过滤返回的字段
    if fields:
        field_list = fields.split(',')
        filtered_data = {
            "msg": "success",
            "code": 200,
            "timestamp": int(datetime.now().timestamp() * 1000)
        }
        for field in field_list:
            if field in device_data:
                filtered_data[field] = device_data[field]
        return jsonify(filtered_data)

    return jsonify(device_data)


@app.route('/stability/support/control', methods=['POST'])
def support_control():
    try:
        data = request.get_json()
        task_id = data.get('taskId')
        defect_id = data.get('defectId')
        state = data.get('state')
        
        # 验证参数
        if not all([task_id is not None, defect_id is not None, state]):
            logging.error("支撑控制请求参数不完整")
            return jsonify({
                "msg": "error",
                "code": 400,
                "error": "请求参数不完整，需要taskId, defectId和state字段"
            }), 400
            
        # 验证state值
        if state not in ["刚性支撑", "柔性复位"]:
            logging.error(f"无效的支撑控制状态: {state}")
            return jsonify({
                "msg": "error",
                "code": 400,
                "error": "无效的state值，必须为'刚性支撑'或'柔性复位'"
            }), 400
        
        logging.info(f"收到支撑控制请求 - 任务ID: {task_id}, 缺陷ID: {defect_id}, 状态: {state}")
        
        return jsonify({
            "msg": "success",
            "code": 200
        })
    except Exception as e:
        logging.error(f"处理支撑控制请求时发生错误: {str(e)}")
        return jsonify({
            "msg": "error",
            "code": 500,
            "error": str(e)
        }), 500



@app.route('/stability/platformHeight/control', methods=['POST'])
def platform_height_control():
    try:
        data = request.get_json()
        task_id = data.get('taskId')
        defect_id = data.get('defectId')
        platform_num = data.get('platformNum')
        state = data.get('state')
        
        # 验证参数
        if not all([task_id is not None, defect_id is not None, platform_num is not None, state]):
            logging.error("平台高度控制请求参数不完整")
            return jsonify({
                "msg": "error",
                "code": 400,
                "error": "请求参数不完整，需要taskId, defectId, platformNum和state字段"
            }), 400
            
        # 验证platformNum值
        if platform_num not in [1, 2]:
            logging.error(f"无效的平台编号: {platform_num}")
            return jsonify({
                "msg": "error",
                "code": 400,
                "error": "无效的platformNum值，必须为1或2"
            }), 400
            
        # 验证state值
        if state not in ["升高", "复位"]:
            logging.error(f"无效的平台控制状态: {state}")
            return jsonify({
                "msg": "error",
                "code": 400,
                "error": "无效的state值，必须为'升高'或'复位'"
            }), 400
        
        logging.info(f"收到平台高度控制请求 - 任务ID: {task_id}, 缺陷ID: {defect_id}, "
                     f"平台号: {platform_num}, 状态: {state}")
        
        return jsonify({
            "msg": "success",
            "code": 200
        })
    except Exception as e:
        logging.error(f"处理平台高度控制请求时发生错误: {str(e)}")
        return jsonify({
            "msg": "error",
            "code": 500,
            "error": str(e)
        }), 500


@app.route('/stability/platformHorizontal/control', methods=['POST'])
def platform_horizontal_control():
    try:
        data = request.get_json()
        task_id = data.get('taskId')
        defect_id = data.get('defectId')
        platform_num = data.get('platformNum')
        state = data.get('state')
        
        # 验证参数
        if not all([task_id is not None, defect_id is not None, platform_num is not None, state]):
            logging.error("平台调平控制请求参数不完整")
            return jsonify({
                "msg": "error",
                "code": 400,
                "error": "请求参数不完整，需要taskId, defectId, platformNum和state字段"
            }), 400
            
        # 验证platformNum值
        if platform_num not in [1, 2]:
            logging.error(f"无效的平台编号: {platform_num}")
            return jsonify({
                "msg": "error",
                "code": 400,
                "error": "无效的platformNum值，必须为1或2"
            }), 400
            
        # 验证state值
        if state not in ["调平", "调平复位"]:
            logging.error(f"无效的调平控制状态: {state}")
            return jsonify({
                "msg": "error",
                "code": 400,
                "error": "无效的state值，必须为'调平'或'调平复位'"
            }), 400
        
        logging.info(f"收到平台调平控制请求 - 任务ID: {task_id}, 缺陷ID: {defect_id}, "
                     f"平台号: {platform_num}, 状态: {state}")
        
        return jsonify({
            "msg": "success",
            "code": 200
        })
    except Exception as e:
        logging.error(f"处理平台调平控制请求时发生错误: {str(e)}")
        return jsonify({
            "msg": "error",
            "code": 500,
            "error": str(e)
        }), 500

@app.route('/stability/error/report', methods=['POST'])
def error_report():
    try:
        data = request.get_json()
        alarm = data.get('alarm', '未知报警')
        state = data.get('state', 'error')
        timestamp = data.get('timestamp')
        
        # 将时间戳转换为可读格式（如果提供了时间戳）
        time_str = ""
        if timestamp:
            try:
                # 假设时间戳是毫秒级的整数
                dt = datetime.fromtimestamp(timestamp / 1000.0)
                time_str = f", 时间: {dt.strftime('%Y-%m-%d %H:%M:%S')}"
            except:
                pass
                
        logging.info(f"收到错误报告 - 报警: {alarm} {state} {time_str}")

        return jsonify({
            "code": 200,
            "msg": "错误报告处理成功"
        })
    except Exception as e:
        logging.error(f"处理错误报告时发生错误: {str(e)}")
        return jsonify({
            "code": 500,
            "msg": f"处理错误报告时发生错误: {str(e)}"
        }), 500


if __name__ == '__main__':
    # 设置服务器地址和端口
    host = '0.0.0.0'  # 监听所有网络接口
    port = 8080

    logging.info(f"启动服务器: http://{host}:{port}/")
    app.run(host=host, port=port, debug=True) 