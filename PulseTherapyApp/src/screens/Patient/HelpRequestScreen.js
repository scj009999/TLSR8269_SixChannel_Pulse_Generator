/**
 * 求助申请界面 - 患者端
 * 
 * 功能：
 * 1. 患者感觉效果不明显时，发出求助申请
 * 2. 填写症状描述
 * 3. 发送给志愿者医生
 * 4. 按需联网（平时不联网）
 * 
 * 安全设计：
 * - 只有"申请调整"，不能自己调参数
 * - 医生审批后，设备自动同步新参数
 */

import React, { useState } from 'react';
import {
  View,
  Text,
  TextInput,
  TouchableOpacity,
  StyleSheet,
  Alert,
  ScrollView,
  ActivityIndicator,
} = 'react-native';
import Icon from 'react-native-vector-icons/MaterialIcons';
import BluetoothService from '../../services/BluetoothService';

const HelpRequestScreen = ({ navigation }) => {
  const [symptoms, setSymptoms] = useState('');
  const [duration, setDuration] = useState('');  // 治疗效果持续时间
  const [intensity, setIntensity] = useState(''); // 当前感觉强度
  const [isSubmitting, setIsSubmitting] = useState(false);

  /**
   * 提交求助申请（按需联网）
   */
  const submitRequest = async () => {
    if (!symptoms.trim()) {
      Alert.alert('请填写症状', '请描述您当前的症状或感受');
      return;
    }

    setIsSubmitting(true);

    try {
      // 按需联网：患者发出请求时，才连接网络
      const helpData = {
        symptoms: symptoms.trim(),
        duration: duration ? parseInt(duration) : null,
        intensity: intensity ? parseInt(intensity) : null,
        timestamp: new Date().toISOString(),
        deviceConnected: BluetoothService.isConnected,
      };

      // 调用蓝牙服务发送求助请求
      await BluetoothService.requestDoctorHelp(
        'patient_001',  // TODO: 从用户配置读取
        helpData
      );

      Alert.alert(
        '求助申请已发送',
        '志愿者医生收到您的申请后，会通过远程协助调整治疗参数。请耐心等待。',
        [
          {
            text: '确定',
            onPress: () => navigation.navigate('Home'),
          },
        ]
      );

      setSymptoms('');
      setDuration('');
      setIntensity('');
    } catch (error) {
      console.error('提交失败:', error);
      Alert.alert('发送失败', error.message);
    } finally {
      setIsSubmitting(false);
    }
  };

  return (
    <ScrollView style={styles.container}>
      {/* 顶部说明 */}
      <View style={styles.header}>
        <Icon name="help-outline" size={48} color="#2196F3" />
        <Text style={styles.title}>申请医生协助</Text>
        <Text style={styles.subtitle}>
          当固定治疗模式效果不明显时，您可以申请志愿者医生远程协助调整参数。
        </Text>
      </View>

      {/* 症状描述（必填） */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>当前症状描述 *</Text>
        <Text style={styles.sectionHint}>
          请详细描述您当前的症状（如：震颤加重、僵硬感增强、疲劳等）
        </Text>
        <TextInput
          style={styles.textInput}
          multiline
          numberOfLines={6}
          placeholder="例如：治疗后效果不明显，右手震颤仍然较重..."
          value={symptoms}
          onChangeText={setSymptoms}
          maxLength={500}
        />
        <Text style={styles.charCount}>{symptoms.length}/500</Text>
      </View>

      {/* 治疗效果持续时间（选填） */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>上次治疗效果持续多久？（选填）</Text>
        <View style={styles.inputRow}>
          <TextInput
            style={styles.numberInput}
            keyboardType="numeric"
            placeholder="小时"
            value={duration}
            onChangeText={setDuration}
          />
          <Text style={styles.inputUnit}>小时</Text>
        </View>
      </View>

      {/* 当前感觉强度（选填） */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>当前症状强度（选填）</Text>
        <View style={styles.intensityContainer}>
          {[1, 2, 3, 4, 5].map((level) => (
            <TouchableOpacity
              key={level}
              style={[
                styles.intensityButton,
                intensity === String(level) && styles.intensityButtonActive,
              ]}
              onPress={() => setIntensity(String(level))}
            >
              <Text
                style={[
                  styles.intensityButtonText,
                  intensity === String(level) && styles.intensityButtonTextActive,
                ]}
              >
                {level}
              </Text>
            </TouchableOpacity>
          ))}
        </View>
        <View style={styles.intensityLabel}>
          <Text style={styles.intensityLabelText}>轻微</Text>
          <Text style={styles.intensityLabelText}>严重</Text>
        </View>
      </View>

      {/* 安全提示 */}
      <View style={styles.safetyTip}>
        <Icon name="info" size={20} color="#FF9800" />
        <Text style={styles.safetyTipText}>
          提示：高级参数调整只能由医生执行。您的申请提交后，志愿者医生会在24小时内审核并远程调整。
        </Text>
      </View>

      {/* 提交按钮 */}
      <TouchableOpacity
        style={[styles.submitButton, isSubmitting && styles.submitButtonDisabled]}
        onPress={submitRequest}
        disabled={isSubmitting}
      >
        {isSubmitting ? (
          <ActivityIndicator size="small" color="#FFF" />
        ) : (
          <Icon name="send" size={24} color="#FFF" />
        )}
        <Text style={styles.submitButtonText}>
          {isSubmitting ? '发送中...' : '发送求助申请'}
        </Text>
      </TouchableOpacity>

      {/* 底部说明 */}
      <View style={styles.footer}>
        <Text style={styles.footerText}>
          依据：昌平实验室刘河生教授团队研究（Nature 2026）
        </Text>
        <Text style={styles.footerText}>
          本项目为公益开源，不收取任何费用。
        </Text>
      </View>
    </ScrollView>
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
    padding: 16,
    backgroundColor: '#E3F2FD',
    borderRadius: 12,
  },
  title: {
    fontSize: 20,
    fontWeight: 'bold',
    color: '#1976D2',
    marginTop: 12,
  },
  subtitle: {
    fontSize: 14,
    color: '#666',
    marginTop: 8,
    textAlign: 'center',
    lineHeight: 20,
  },
  section: {
    marginBottom: 20,
    padding: 16,
    backgroundColor: '#FFF',
    borderRadius: 12,
    elevation: 2,
  },
  sectionTitle: {
    fontSize: 16,
    fontWeight: 'bold',
    color: '#333',
    marginBottom: 4,
  },
  sectionHint: {
    fontSize: 12,
    color: '#999',
    marginBottom: 12,
    lineHeight: 16,
  },
  textInput: {
    borderWidth: 1,
    borderColor: '#E0E0E0',
    borderRadius: 8,
    padding: 12,
    fontSize: 16,
    color: '#333',
    textAlignVertical: 'top',
    minHeight: 120,
  },
  charCount: {
    fontSize: 12,
    color: '#999',
    textAlign: 'right',
    marginTop: 4,
  },
  inputRow: {
    flexDirection: 'row',
    alignItems: 'center',
  },
  numberInput: {
    flex: 1,
    borderWidth: 1,
    borderColor: '#E0E0E0',
    borderRadius: 8,
    padding: 12,
    fontSize: 16,
    color: '#333',
  },
  inputUnit: {
    fontSize: 16,
    color: '#666',
    marginLeft: 8,
  },
  intensityContainer: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    marginVertical: 8,
  },
  intensityButton: {
    width: 48,
    height: 48,
    borderRadius: 24,
    borderWidth: 2,
    borderColor: '#2196F3',
    alignItems: 'center',
    justifyContent: 'center',
  },
  intensityButtonActive: {
    backgroundColor: '#2196F3',
  },
  intensityButtonText: {
    fontSize: 18,
    fontWeight: 'bold',
    color: '#2196F3',
  },
  intensityButtonTextActive: {
    color: '#FFF',
  },
  intensityLabel: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    marginTop: 4,
  },
  intensityLabelText: {
    fontSize: 12,
    color: '#999',
  },
  safetyTip: {
    flexDirection: 'row',
    padding: 12,
    backgroundColor: '#FFF3E0',
    borderRadius: 8,
    marginBottom: 20,
  },
  safetyTipText: {
    flex: 1,
    fontSize: 14,
    color: '#E65100',
    marginLeft: 8,
    lineHeight: 20,
  },
  submitButton: {
    flexDirection: 'row',
    alignItems: 'center',
    justifyContent: 'center',
    padding: 16,
    backgroundColor: '#2196F3',
    borderRadius: 12,
    elevation: 4,
    marginBottom: 20,
  },
  submitButtonDisabled: {
    backgroundColor: '#90CAF9',
  },
  submitButtonText: {
    color: '#FFF',
    fontSize: 18,
    fontWeight: 'bold',
    marginLeft: 8,
  },
  footer: {
    alignItems: 'center',
    marginBottom: 32,
  },
  footerText: {
    fontSize: 12,
    color: '#999',
    textAlign: 'center',
    lineHeight: 16,
  },
});

export default HelpRequestScreen;
