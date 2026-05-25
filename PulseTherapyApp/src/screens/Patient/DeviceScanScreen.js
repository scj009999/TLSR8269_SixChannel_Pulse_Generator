/**
 * 设备扫描界面 - 蓝牙扫描 TLSR8269 设备
 * 功能：
 * 1. 扫描附近设备
 * 2. 显示设备列表
 * 3. 连接设备
 * 4. WiFi 备用连接
 */

import React, { useState, useEffect } from 'react';
import {
  View,
  Text,
  TouchableOpacity,
  StyleSheet,
  FlatList,
  ActivityIndicator,
  Alert,
} from 'react-native';
import Icon from 'react-native-vector-icons/MaterialIcons';
import BluetoothService from '../../services/BluetoothService';

const DeviceScanScreen = ({ navigation }) => {
  const [devices, setDevices] = useState([]);
  const [isScanning, setIsScanning] = useState(false);
  const [connectedDevice, setConnectedDevice] = useState(null);

  useEffect(() => {
    checkBluetoothState();
    return () => {
      BluetoothService.stopScan();
    };
  }, []);

  /**
   * 检查蓝牙状态
   */
  const checkBluetoothState = async () => {
    const state = await BluetoothService.manager.state();
    if (state !== 'PoweredOn') {
      Alert.alert(
        '蓝牙未开启',
        '请打开蓝牙以连接治疗设备',
        [
          { text: '取消', style: 'cancel' },
          { text: '去开启', onPress: () => BluetoothService.manager.enable() }
        ]
      );
    }
  };

  /**
   * 开始扫描设备
   */
  const startScan = async () => {
    setIsScanning(true);
    setDevices([]);

    try {
      await BluetoothService.requestPermissions();
      
      BluetoothService.startScan((device) => {
        setDevices(prev => {
          // 避免重复
          if (prev.find(d => d.id === device.id)) return prev;
          return [...prev, device];
        });
      });

      // 10秒后自动停止扫描
      setTimeout(() => {
        stopScan();
      }, 10000);

    } catch (error) {
      console.error('扫描失败:', error);
      Alert.alert('扫描失败', error.message);
      setIsScanning(false);
    }
  };

  /**
   * 停止扫描
   */
  const stopScan = () => {
    BluetoothService.stopScan();
    setIsScanning(false);
  };

  /**
   * 连接设备
   */
  const connectDevice = async (device) => {
    try {
      setIsScanning(false);
      await BluetoothService.connectDevice(device.id);
      
      setConnectedDevice(device);
      Alert.alert(
        '连接成功',
        `已连接设备: ${device.name || device.id}`,
        [
          {
            text: '确定',
            onPress: () => navigation.navigate('Home')
          }
        ]
      );
    } catch (error) {
      console.error('连接失败:', error);
      Alert.alert('连接失败', error.message);
    }
  };

  /**
   * WiFi 备用连接
   */
  const connectViaWiFi = async () => {
    try {
      Alert.prompt(
        'WiFi 连接',
        '请输入设备 ID (如: PulseGen-001)',
        [
          { text: '取消', style: 'cancel' },
          {
            text: '连接',
            onPress: async (deviceId) => {
              try {
                await BluetoothService.connectViaWiFi(deviceId);
                Alert.alert('连接成功', `已通过 WiFi 连接设备: ${deviceId}`);
                navigation.navigate('Home');
              } catch (error) {
                Alert.alert('WiFi 连接失败', error.message);
              }
            }
          }
        ]
      );
    } catch (error) {
      console.error('WiFi 连接失败:', error);
    }
  };

  /**
   * 渲染设备列表
   */
  const renderDeviceItem = ({ item }) => (
    <TouchableOpacity
      style={styles.deviceItem}
      onPress={() => connectDevice(item)}
    >
      <Icon name="bluetooth" size={24} color="#2196F3" />
      <View style={styles.deviceInfo}>
        <Text style={styles.deviceName}>{item.name || '未知设备'}</Text>
        <Text style={styles.deviceId}>ID: {item.id}</Text>
        <Text style={styles.deviceRssi}>信号: {item.rssi} dBm</Text>
      </View>
      <Icon name="chevron-right" size={24} color="#999" />
    </TouchableOpacity>
  );

  return (
    <View style={styles.container}>
      {/* 顶部标题 */}
      <View style={styles.header}>
        <Icon name="bluetooth-searching" size={32} color="#2196F3" />
        <Text style={styles.title}>扫描治疗设备</Text>
        <Text style={styles.subtitle}>
          请选择要连接的治疗仪
        </Text>
      </View>

      {/* 扫描按钮 */}
      <TouchableOpacity
        style={[styles.scanButton, isScanning && styles.scanButtonActive]}
        onPress={isScanning ? stopScan : startScan}
      >
        <Icon 
          name={isScanning ? 'stop' : 'bluetooth-searching'} 
          size={24} 
          color="#FFF" 
        />
        <Text style={styles.scanButtonText}>
          {isScanning ? '停止扫描' : '开始扫描'}
        </Text>
      </TouchableOpacity>

      {/* 扫描动画 */}
      {isScanning && (
        <View style={styles.scanningIndicator}>
          <ActivityIndicator size="large" color="#2196F3" />
          <Text style={styles.scanningText}>正在扫描附近设备...</Text>
        </View>
      )}

      {/* 设备列表 */}
      {devices.length > 0 && (
        <View style={styles.deviceListContainer}>
          <Text style={styles.listTitle}>发现 {devices.length} 个设备</Text>
          <FlatList
            data={devices}
            renderItem={renderDeviceItem}
            keyExtractor={item => item.id}
            style={styles.deviceList}
          />
        </View>
      )}

      {/* 空状态 */}
      {!isScanning && devices.length === 0 && (
        <View style={styles.emptyState}>
          <Icon name="bluetooth-audio" size={64} color="#E0E0E0" />
          <Text style={styles.emptyText}>未发现设备</Text>
          <Text style={styles.emptyHint}>
            请点击"开始扫描"按钮
          </Text>
        </View>
      )}

      {/* WiFi 备用连接 */}
      <TouchableOpacity style={styles.wifiButton} onPress={connectViaWiFi}>
        <Icon name="wifi" size={20} color="#4CAF50" />
        <Text style={styles.wifiButtonText}>WiFi 备用连接</Text>
      </TouchableOpacity>

      {/* 已连接设备信息 */}
      {connectedDevice && (
        <View style={styles.connectedInfo}>
          <Icon name="check-circle" size={20} color="#4CAF50" />
          <Text style={styles.connectedText}>
            已连接: {connectedDevice.name || connectedDevice.id}
          </Text>
        </View>
      )}
    </View>
  );
};

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#F5F5F5',
    padding: 16,
  },
  header: {
    alignItems: 'center',
    marginBottom: 24,
  },
  title: {
    fontSize: 24,
    fontWeight: 'bold',
    color: '#333',
    marginTop: 12,
  },
  subtitle: {
    fontSize: 14,
    color: '#666',
    marginTop: 4,
  },
  scanButton: {
    flexDirection: 'row',
    alignItems: 'center',
    justifyContent: 'center',
    padding: 16,
    backgroundColor: '#2196F3',
    borderRadius: 12,
    marginBottom: 16,
    elevation: 4,
  },
  scanButtonActive: {
    backgroundColor: '#F44336',
  },
  scanButtonText: {
    color: '#FFF',
    fontSize: 18,
    fontWeight: 'bold',
    marginLeft: 8,
  },
  scanningIndicator: {
    alignItems: 'center',
    marginVertical: 16,
  },
  scanningText: {
    fontSize: 14,
    color: '#666',
    marginTop: 8,
  },
  deviceListContainer: {
    flex: 1,
  },
  listTitle: {
    fontSize: 16,
    fontWeight: 'bold',
    color: '#333',
    marginBottom: 8,
  },
  deviceList: {
    flex: 1,
  },
  deviceItem: {
    flexDirection: 'row',
    alignItems: 'center',
    padding: 16,
    backgroundColor: '#FFF',
    borderRadius: 8,
    marginBottom: 8,
    elevation: 2,
  },
  deviceInfo: {
    flex: 1,
    marginLeft: 12,
  },
  deviceName: {
    fontSize: 16,
    fontWeight: 'bold',
    color: '#333',
  },
  deviceId: {
    fontSize: 12,
    color: '#666',
    marginTop: 2,
  },
  deviceRssi: {
    fontSize: 12,
    color: '#999',
    marginTop: 2,
  },
  emptyState: {
    flex: 1,
    alignItems: 'center',
    justifyContent: 'center',
  },
  emptyText: {
    fontSize: 18,
    color: '#999',
    marginTop: 16,
  },
  emptyHint: {
    fontSize: 14,
    color: '#BBB',
    marginTop: 8,
  },
  wifiButton: {
    flexDirection: 'row',
    alignItems: 'center',
    justifyContent: 'center',
    padding: 12,
    backgroundColor: '#FFF',
    borderRadius: 8,
    marginTop: 16,
    borderWidth: 1,
    borderColor: '#4CAF50',
  },
  wifiButtonText: {
    color: '#4CAF50',
    fontSize: 14,
    fontWeight: 'bold',
    marginLeft: 8,
  },
  connectedInfo: {
    flexDirection: 'row',
    alignItems: 'center',
    justifyContent: 'center',
    padding: 12,
    backgroundColor: '#E8F5E9',
    borderRadius: 8,
    marginTop: 16,
  },
  connectedText: {
    color: '#4CAF50',
    fontSize: 14,
    fontWeight: 'bold',
    marginLeft: 8,
  },
});

export default DeviceScanScreen;
