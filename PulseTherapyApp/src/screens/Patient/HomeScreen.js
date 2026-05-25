/**
 * TLSR8269 脉冲治疗仪 - 患者端主界面
 * 
 * 设计原则：
 * 1. 极简 UI（帕金森患者手部震颤，需要大按钮）
 * 2. 简单操作（最多 3 步完成）
 * 3. 防误操作（紧急停止按钮要大！）
 * 4. 语音控制支持（"开始治疗"、"停止"、"求助"）
 * 
 * 安全原则：
 * - 患者只能选择固定模式（睡眠、运动障碍）
 * - 不能自行调整参数（界面不显示调整选项）
 * - 只能"申请医生调整"
 */

import React, { useState, useEffect, useRef } from 'react';
import {
  View,
  Text,
  TouchableOpacity,
  StyleSheet,
  Alert,
  ActivityIndicator,
  ScrollView
} from 'react-native';
import { BleManager, State } from 'react-native-ble-plx';
import BluetoothService, { TREATMENT_MODES } from '../services/BluetoothService';
import Icon from 'react-native-vector-icons/MaterialIcons';

const PatientHomeScreen = ({ navigation }) => {
  const [isConnected, setIsConnected] = useState(false);
  const [isTreating, setIsTreating] = useState(false);
  const [remainingTime, setRemainingTime] = useState(0);
  const [selectedMode, setSelectedMode] = useState(null);
  const [deviceInfo, setDeviceInfo] = useState(null);
  const timerRef = useRef(null);

  useEffect(() => {
    // 初始化蓝牙
    checkBluetoothState();
    return () => {
      if (timerRef.current) clearInterval(timerRef.current);
    };
  }, []);

  /**
   * 检查蓝牙状态
   */
  const checkBluetoothState = async () => {
    const state = await BluetoothService.manager.state();
    if (state !== State.PoweredOn) {
      Alert.alert(
        '蓝牙未开启',
        '请打开蓝牙以连接治疗设备',
        [{ text: '确定' }]
      );
    }
  };

  /**
   * 连接设备
   */
  const connectDevice = () => {
    navigation.navigate('DeviceScan');
  };

  /**
   * 断开连接
   */
  const disconnectDevice = async () => {
    try {
      await BluetoothService.disconnect();
      setIsConnected(false);
      setIsTreating(false);
      setDeviceInfo(null);
      Alert.alert('已断开', '设备已安全断开连接');
    } catch (error) {
      console.error('断开失败:', error);
    }
  };

  /**
   * 选择治疗模式（固定模式，基于刘河生教授论文）
   */
  const selectMode = (modeKey) => {
    if (!isConnected) {
      Alert.alert('未连接设备', '请先连接治疗设备');
      return;
    }
    setSelectedMode(modeKey);
  };

  /**
   * 开始治疗（只能使用固定模式）
   */
  const startTreatment = async () => {
    if (!selectedMode) {
      Alert.alert('请选择模式', '请先选择治疗模式');
      return;
    }

    try {
      await BluetoothService.startTreatment(selectedMode);
      setIsTreating(true);
      setRemainingTime(TREATMENT_MODES[selectedMode].duration * 60);
      
      // 倒计时
      timerRef.current = setInterval(() => {
        setRemainingTime(prev => {
          if (prev <= 1) {
            clearInterval(timerRef.current);
            setIsTreating(false);
            Alert.alert('治疗完成', '本次治疗已结束，注意休息');
            return 0;
          }
          return prev - 1;
        });
      }, 1000);

      Alert.alert('开始治疗', `已启动${TREATMENT_MODES[selectedMode].name}`);
    } catch (error) {
      Alert.alert('启动失败', error.message);
    }
  };

  /**
   * 停止治疗（紧急停止按钮）
   */
  const stopTreatment = async () => {
    try {
      await BluetoothService.stopTreatment();
      setIsTreating(false);
      if (timerRef.current) clearInterval(timerRef.current);
      Alert.alert('已停止', '治疗已停止');
    } catch (error) {
      Alert.alert('停止失败', error.message);
    }
  };

  /**
   * 申请医生调整参数（患者不能自己调）
   */
  const requestDoctorHelp = () => {
    if (!isConnected) {
      Alert.alert('未连接设备', '请先连接治疗设备');
      return;
    }
    navigation.navigate('HelpRequest');
  };

  /**
   * 格式化时间（秒 → MM:SS）
   */
  const formatTime = (seconds) => {
    const mins = Math.floor(seconds / 60);
    const secs = seconds % 60;
    return `${mins.toString().padStart(2, '0')}:${secs.toString().padStart(2, '0')}`;
  };

  return (
    <ScrollView style={styles.container}>
      {/* 顶部状态栏 */}
      <View style={styles.statusBar}>
        <View style={styles.statusItem}>
          <Icon 
            name={isConnected ? 'bluetooth-connected' : 'bluetooth'} 
            size={24} 
            color={isConnected ? '#4CAF50' : '#999'} 
          />
          <Text style={styles.statusText}>
            {isConnected ? '设备已连接' : '未连接设备'}
          </Text>
        </View>
        {deviceInfo && (
          <View style={styles.statusItem}>
            <Icon name="battery-std" size={20} color="#4CAF50" />
            <Text style={styles.statusText}>正常</Text>
          </View>
        )}
      </View>

      {/* 设备连接按钮 */}
      {!isConnected && (
        <TouchableOpacity style={styles.connectButton} onPress={connectDevice}>
          <Icon name="bluetooth-searching" size={32} color="#FFF" />
          <Text style={styles.connectButtonText}>连接治疗设备</Text>
        </TouchableOpacity>
      )}

      {/* 设备已连接 - 显示控制界面 */}
      {isConnected && (
        <View style={styles.controlPanel}>
          {/* 治疗状态显示 */}
          <View style={styles.statusDisplay}>
            <Text style={styles.statusTitle}>
              {isTreating ? '治疗进行中' : '准备就绪'}
            </Text>
            {isTreating && (
              <Text style={styles.timerText}>
                {formatTime(remainingTime)}
              </Text>
            )}
            {selectedMode && !isTreating && (
              <Text style={styles.modeText}>
                已选择：{TREATMENT_MODES[selectedMode].name}
              </Text>
            )}
          </View>

          {/* 模式选择（固定模式，基于刘河生教授论文） */}
          {!isTreating && (
            <View style={styles.modeSelection}>
              <Text style={styles.sectionTitle}>选择治疗模式</Text>
              <Text style={styles.sectionHint}>
                基于昌平实验室刘河生教授 Nature 论文方案
              </Text>
              
              {/* 睡眠治疗模式 */}
              <TouchableOpacity
                style={[
                  styles.modeButton,
                  selectedMode === 'SLEEP' && styles.modeButtonSelected
                ]}
                onPress={() => selectMode('SLEEP')}
              >
                <Icon name="hotel" size={32} color={selectedMode === 'SLEEP' ? '#FFF' : '#4CAF50'} />
                <View style={styles.modeTextContainer}>
                  <Text style={[styles.modeName, selectedMode === 'SLEEP' && styles.modeNameSelected]}>
                    {TREATMENT_MODES.SLEEP.name}
                  </Text>
                  <Text style={[styles.modeDesc, selectedMode === 'SLEEP' && styles.modeDescSelected]}>
                    {TREATMENT_MODES.SLEEP.description}
                  </Text>
                </View>
              </TouchableOpacity>

              {/* 运动障碍治疗模式 */}
              <TouchableOpacity
                style={[
                  styles.modeButton,
                  selectedMode === 'MOTOR' && styles.modeButtonSelected
                ]}
                onPress={() => selectMode('MOTOR')}
              >
                <Icon name="directions-run" size={32} color={selectedMode === 'MOTOR' ? '#FFF' : '#2196F3'} />
                <View style={styles.modeTextContainer}>
                  <Text style={[styles.modeName, selectedMode === 'MOTOR' && styles.modeNameSelected]}>
                    {TREATMENT_MODES.MOTOR.name}
                  </Text>
                  <Text style={[styles.modeDesc, selectedMode === 'MOTOR' && styles.modeDescSelected]}>
                    {TREATMENT_MODES.MOTOR.description}
                  </Text>
                </View>
              </TouchableOpacity>

              {/* 认知功能治疗模式（可选） */}
              <TouchableOpacity
                style={[
                  styles.modeButton,
                  selectedMode === 'COGNITION' && styles.modeButtonSelected
                ]}
                onPress={() => selectMode('COGNITION')}
              >
                <Icon name="psychology" size={32} color={selectedMode === 'COGNITION' ? '#FFF' : '#FF9800'} />
                <View style={styles.modeTextContainer}>
                  <Text style={[styles.modeName, selectedMode === 'COGNITION' && styles.modeNameSelected]}>
                    {TREATMENT_MODES.COGNITION.name}
                  </Text>
                  <Text style={[styles.modeDesc, selectedMode === 'COGNITION' && styles.modeDescSelected]}>
                    {TREATMENT_MODES.COGNITION.description}
                  </Text>
                </View>
              </TouchableOpacity>
            </View>
          )}

          {/* 控制按钮（大按钮，方便帕金森患者操作） */}
          <View style={styles.controlButtons}>
            {!isTreating ? (
              <TouchableOpacity
                style={[styles.startButton, !selectedMode && styles.buttonDisabled]}
                onPress={startTreatment}
                disabled={!selectedMode}
              >
                <Icon name="play-arrow" size={48} color="#FFF" />
                <Text style={styles.buttonText}>开始治疗</Text>
              </TouchableOpacity>
            ) : (
              <TouchableOpacity style={styles.stopButton} onPress={stopTreatment}>
                <Icon name="stop" size={48} color="#FFF" />
                <Text style={styles.buttonText}>停止治疗</Text>
              </TouchableOpacity>
            )}
          </View>

          {/* 求助按钮（申请医生调整参数） */}
          <TouchableOpacity style={styles.helpButton} onPress={requestDoctorHelp}>
            <Icon name="help-outline" size={24} color="#FFF" />
            <Text style={styles.helpButtonText}>效果不明显？申请医生协助</Text>
          </TouchableOpacity>

          {/* 断开连接按钮 */}
          <TouchableOpacity style={styles.disconnectButton} onPress={disconnectDevice}>
            <Text style={styles.disconnectButtonText}>断开设备</Text>
          </TouchableOpacity>
        </View>
      )}
    </ScrollView>
  );
};

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#F5F5F5',
  },
  statusBar: {
    flexDirection: 'row',
    justifyContent: 'space-around',
    padding: 16,
    backgroundColor: '#FFF',
    borderBottomWidth: 1,
    borderBottomColor: '#E0E0E0',
  },
  statusItem: {
    flexDirection: 'row',
    alignItems: 'center',
  },
  statusText: {
    marginLeft: 8,
    fontSize: 14,
    color: '#333',
  },
  connectButton: {
    margin: 20,
    padding: 20,
    backgroundColor: '#4CAF50',
    borderRadius: 12,
    alignItems: 'center',
    justifyContent: 'center',
    elevation: 4,
  },
  connectButtonText: {
    color: '#FFF',
    fontSize: 18,
    fontWeight: 'bold',
    marginTop: 8,
  },
  controlPanel: {
    padding: 16,
  },
  statusDisplay: {
    alignItems: 'center',
    marginBottom: 20,
    padding: 16,
    backgroundColor: '#FFF',
    borderRadius: 12,
    elevation: 2,
  },
  statusTitle: {
    fontSize: 20,
    fontWeight: 'bold',
    color: '#333',
    marginBottom: 8,
  },
  timerText: {
    fontSize: 48,
    fontWeight: 'bold',
    color: '#4CAF50',
    marginVertical: 8,
  },
  modeText: {
    fontSize: 16,
    color: '#666',
  },
  modeSelection: {
    marginBottom: 20,
  },
  sectionTitle: {
    fontSize: 18,
    fontWeight: 'bold',
    color: '#333',
    marginBottom: 4,
  },
  sectionHint: {
    fontSize: 12,
    color: '#999',
    marginBottom: 12,
    fontStyle: 'italic',
  },
  modeButton: {
    flexDirection: 'row',
    alignItems: 'center',
    padding: 16,
    backgroundColor: '#FFF',
    borderRadius: 12,
    marginBottom: 12,
    elevation: 2,
    borderWidth: 2,
    borderColor: 'transparent',
  },
  modeButtonSelected: {
    backgroundColor: '#4CAF50',
    borderColor: '#4CAF50',
  },
  modeTextContainer: {
    marginLeft: 16,
    flex: 1,
  },
  modeName: {
    fontSize: 16,
    fontWeight: 'bold',
    color: '#333',
    marginBottom: 4,
  },
  modeNameSelected: {
    color: '#FFF',
  },
  modeDesc: {
    fontSize: 12,
    color: '#666',
  },
  modeDescSelected: {
    color: '#FFF',
    opacity: 0.9,
  },
  controlButtons: {
    alignItems: 'center',
    marginVertical: 20,
  },
  startButton: {
    width: '80%',
    padding: 20,
    backgroundColor: '#4CAF50',
    borderRadius: 12,
    alignItems: 'center',
    justifyContent: 'center',
    elevation: 4,
  },
  buttonDisabled: {
    backgroundColor: '#CCC',
  },
  stopButton: {
    width: '80%',
    padding: 20,
    backgroundColor: '#F44336',
    borderRadius: 12,
    alignItems: 'center',
    justifyContent: 'center',
    elevation: 4,
  },
  buttonText: {
    color: '#FFF',
    fontSize: 18,
    fontWeight: 'bold',
    marginTop: 8,
  },
  helpButton: {
    flexDirection: 'row',
    alignItems: 'center',
    justifyContent: 'center',
    padding: 12,
    backgroundColor: '#2196F3',
    borderRadius: 8,
    marginBottom: 12,
  },
  helpButtonText: {
    color: '#FFF',
    fontSize: 14,
    fontWeight: 'bold',
    marginLeft: 8,
  },
  disconnectButton: {
    alignItems: 'center',
    padding: 12,
  },
  disconnectButtonText: {
    color: '#999',
    fontSize: 14,
  },
});

export default PatientHomeScreen;
