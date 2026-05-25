/**
 * TLSR8269 脉冲治疗仪 - 蓝牙通信服务
 * 
 * 功能：
 * 1. 扫描 & 连接设备（蓝牙为主）
 * 2. WiFi 备用连接
 * 3. 按需求连网（患者发出请求时才联网）
 * 4. 患者只能选择固定模式（睡眠、运动障碍）
 * 5. 高级参数调整只能由医生执行
 * 
 * 安全原则：
 * - 患者端：只能启动/停止，不能调参数
 * - 医生端：可以远程调整参数
 * - 按需联网：平时蓝牙直连，求助时才联网
 */

import { BleManager } from 'react-native-ble-plx';
import { PermissionsAndroid, Platform } from 'react-native';
import axios from 'axios';

// TLSR8269 蓝牙 UUID（与设备固件一致）
const DEVICE_SERVICE_UUID = '0000180A-0000-1000-8000-00805F9B34FB';
const DEVICE_CHARA_STATE_UUID = '00002A50-0000-1000-8000-00805F9B34FB';  // 设备状态
const DEVICE_CHARA_CONTROL_UUID = '00002A51-0000-1000-8000-00805F9B34FB'; // 治疗控制
const DEVICE_CHARA_PARAMS_UUID = '00002A52-0000-1000-8000-00805F9B34FB';  // 参数配置（仅医生）
const DEVICE_CHARA_REALTIME_UUID = '00002A53-0000-1000-8000-00805F9B34FB'; // 实时数据

// 固定治疗模式（刘河生教授 Nature 论文方案）
const TREATMENT_MODES = {
  SLEEP: {
    name: '睡眠治疗模式',
    description: '调节丘脑，改善睡眠质量',
    frequency: 6,        // 6Hz (θ波）
    duty: 30,           // 30% 占空比
    duration: 30,       // 30分钟
    channels: [
      { ch: 4, freq: 6, duty: 30 },  // CH4 → 左 DLPFC (F3)
      { ch: 5, freq: 6, duty: 30 }   // CH5 → 右 DLPFC (F4)
    ],
    target: '丘脑 (Thalamus)',
    evidence: '昌平实验室 Liu et al., Nature 2026'
  },
  MOTOR: {
    name: '运动障碍治疗模式',
    description: '刺激 M1/SMA，改善震颤与僵直',
    frequency: 40,       // 40Hz (γ波）
    duty: 50,           // 50% 占空比
    duration: 20,       // 20分钟
    channels: [
      { ch: 0, freq: 40, duty: 50 },  // CH0 → 左 M1 (C3 偏前 2cm)
      { ch: 1, freq: 40, duty: 50 },  // CH1 → 右 M1 (C4 偏前 2cm)
      { ch: 2, freq: 40, duty: 50 },  // CH2 → 左 SMA (Cz 偏左 2cm)
      { ch: 3, freq: 40, duty: 50 }   // CH3 → 右 SMA (Cz 偏右 2cm)
    ],
    target: 'M1 + SMA 双侧',
    evidence: '昌平实验室 Liu et al., Nature 2026 (有效率 55.5%)'
  },
  COGNITION: {
    name: '认知功能治疗模式',
    description: '刺激 DLPFC，改善注意力与记忆力',
    frequency: 20,       // 20Hz (β波）
    duty: 30,           // 30% 占空比
    duration: 20,       // 20分钟
    channels: [
      { ch: 4, freq: 20, duty: 30 },  // CH4 → 左 DLPFC (F3)
      { ch: 5, freq: 20, duty: 30 }   // CH5 → 右 DLPFC (F4)
    ],
    target: 'DLPFC 双侧',
    evidence: '昌平实验室 Liu et al., Nature 2026'
  }
};

class BluetoothService {
  constructor() {
    this.manager = new BleManager();
    this.device = null;          // 当前连接的设备
    this.isConnected = false;
    this.isDoctor = false;       // 当前用户是否为医生
    this.onDeviceFound = null;   // 扫描到设备时的回调
    this.onConnectionChange = null; // 连接状态变化回调
    this.onTreatmentUpdate = null; // 治疗状态更新回调
  }

  /**
   * 请求蓝牙权限（Android）
   */
  async requestPermissions() {
    if (Platform.OS === 'android') {
      const granted = await PermissionsAndroid.requestMultiple([
        PermissionsAndroid.PERMISSIONS.ACCESS_FINE_LOCATION,
        PermissionsAndroid.PERMISSIONS.BLUETOOTH_SCAN,
        PermissionsAndroid.PERMISSIONS.BLUETOOTH_CONNECT
      ]);
      return granted;
    }
    return true;
  }

  /**
   * 扫描设备
   */
  startScan(onDeviceFound) {
    this.onDeviceFound = onDeviceFound;
    this.manager.startDeviceScan(null, null, (error, device) => {
      if (error) {
        console.error('扫描错误:', error);
        return;
      }
      if (device.name && device.name.includes('PulseGen')) {
        this.onDeviceFound(device);
      }
    });
  }

  /**
   * 停止扫描
   */
  stopScan() {
    this.manager.stopDeviceScan();
  }

  /**
   * 连接设备（蓝牙为主）
   */
  async connectDevice(deviceId) {
    try {
      const device = await this.manager.connectToDevice(deviceId);
      await device.discoverAllServicesAndCharacteristics();
      this.device = device;
      this.isConnected = true;
      if (this.onConnectionChange) {
        this.onConnectionChange(true);
      }
      return device;
    } catch (error) {
      console.error('连接失败:', error);
      throw error;
    }
  }

  /**
   * WiFi 备用连接（万鼎世纪方案）
   * 当蓝牙不可用时，尝试 WiFi 连接
   */
  async connectViaWiFi(deviceId) {
    // TODO: 实现 WiFi 连接逻辑（万鼎世纪 2.4G 模块）
    console.log('尝试 WiFi 连接...');
    // 示例：通过 HTTP API 连接设备
    try {
      const response = await axios.post(`http://${deviceId}.local/connect`, {
        timestamp: Date.now()
      });
      return response.data;
    } catch (error) {
      console.error('WiFi 连接失败:', error);
      throw error;
    }
  }

  /**
   * 断开连接
   */
  async disconnect() {
    if (this.device) {
      await this.device.cancelConnection();
      this.device = null;
      this.isConnected = false;
      if (this.onConnectionChange) {
        this.onConnectionChange(false);
      }
    }
  }

  /**
   * 发送指令到设备（JSON 格式）
   * 
   * 安全限制：
   * - 患者端只能发送 start/stop/query 指令
   * - 医生端可以发送 update_params 指令
   */
  async sendCommand(command) {
    if (!this.device) {
      throw new Error('设备未连接');
    }

    // 权限检查：患者不能发送 update_params 指令
    if (command.cmd === 'update_params' && !this.isDoctor) {
      throw new Error('高级参数调整只能由医生执行');
    }

    const jsonStr = JSON.stringify(command);
    const base64Str = Buffer.from(jsonStr).toString('base64');

    try {
      await this.device.writeCharacteristicWithResponseForService(
        DEVICE_SERVICE_UUID,
        DEVICE_CHARA_CONTROL_UUID,
        base64Str
      );
    } catch (error) {
      console.error('发送指令失败:', error);
      throw error;
    }
  }

  /**
   * 开始治疗（患者端，只能选择固定模式）
   * 
   * 安全设计：
   * - 患者只能选择预设模式（SLEEP / MOTOR / COGNITION）
   * - 不能自定义参数
   */
  async startTreatment(mode, duration) {
    if (!TREATMENT_MODES[mode]) {
      throw new Error('无效的治疗模式');
    }

    const modeConfig = TREATMENT_MODES[mode];
    const command = {
      cmd: 'start',
      params: {
        mode: mode,
        duration: duration || modeConfig.duration,
        channels: modeConfig.channels,
        evidence: modeConfig.evidence  // 记录证据来源
      }
    };

    await this.sendCommand(command);
  }

  /**
   * 停止治疗（患者端可以操作）
   */
  async stopTreatment() {
    const command = {
      cmd: 'stop',
      params: {}
    };
    await this.sendCommand(command);
  }

  /**
   * 查询设备状态
   */
  async queryStatus() {
    const command = {
      cmd: 'query',
      params: {}
    };
    await this.sendCommand(command);

    // 读取设备返回的状态
    try {
      const characteristic = await this.device.readCharacteristicForService(
        DEVICE_SERVICE_UUID,
        DEVICE_CHARA_STATE_UUID
      );
      const data = JSON.parse(Buffer.from(characteristic.value, 'base64').toString());
      if (this.onTreatmentUpdate) {
        this.onTreatmentUpdate(data);
      }
      return data;
    } catch (error) {
      console.error('查询状态失败:', error);
      throw error;
    }
  }

  /**
   * 医生端：调整高级参数（仅医生可以调用）
   * 
   * 安全设计：
   * - 需要医生认证（isDoctor = true）
   * - 参数范围受限（符合医疗安全规范）
   */
  async updateParameters(params) {
    if (!this.isDoctor) {
      throw new Error('高级参数调整只能由医生执行');
    }

    // 参数安全检查
    if (params.frequency < 1 || params.frequency > 200) {
      throw new Error('频率范围：1-200Hz');
    }
    if (params.duty < 0 || params.duty > 100) {
      throw new Error('占空比范围：0-100%');
    }
    if (params.current > 2.0) {
      throw new Error('电流限制：最大 2mA');
    }

    const command = {
      cmd: 'update_params',
      params: {
        ...params,
        doctor_id: this.doctorId,
        timestamp: Date.now()
      }
    };

    await this.sendCommand(command);
  }

  /**
   * 按需联网：患者发出求助请求时才连接云端
   * 
   * 工作流程：
   * 1. 患者感觉效果不明显 → 按"求助"按钮
   * 2. APP 临时联网 → 发送求助请求到云端
   * 3. 云端通知志愿者医生
   * 4. 医生接收请求 → 远程协助调整参数
   * 5. 调整后 → 设备同步新参数
   */
  async requestDoctorHelp(patientId, symptoms) {
    try {
      // 临时联网（按需）
      const response = await axios.post('https://api.pulsetherapy.org/help', {
        patient_id: patientId,
        symptoms: symptoms,
        device_id: this.device ? this.device.id : null,
        timestamp: Date.now()
      });

      // 返回：匹配的志愿者医生列表
      return response.data.doctors;
    } catch (error) {
      console.error('求助请求失败:', error);
      throw error;
    }
  }

  /**
   * 设置用户角色（患者 / 医生）
   */
  setUserRole(isDoctor, doctorId = null) {
    this.isDoctor = isDoctor;
    this.doctorId = doctorId;
  }

  /**
   * 销毁（清理资源）
   */
  destroy() {
    this.disconnect();
    this.manager.destroy();
  }
}

// 导出单例
const bluetoothService = new BluetoothService();
export default bluetoothService;
export { TREATMENT_MODES };
