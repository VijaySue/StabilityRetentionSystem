import requests
import json
import time
from datetime import datetime

def log_message(message):
    """打印带时间戳的日志消息"""
    timestamp = datetime.now().strftime('%Y-%m-%d %H:%M:%S')
    print(f"[{timestamp}] {message}")

def test_system_status(server_ip, server_port):
    """测试系统状态接口"""
    url = f"http://{server_ip}:{server_port}/stability/system/status"
    
    try:
        log_message(f"测试系统状态接口: {url}")
        response = requests.get(url, timeout=5)
        log_message(f"请求状态码: {response.status_code}")
        
        if response.status_code == 200:
            response_data = response.json()
            log_message(f"接口返回: {json.dumps(response_data, ensure_ascii=False, indent=2)}")
            return True
        else:
            log_message(f"请求失败，状态码: {response.status_code}")
            log_message(f"错误信息: {response.text}")
            return False
            
    except requests.exceptions.ConnectionError:
        log_message("连接被拒绝，服务器可能未启动或端口错误。")
        return False
    except requests.exceptions.Timeout:
        log_message("请求超时，请检查服务器是否正常运行。")
        return False
    except Exception as e:
        log_message(f"发生错误: {e}")
        return False

def test_device_state(server_ip, server_port, fields=None):
    """测试设备状态接口，可选择性地指定需要返回的字段"""
    base_url = f"http://{server_ip}:{server_port}/stability/device/state"
    
    # 如果指定了字段，添加到URL查询参数
    url = base_url
    if fields:
        url = f"{base_url}?fields={fields}"
    
    try:
        log_message(f"测试设备状态接口: {url}")
        response = requests.get(url, timeout=5)
        log_message(f"请求状态码: {response.status_code}")
        
        if response.status_code == 200:
            response_data = response.json()
            log_message(f"接口返回: {json.dumps(response_data, ensure_ascii=False, indent=2)}")
            return True
        else:
            log_message(f"请求失败，状态码: {response.status_code}")
            log_message(f"错误信息: {response.text}")
            return False
            
    except requests.exceptions.ConnectionError:
        log_message("连接被拒绝，服务器可能未启动或端口错误。")
        return False
    except requests.exceptions.Timeout:
        log_message("请求超时，请检查服务器是否正常运行。")
        return False
    except Exception as e:
        log_message(f"发生错误: {e}")
        return False

def test_support_control(server_ip, server_port, task_id, defect_id, state):
    """测试支撑控制接口"""
    url = f"http://{server_ip}:{server_port}/stability/support/control"
    
    # 验证state值
    if state not in ["刚性支撑", "柔性复位"]:
        log_message(f"无效的支撑控制状态: {state}，有效值为'刚性支撑'或'柔性复位'")
        return False
    
    # 构造请求参数
    request_data = {
        "state": state,
        "taskId": task_id,
        "defectId": defect_id
    }
    
    try:
        log_message(f"测试支撑控制接口: {url}")
        log_message(f"请求参数: {json.dumps(request_data, ensure_ascii=False)}")
        response = requests.post(url, json=request_data, timeout=5)
        log_message(f"请求状态码: {response.status_code}")
        
        if response.status_code == 200:
            response_data = response.json()
            log_message(f"接口返回: {json.dumps(response_data, ensure_ascii=False, indent=2)}")
            return True
        else:
            log_message(f"请求失败，状态码: {response.status_code}")
            log_message(f"错误信息: {response.text}")
            return False
            
    except requests.exceptions.ConnectionError:
        log_message("连接被拒绝，服务器可能未启动或端口错误。")
        return False
    except requests.exceptions.Timeout:
        log_message("请求超时，请检查服务器是否正常运行。")
        return False
    except Exception as e:
        log_message(f"发生错误: {e}")
        return False

def test_platform_height_control(server_ip, server_port, task_id, defect_id, platform_num, state):
    """测试平台高度控制接口"""
    url = f"http://{server_ip}:{server_port}/stability/platformHeight/control"
    
    # 验证platformNum值
    if platform_num not in [1, 2]:
        log_message(f"无效的平台编号: {platform_num}，有效值为1或2")
        return False
        
    # 验证state值
    if state not in ["升高", "复位"]:
        log_message(f"无效的平台控制状态: {state}，有效值为'升高'或'复位'")
        return False
    
    # 构造请求参数
    request_data = {
        "taskId": task_id,
        "defectId": defect_id,
        "platformNum": platform_num,
        "state": state
    }
    
    try:
        log_message(f"测试平台高度控制接口: {url}")
        log_message(f"请求参数: {json.dumps(request_data, ensure_ascii=False)}")
        response = requests.post(url, json=request_data, timeout=5)
        log_message(f"请求状态码: {response.status_code}")
        
        if response.status_code == 200:
            response_data = response.json()
            log_message(f"接口返回: {json.dumps(response_data, ensure_ascii=False, indent=2)}")
            return True
        else:
            log_message(f"请求失败，状态码: {response.status_code}")
            log_message(f"错误信息: {response.text}")
            return False
            
    except requests.exceptions.ConnectionError:
        log_message("连接被拒绝，服务器可能未启动或端口错误。")
        return False
    except requests.exceptions.Timeout:
        log_message("请求超时，请检查服务器是否正常运行。")
        return False
    except Exception as e:
        log_message(f"发生错误: {e}")
        return False

def test_platform_horizontal_control(server_ip, server_port, task_id, defect_id, platform_num, state):
    """测试平台调平控制接口"""
    url = f"http://{server_ip}:{server_port}/stability/platformHorizontal/control"
    
    # 验证platformNum值
    if platform_num not in [1, 2]:
        log_message(f"无效的平台编号: {platform_num}，有效值为1或2")
        return False
        
    # 验证state值
    if state not in ["调平", "调平复位"]:
        log_message(f"无效的调平控制状态: {state}，有效值为'调平'或'调平复位'")
        return False
    
    # 构造请求参数
    request_data = {
        "taskId": task_id,
        "defectId": defect_id,
        "platformNum": platform_num,
        "state": state
    }
    
    try:
        log_message(f"测试平台调平控制接口: {url}")
        log_message(f"请求参数: {json.dumps(request_data, ensure_ascii=False)}")
        response = requests.post(url, json=request_data, timeout=5)
        log_message(f"请求状态码: {response.status_code}")
        
        if response.status_code == 200:
            response_data = response.json()
            log_message(f"接口返回: {json.dumps(response_data, ensure_ascii=False, indent=2)}")
            return True
        else:
            log_message(f"请求失败，状态码: {response.status_code}")
            log_message(f"错误信息: {response.text}")
            return False
            
    except requests.exceptions.ConnectionError:
        log_message("连接被拒绝，服务器可能未启动或端口错误。")
        return False
    except requests.exceptions.Timeout:
        log_message("请求超时，请检查服务器是否正常运行。")
        return False
    except Exception as e:
        log_message(f"发生错误: {e}")
        return False

def test_callback_server(callback_ip, callback_port):
    """测试回调服务器是否可用"""
    url = f"http://{callback_ip}:{callback_port}/health"
    
    try:
        log_message(f"测试回调服务器: {url}")
        response = requests.get(url, timeout=5)
        log_message(f"请求状态码: {response.status_code}")
        
        if response.status_code == 200:
            response_data = response.json()
            log_message(f"回调服务器状态: {json.dumps(response_data, ensure_ascii=False, indent=2)}")
            return True
        else:
            log_message(f"回调服务器不可用，状态码: {response.status_code}")
            return False
            
    except requests.exceptions.ConnectionError:
        log_message("连接被拒绝，回调服务器可能未启动或端口错误。")
        return False
    except requests.exceptions.Timeout:
        log_message("请求超时，请检查回调服务器是否正常运行。")
        return False
    except Exception as e:
        log_message(f"发生错误: {e}")
        return False

def run_comprehensive_test(server_ip, server_port, task_id, defect_id):
    """运行全面的接口测试"""
    
    # 测试系统状态
    if not test_system_status(server_ip, server_port):
        log_message("系统状态接口测试失败，终止后续测试")
        return
        
    # 测试设备状态 - 完整状态
    test_device_state(server_ip, server_port)
    
    # 测试设备状态 - 过滤字段
    test_device_state(server_ip, server_port, "operationMode,emergencyStop,cylinderState,platform1State,platform2State")
    
    # 测试支撑控制
    test_support_control(server_ip, server_port, task_id, defect_id, "刚性支撑")
    time.sleep(1)  # 间隔一秒
    test_support_control(server_ip, server_port, task_id, defect_id, "柔性复位")
    
    # 测试平台高度控制
    test_platform_height_control(server_ip, server_port, task_id, defect_id, 1, "升高")
    time.sleep(1)  # 间隔一秒
    test_platform_height_control(server_ip, server_port, task_id, defect_id, 1, "复位")
    time.sleep(1)  # 间隔一秒
    test_platform_height_control(server_ip, server_port, task_id, defect_id, 2, "升高")
    time.sleep(1)  # 间隔一秒
    test_platform_height_control(server_ip, server_port, task_id, defect_id, 2, "复位")
    
    # 测试平台调平控制
    test_platform_horizontal_control(server_ip, server_port, task_id, defect_id, 1, "调平")
    time.sleep(1)  # 间隔一秒
    test_platform_horizontal_control(server_ip, server_port, task_id, defect_id, 1, "调平复位")
    time.sleep(1)  # 间隔一秒
    test_platform_horizontal_control(server_ip, server_port, task_id, defect_id, 2, "调平")
    time.sleep(1)  # 间隔一秒
    test_platform_horizontal_control(server_ip, server_port, task_id, defect_id, 2, "调平复位")
    
    log_message("全面接口测试完成")

def get_int_input(prompt, default=None):
    """获取整数输入，带有默认值"""
    if default is not None:
        value = input(f"{prompt} [{default}]: ")
        if value.strip() == "":
            return default
        try:
            return int(value)
        except ValueError:
            print("输入无效，请输入数字")
            return get_int_input(prompt, default)
    else:
        try:
            return int(input(f"{prompt}: "))
        except ValueError:
            print("输入无效，请输入数字")
            return get_int_input(prompt)

def show_menu():
    """显示主菜单"""
    print("\n" + "="*60)
    print("稳定性系统接口测试工具".center(50))
    print("="*60)
    print("1. 测试系统状态接口 (/stability/system/status)")
    print("2. 测试设备状态接口 (/stability/device/state)")
    print("3. 测试支撑控制接口 (/stability/support/control)")
    print("4. 测试平台高度控制接口 (/stability/platformHeight/control)")
    print("5. 测试平台调平控制接口 (/stability/platformHorizontal/control)")
    print("6. 测试所有接口")
    print("7. 修改服务器设置")
    print("0. 退出程序")
    print("="*60)
    return get_int_input("请输入选项编号", 0)

def show_support_menu():
    """显示支撑控制菜单"""
    print("\n---支撑控制选项---")
    print("1. 刚性支撑")
    print("2. 柔性复位")
    print("0. 返回主菜单")
    return get_int_input("请选择操作", 0)

def show_platform_height_menu():
    """显示平台高度控制菜单"""
    print("\n---平台高度控制选项---")
    print("1. 平台1-升高")
    print("2. 平台1-复位")
    print("3. 平台2-升高")
    print("4. 平台2-复位")
    print("0. 返回主菜单")
    return get_int_input("请选择操作", 0)

def show_platform_level_menu():
    """显示平台调平控制菜单"""
    print("\n---平台调平控制选项---")
    print("1. 平台1-调平")
    print("2. 平台1-调平复位")
    print("3. 平台2-调平")
    print("4. 平台2-调平复位")
    print("0. 返回主菜单")
    return get_int_input("请选择操作", 0)

def main():
    """主函数，交互式菜单"""
    # 默认设置
    server_ip = "192.168.6.140"
    server_port = 8080
    task_id = 125
    defect_id = 91
    
    while True:
        choice = show_menu()
        
        if choice == 0:
            print("程序已退出")
            break
            
        elif choice == 1:  # 测试系统状态
            test_system_status(server_ip, server_port)
            input("按回车键继续...")
            
        elif choice == 2:  # 测试设备状态
            print("\n可用字段: operationMode, emergencyStop, oilPumpStatus, cylinderState, platform1State, platform2State")
            print("          heaterStatus, coolingStatus, leveling1Status, leveling2Status")
            print("          cylinderPressure, liftPressure, platform1TiltAngle, platform2TiltAngle")
            print("          platform1Position, platform2Position, timestamp")
            fields = input("请输入需要返回的字段，多个字段用逗号分隔（直接回车返回全部字段）: ")
            if fields.strip():
                test_device_state(server_ip, server_port, fields)
            else:
                test_device_state(server_ip, server_port)
            input("按回车键继续...")
            
        elif choice == 3:  # 测试支撑控制
            sub_choice = show_support_menu()
            if sub_choice == 1:
                test_support_control(server_ip, server_port, task_id, defect_id, "刚性支撑")
            elif sub_choice == 2:
                test_support_control(server_ip, server_port, task_id, defect_id, "柔性复位")
            input("按回车键继续...")
            
        elif choice == 4:  # 测试平台高度控制
            sub_choice = show_platform_height_menu()
            if sub_choice == 1:
                test_platform_height_control(server_ip, server_port, task_id, defect_id, 1, "升高")
            elif sub_choice == 2:
                test_platform_height_control(server_ip, server_port, task_id, defect_id, 1, "复位")
            elif sub_choice == 3:
                test_platform_height_control(server_ip, server_port, task_id, defect_id, 2, "升高")
            elif sub_choice == 4:
                test_platform_height_control(server_ip, server_port, task_id, defect_id, 2, "复位")
            input("按回车键继续...")
            
        elif choice == 5:  # 测试平台调平控制
            sub_choice = show_platform_level_menu()
            if sub_choice == 1:
                test_platform_horizontal_control(server_ip, server_port, task_id, defect_id, 1, "调平")
            elif sub_choice == 2:
                test_platform_horizontal_control(server_ip, server_port, task_id, defect_id, 1, "调平复位")
            elif sub_choice == 3:
                test_platform_horizontal_control(server_ip, server_port, task_id, defect_id, 2, "调平")
            elif sub_choice == 4:
                test_platform_horizontal_control(server_ip, server_port, task_id, defect_id, 2, "调平复位")
            input("按回车键继续...")
            
        elif choice == 6:  # 测试所有接口
            print("\n开始执行全面测试...")
            run_comprehensive_test(server_ip, server_port, task_id, defect_id)
            input("按回车键继续...")
            
        elif choice == 7:  # 修改服务器设置
            print("\n当前服务器设置:")
            print(f"服务器IP: {server_ip}")
            print(f"服务器端口: {server_port}")
            print(f"任务ID: {task_id}")
            print(f"缺陷ID: {defect_id}")
            print("\n输入新的设置（直接回车保留当前值）:")
            
            new_ip = input(f"服务器IP [{server_ip}]: ")
            if new_ip.strip():
                server_ip = new_ip
                
            new_port = get_int_input(f"服务器端口", server_port)
            server_port = new_port
            
            new_task_id = get_int_input(f"任务ID", task_id)
            task_id = new_task_id
            
            new_defect_id = get_int_input(f"缺陷ID", defect_id)
            defect_id = new_defect_id
            
            print("\n新的服务器设置已保存")
            input("按回车键继续...")
            
        else:
            print("无效的选项，请重新输入")
            
if __name__ == "__main__":
    main() 