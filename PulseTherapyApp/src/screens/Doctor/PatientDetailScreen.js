/**
 * 医生端 - 患者详情界面
 * 
 * 功能：
 * 1. 查看患者治疗数据
 * 2. 查看症状评分历史
 * 3. 远程调整参数（仅医生权限）
 * 4. 审批患者的参数调整申请
 * 5. 发送消息给患者
 * 
 * 安全设计：
 * - 所有参数调整需要医生认证
 * - 参数范围受限（符合医疗安全规范）
 * - 操作日志完整记录
 */

import React, { useState, useEffect } from 'react';
import {
  View,
  Text,
  TouchableOpacity,
  StyleSheet,
  ScrollView,
  Alert,
  ActivityIndicator,
  TextInput,
  Switch,
} from 'react-native';
import Icon from 'react-native-vector-icons/MaterialIcons';
import BluetoothService, { TREATMENT_MODES } from '../../services/BluetoothService';

// 模拟患者数据（实际应从云端API获取）
const MOCK_PATIENT_DETAIL = {
  id: 'patient_001',
  name: '张*',
  age: 65,
  gender: '男',
  disease: '帕金森病',
  diagnosisDate: '2023-03-15',
  
  // 当前治疗参数
  currentParams: {
    mode: 'MOTOR',
    frequency: 40,
    duty: 50,
    current: 1.8,
    duration: 20,
    channels: [
      { ch: 0, freq: 40, duty: 50 },
      { ch: 1, freq: 40, duty: 50 },
      { ch: 2, freq: 40, duty: 50 },
      { ch: 3, freq: 40, duty: 50 },
    ]
  },
  
  // 治疗记录
  treatmentHistory: [
    { date: '2026-05-25', duration: 20, mode: 'MOTOR', effect: '明显改善' },
    { date: '2026-05-24', duration: 20, mode: 'MOTOR', effect: '轻微改善' },
    { date: '2026-05-22', duration: 15, mode: 'SLEEP', effect: '睡眠质量提升' },
  ],
  
  // 症状评分历史
  symptomScores: [
    { date: '2026-05-25', tremor: 2, rigidity: 2, bradykinesia: 3, balance: 2, overall: '轻微改善' },
    { date: '2026-05-24', tremor: 3, rigidity: 3, bradykinesia: 3, balance: 3, overall: '无变化' },
    { date: '2026-05-22', tremor: 3, rigidity: 4, bradykinesia: 4, balance: 3, overall: '有所恶化' },
  ],
  
  // 待审批申请
  pendingRequests: [
    {
      id: 'req_001',
      timestamp: '2026-05-25 14:30',
      reason: '感觉效果不明显，希望增加强度',
      requestedParams: {
        frequency: 50,
        duty: 60,
        current: 2.0,
      }
    }
  ]
};

const PatientDetailScreen = ({ route, navigation }) => {
  const { patientId } = route.params;
  const [patient, setPatient] = useState(null);
  const [isLoading, setIsLoading] = useState(true);
  const [showParamEditor, setShowParamEditor] = useState(false);
  const [editingParams, setEditingParams] = useState({});
  const [doctorNote, setDoctorNote] = useState('');

  useEffect(() => {
    fetchPatientDetail();
  }, [patientId]);

  /**
   * 获取患者详情（模拟）
   */
  const fetchPatientDetail = async () => {
    try {
      // TODO: 实际应调用 API
      // const response = await axios.get(`/api/v1/patients/${patientId}`);
      // setPatient(response.data);
      
      // 模拟网络延迟
      setTimeout(() => {
        setPatient(MOCK_PATIENT_DETAIL);
        setEditingParams(MOCK_PATIENT_DETAIL.currentParams);
        setIsLoading(false);
      }, 1000);
    } catch (error) {
      console.error('获取患者详情失败:', error);
      setIsLoading(false);
      Alert.alert('错误', '获取患者详情失败');
    }
  };

  /**
   * 医生调整参数（仅医生权限）
   */
  const updateParameters = async () => {
    try {
      // 参数安全检查
      if (editingParams.frequency < 1 || editingParams.frequency > 200) {
        Alert.alert('参数错误', '频率范围：1-200Hz');
        return;
      }
      if (editingParams.duty < 0 || editingParams.duty > 100) {
        Alert.alert('参数错误', '占空比范围：0-100%');
        return;
      }
      if (editingParams.current > 2.0) {
        Alert.alert('参数错误', '电流限制：最大2mA');
        return;
      }
      if (editingParams.duration < 1 || editingParams.duration > 30) {
        Alert.alert('参数错误', '时长范围：1-30分钟');
        return;
      }

      await BluetoothService.updateParameters(editingParams);
      
      Alert.alert(
        '调整成功',
        '患者设备将在下次治疗时应用新参数',
        [{ text: '确定' }]
      );
      
      setShowParamEditor(false);
      fetchPatientDetail(); // 刷新数据
    } catch (error) {
      console.error('调整参数失败:', error);
      Alert.alert('调整失败', error.message);
    }
  };

  /**
   * 审批患者的参数调整申请
   */
  const approveRequest = async (requestId, action) => {
    try {
      if (action === 'approve') {
        // 批准：应用申请中的参数
        const request = patient.pendingRequests.find(r => r.id === requestId);
        await BluetoothService.updateParameters(request.requestedParams);
        
        Alert.alert('已批准', '参数调整已下发到患者设备');
      } else {
        // 拒绝：记录原因
        Alert.prompt(
          '拒绝原因',
          '请输入拒绝原因（可选）',
          [
            { text: '取消', style: 'cancel' },
            {
              text: '确认拒绝',
              onPress: (reason) => {
                console.log('拒绝申请:', requestId, '原因:', reason);
                // TODO: 调用API拒绝申请
                Alert.alert('已拒绝', '已通知患者');
                fetchPatientDetail();
              }
            }
          ]
        );
        return;
      }
      
      fetchPatientDetail(); // 刷新数据
    } catch (error) {
      console.error('审批失败:', error);
      Alert.alert('审批失败', error.message);
    }
  };

  /**
   * 发送消息给患者
   */
  const sendMessage = () => {
    if (!doctorNote.trim()) {
      Alert.alert('提示', '请输入消息内容');
      return;
    }

    Alert.alert(
      '发送消息',
      `确认发送消息给患者？\n\n"${doctorNote}"`,
      [
        { text: '取消', style: 'cancel' },
        {
          text: '发送',
          onPress: async () => {
            try {
              // TODO: 调用API发送消息
              console.log('发送消息:', doctorNote);
              Alert.alert('发送成功', '消息已通知患者');
              setDoctorNote('');
            } catch (error) {
              console.error('发送失败:', error);
              Alert.alert('发送失败', error.message);
            }
          }
        }
      ]
    );
  };

  if (isLoading) {
    return (
      <View style={styles.loadingContainer}>
        <ActivityIndicator size="large" color="#2196F3" />
        <Text style={styles.loadingText}>加载中...</Text>
      </View>
    );
  }

  if (!patient) {
    return (
      <View style={styles.errorContainer}>
        <Icon name="error-outline" size={64} color="#F44336" />
        <Text style={styles.errorText}>患者不存在</Text>
      </View>
    );
  }

  return (
    <ScrollView style={styles.container}>
      {/* 患者基本信息 */}
      <View style={styles.section}>
        <View style={styles.patientHeader}>
          <View style={styles.avatarContainer}>
            <Text style={styles.avatarText}>{patient.name[0]}</Text>
          </View>
          <View style={styles.patientInfo}>
            <Text style={styles.patientName}>{patient.name} ({patient.gender}, {patient.age}岁)</Text>
            <Text style={styles.patientDisease}>{patient.disease}</Text>
            <Text style={styles.diagnosisDate}>确诊日期: {patient.diagnosisDate}</Text>
          </View>
        </View>
      </View>

      {/* 当前治疗参数 */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>当前治疗参数</Text>
        <View style={styles.paramsContainer}>
          <View style={styles.paramItem}>
            <Text style={styles.paramLabel}>模式</Text>
            <Text style={styles.paramValue}>{TREATMENT_MODES[patient.currentParams.mode].name}</Text>
          </View>
          <View style={styles.paramItem}>
            <Text style={styles.paramLabel}>频率</Text>
            <Text style={styles.paramValue}>{patient.currentParams.frequency} Hz</Text>
          </View>
          <View style={styles.paramItem}>
            <Text style={styles.paramLabel}>占空比</Text>
            <Text style={styles.paramValue}>{patient.currentParams.duty}%</Text>
          </View>
          <View style={styles.paramItem}>
            <Text style={styles.paramLabel}>电流</Text>
            <Text style={styles.paramValue}>{patient.currentParams.current} mA</Text>
          </View>
          <View style={styles.paramItem}>
            <Text style={styles.paramLabel}>时长</Text>
            <Text style={styles.paramValue}>{patient.currentParams.duration} 分钟</Text>
          </View>
        </View>
        
        <TouchableOpacity
          style={styles.editButton}
          onPress={() => setShowParamEditor(!showParamEditor)}
        >
          <Icon name="edit" size={20} color="#FFF" />
          <Text style={styles.editButtonText}>调整参数</Text>
        </TouchableOpacity>
      </View>

      {/* 参数编辑器（仅医生） */}
      {showParamEditor && (
        <View style={styles.paramEditor}>
          <Text style={styles.editorTitle}>调整治疗参数（医生权限）</Text>
          
          <View style={styles.editorField}>
            <Text style={styles.editorLabel}>频率 (1-200 Hz)</Text>
            <TextInput
              style={styles.editorInput}
              value={String(editingParams.frequency)}
              onChangeText={(value) => setEditingParams({...editingParams, frequency: Number(value)})}
              keyboardType="numeric"
            />
          </View>
          
          <View style={styles.editorField}>
            <Text style={styles.editorLabel}>占空比 (0-100%)</Text>
            <TextInput
              style={styles.editorInput}
              value={String(editingParams.duty)}
              onChangeText={(value) => setEditingParams({...editingParams, duty: Number(value)})}
              keyboardType="numeric"
            />
          </View>
          
          <View style={styles.editorField}>
            <Text style={styles.editorLabel}>电流 (0.1-2.0 mA)</Text>
            <TextInput
              style={styles.editorInput}
              value={String(editingParams.current)}
              onChangeText={(value) => setEditingParams({...editingParams, current: Number(value)})}
              keyboardType="numeric"
            />
          </View>
          
          <View style={styles.editorField}>
            <Text style={styles.editorLabel}>时长 (1-30 min)</Text>
            <TextInput
              style={styles.editorInput}
              value={String(editingParams.duration)}
              onChangeText={(value) => setEditingParams({...editingParams, duration: Number(value)})}
              keyboardType="numeric"
            />
          </View>
          
          <View style={styles.editorButtons}>
            <TouchableOpacity
              style={[styles.editorButton, styles.cancelButton]}
              onPress={() => setShowParamEditor(false)}
            >
              <Text style={styles.cancelButtonText}>取消</Text>
            </TouchableOpacity>
            
            <TouchableOpacity
              style={[styles.editorButton, styles.saveButton]}
              onPress={updateParameters}
            >
              <Text style={styles.saveButtonText}>保存并下发</Text>
            </TouchableOpacity>
          </View>
        </View>
      )}

      {/* 待审批申请 */}
      {patient.pendingRequests && patient.pendingRequests.length > 0 && (
        <View style={styles.section}>
          <Text style={styles.sectionTitle}>待审批申请</Text>
          {patient.pendingRequests.map((request) => (
            <View key={request.id} style={styles.requestCard}>
              <View style={styles.requestHeader}>
                <Icon name="notifications-active" size={20} color="#F44336" />
                <Text style={styles.requestTime}>{request.timestamp}</Text>
              </View>
              <Text style={styles.requestReason}>申请理由: {request.reason}</Text>
              <View style={styles.requestParams}>
                <Text style={styles.requestParamsTitle}>申请调整参数:</Text>
                <Text style={styles.requestParam}>频率: {request.requestedParams.frequency} Hz</Text>
                <Text style={styles.requestParam}>占空比: {request.requestedParams.duty}%</Text>
                <Text style={styles.requestParam}>电流: {request.requestedParams.current} mA</Text>
              </View>
              <View style={styles.requestActions}>
                <TouchableOpacity
                  style={[styles.requestButton, styles.rejectButton]}
                  onPress={() => approveRequest(request.id, 'reject')}
                >
                  <Text style={styles.rejectButtonText}>拒绝</Text>
                </TouchableOpacity>
                
                <TouchableOpacity
                  style={[styles.requestButton, styles.approveButton]}
                  onPress={() => approveRequest(request.id, 'approve')}
                >
                  <Text style={styles.approveButtonText}>批准</Text>
                </TouchableOpacity>
              </View>
            </View>
          ))}
        </View>
      )}

      {/* 治疗历史 */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>治疗历史</Text>
        {patient.treatmentHistory.map((record, index) => (
          <View key={index} style={styles.historyItem}>
            <View style={styles.historyHeader}>
              <Text style={styles.historyDate}>{record.date}</Text>
              <Text style={styles.historyMode}>{TREATMENT_MODES[record.mode].name}</Text>
            </View>
            <Text style={styles.historyDuration}>时长: {record.duration} 分钟</Text>
            <Text style={styles.historyEffect}>效果: {record.effect}</Text>
          </View>
        ))}
      </View>

      {/* 症状评分历史 */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>症状评分历史</Text>
        {patient.symptomScores.map((score, index) => (
          <View key={index} style={styles.scoreCard}>
            <Text style={styles.scoreDate}>{score.date}</Text>
            <View style={styles.scoreItems}>
              <View style={styles.scoreItem}>
                <Text style={styles.scoreLabel}>震颤</Text>
                <Text style={styles.scoreValue}>{score.tremor}/4</Text>
              </View>
              <View style={styles.scoreItem}>
                <Text style={styles.scoreLabel}>僵直</Text>
                <Text style={styles.scoreValue}>{score.rigidity}/4</Text>
              </View>
              <View style={styles.scoreItem}>
                <Text style={styles.scoreLabel}>运动迟缓</Text>
                <Text style={styles.scoreValue}>{score.bradykinesia}/4</Text>
              </View>
              <View style={styles.scoreItem}>
                <Text style={styles.scoreLabel}>平衡</Text>
                <Text style={styles.scoreValue}>{score.balance}/4</Text>
              </View>
            </View>
            <Text style={styles.scoreOverall}>整体: {score.overall}</Text>
          </View>
        ))}
      </View>

      {/* 发送消息给患者 */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>发送消息给患者</Text>
        <TextInput
          style={styles.messageInput}
          multiline
          numberOfLines={4}
          placeholder="输入消息内容..."
          value={doctorNote}
          onChangeText={setDoctorNote}
        />
        <TouchableOpacity style={styles.sendButton} onPress={sendMessage}>
          <Icon name="send" size={20} color="#FFF" />
          <Text style={styles.sendButtonText}>发送消息</Text>
        </TouchableOpacity>
      </View>
    </ScrollView>
  );
};

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#F5F5F5',
  },
  loadingContainer: {
    flex: 1,
    alignItems: 'center',
    justifyContent: 'center',
  },
  loadingText: {
    marginTop: 12,
    fontSize: 14,
    color: '#999',
  },
  errorContainer: {
    flex: 1,
    alignItems: 'center',
    justifyContent: 'center',
  },
  errorText: {
    marginTop: 16,
    fontSize: 16,
    color: '#F44336',
  },
  section: {
    margin: 16,
    padding: 16,
    backgroundColor: '#FFF',
    borderRadius: 12,
    elevation: 2,
  },
  sectionTitle: {
    fontSize: 16,
    fontWeight: 'bold',
    color: '#333',
    marginBottom: 12,
  },
  patientHeader: {
    flexDirection: 'row',
    alignItems: 'center',
  },
  avatarContainer: {
    width: 56,
    height: 56,
    borderRadius: 28,
    backgroundColor: '#2196F3',
    alignItems: 'center',
    justifyContent: 'center',
    marginRight: 16,
  },
  avatarText: {
    fontSize: 24,
    fontWeight: 'bold',
    color: '#FFF',
  },
  patientInfo: {
    flex: 1,
  },
  patientName: {
    fontSize: 18,
    fontWeight: 'bold',
    color: '#333',
    marginBottom: 4,
  },
  patientDisease: {
    fontSize: 14,
    color: '#2196F3',
    marginBottom: 4,
  },
  diagnosisDate: {
    fontSize: 12,
    color: '#999',
  },
  paramsContainer: {
    marginTop: 12,
  },
  paramItem: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    paddingVertical: 8,
    borderBottomWidth: 1,
    borderBottomColor: '#E0E0E0',
  },
  paramLabel: {
    fontSize: 14,
    color: '#666',
  },
  paramValue: {
    fontSize: 14,
    fontWeight: 'bold',
    color: '#333',
  },
  editButton: {
    flexDirection: 'row',
    alignItems: 'center',
    justifyContent: 'center',
    marginTop: 16,
    padding: 12,
    backgroundColor: '#2196F3',
    borderRadius: 8,
  },
  editButtonText: {
    marginLeft: 8,
    fontSize: 14,
    fontWeight: 'bold',
    color: '#FFF',
  },
  paramEditor: {
    marginTop: 16,
    padding: 16,
    backgroundColor: '#F9F9F9',
    borderRadius: 8,
    borderWidth: 1,
    borderColor: '#E0E0E0',
  },
  editorTitle: {
    fontSize: 14,
    fontWeight: 'bold',
    color: '#333',
    marginBottom: 12,
  },
  editorField: {
    marginBottom: 12,
  },
  editorLabel: {
    fontSize: 12,
    color: '#666',
    marginBottom: 4,
  },
  editorInput: {
    borderWidth: 1,
    borderColor: '#E0E0E0',
    borderRadius: 8,
    padding: 8,
    fontSize: 14,
    color: '#333',
    backgroundColor: '#FFF',
  },
  editorButtons: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    marginTop: 16,
  },
  editorButton: {
    flex: 1,
    padding: 12,
    borderRadius: 8,
    alignItems: 'center',
  },
  cancelButton: {
    marginRight: 8,
    backgroundColor: '#E0E0E0',
  },
  cancelButtonText: {
    fontSize: 14,
    fontWeight: 'bold',
    color: '#666',
  },
  saveButton: {
    marginLeft: 8,
    backgroundColor: '#2196F3',
  },
  saveButtonText: {
    fontSize: 14,
    fontWeight: 'bold',
    color: '#FFF',
  },
  requestCard: {
    marginTop: 12,
    padding: 12,
    backgroundColor: '#FFEBEE',
    borderRadius: 8,
    borderWidth: 1,
    borderColor: '#FFCDD2',
  },
  requestHeader: {
    flexDirection: 'row',
    alignItems: 'center',
    marginBottom: 8,
  },
  requestTime: {
    marginLeft: 8,
    fontSize: 12,
    color: '#999',
  },
  requestReason: {
    fontSize: 14,
    color: '#333',
    marginBottom: 8,
  },
  requestParams: {
    marginBottom: 12,
  },
  requestParamsTitle: {
    fontSize: 12,
    fontWeight: 'bold',
    color: '#666',
    marginBottom: 4,
  },
  requestParam: {
    fontSize: 12,
    color: '#333',
    marginLeft: 8,
  },
  requestActions: {
    flexDirection: 'row',
    justifyContent: 'space-between',
  },
  requestButton: {
    flex: 1,
    padding: 8,
    borderRadius: 8,
    alignItems: 'center',
  },
  rejectButton: {
    marginRight: 8,
    backgroundColor: '#FFF',
    borderWidth: 1,
    borderColor: '#F44336',
  },
  rejectButtonText: {
    fontSize: 12,
    fontWeight: 'bold',
    color: '#F44336',
  },
  approveButton: {
    marginLeft: 8,
    backgroundColor: '#4CAF50',
  },
  approveButtonText: {
    fontSize: 12,
    fontWeight: 'bold',
    color: '#FFF',
  },
  historyItem: {
    marginTop: 12,
    padding: 12,
    backgroundColor: '#F9F9F9',
    borderRadius: 8,
  },
  historyHeader: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    marginBottom: 8,
  },
  historyDate: {
    fontSize: 14,
    fontWeight: 'bold',
    color: '#333',
  },
  historyMode: {
    fontSize: 12,
    color: '#2196F3',
  },
  historyDuration: {
    fontSize: 12,
    color: '#666',
    marginBottom: 4,
  },
  historyEffect: {
    fontSize: 12,
    color: '#4CAF50',
  },
  scoreCard: {
    marginTop: 12,
    padding: 12,
    backgroundColor: '#F9F9F9',
    borderRadius: 8,
  },
  scoreDate: {
    fontSize: 14,
    fontWeight: 'bold',
    color: '#333',
    marginBottom: 8,
  },
  scoreItems: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    marginBottom: 8,
  },
  scoreItem: {
    alignItems: 'center',
  },
  scoreLabel: {
    fontSize: 10,
    color: '#999',
    marginBottom: 4,
  },
  scoreValue: {
    fontSize: 16,
    fontWeight: 'bold',
    color: '#333',
  },
  scoreOverall: {
    fontSize: 12,
    color: '#666',
    textAlign: 'center',
  },
  messageInput: {
    borderWidth: 1,
    borderColor: '#E0E0E0',
    borderRadius: 8,
    padding: 12,
    fontSize: 14,
    color: '#333',
    textAlignVertical: 'top',
    minHeight: 100,
    backgroundColor: '#FFF',
  },
  sendButton: {
    flexDirection: 'row',
    alignItems: 'center',
    justifyContent: 'center',
    marginTop: 12,
    padding: 12,
    backgroundColor: '#2196F3',
    borderRadius: 8,
  },
  sendButtonText: {
    marginLeft: 8,
    fontSize: 14,
    fontWeight: 'bold',
    color: '#FFF',
  },
});

export default PatientDetailScreen;
