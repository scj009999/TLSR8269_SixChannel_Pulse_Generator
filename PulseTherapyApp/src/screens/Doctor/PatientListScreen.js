/**
 * 医生端 - 患者列表界面
 * 
 * 功能：
 * 1. 显示所有患者（状态标记：正常/需关注/待审批）
 * 2. 搜索筛选
 * 3. 分组管理
 * 4. 点击进入患者详情
 * 
 * 权限控制：
 * - 只有医生能访问此界面
 * - 需要医生认证（登录时验证）
 */

import React, { useState, useEffect } from 'react';
import {
  View,
  Text,
  TouchableOpacity,
  StyleSheet,
  FlatList,
  TextInput,
  ActivityIndicator,
  RefreshControl,
} from 'react-native';
import Icon from 'react-native-vector-icons/MaterialIcons';
import BluetoothService from '../../services/BluetoothService';

// 模拟数据（实际应从云端API获取）
const MOCK_PATIENTS = [
  {
    id: 'patient_001',
    name: '张*',
    age: 65,
    disease: '帕金森病',
    lastTreatment: '2026-05-25 10:00',
    status: 'normal',      // 正常
    symptomsScore: 3,        // UPDRS 评分 3/4
    needAttention: false,
    hasPendingRequest: false,
  },
  {
    id: 'patient_002',
    name: '李*',
    age: 72,
    disease: '帕金森叠加综合征',
    lastTreatment: '2026-05-24 09:30',
    status: 'attention',    // 需关注
    symptomsScore: 4,        // 恶化
    needAttention: true,
    hasPendingRequest: true,     // 有待审批参数调整
  },
  {
    id: 'patient_003',
    name: '王*',
    age: 68,
    disease: '老年痴呆',
    lastTreatment: '2026-05-20 14:00',
    status: 'pending',      // 待审批
    symptomsScore: 2,
    needAttention: false,
    hasPendingRequest: true,
  },
];

const PatientListScreen = ({ navigation }) => {
  const [patients, setPatients] = useState([]);
  const [filteredPatients, setFilteredPatients] = useState([]);
  const [searchQuery, setSearchQuery] = useState('');
  const [isLoading, setIsLoading] = useState(true);
  const [isRefreshing, setIsRefreshing] = useState(false);
  const [filterStatus, setFilterStatus] = useState('all'); // all, normal, attention, pending

  useEffect(() => {
    fetchPatients();
  }, []);

  /**
   * 获取患者列表（模拟）
   */
  const fetchPatients = async () => {
    try {
      // TODO: 实际应调用 API
      // const response = await axios.get('/api/v1/patients');
      // setPatients(response.data);
      
      // 模拟网络延迟
      setTimeout(() => {
        setPatients(MOCK_PATIENTS);
        setFilteredPatients(MOCK_PATIENTS);
        setIsLoading(false);
      }, 1000);
    } catch (error) {
      console.error('获取患者列表失败:', error);
      setIsLoading(false);
    }
  };

  /**
   * 下拉刷新
   */
  const handleRefresh = async () => {
    setIsRefreshing(true);
    await fetchPatients();
    setIsRefreshing(false);
  };

  /**
   * 搜索筛选
   */
  const handleSearch = (query) => {
    setSearchQuery(query);
    if (!query.trim()) {
      filterByStatus(filterStatus, MOCK_PATIENTS);
      return;
    }

    const filtered = patients.filter(p => 
      p.name.includes(query) || 
      p.disease.includes(query)
    );
    setFilteredPatients(filtered);
  };

  /**
   * 按状态筛选
   */
  const filterByStatus = (status, patientList = patients) => {
    setFilterStatus(status);
    if (status === 'all') {
      setFilteredPatients(patientList);
    } else {
      const filtered = patientList.filter(p => p.status === status);
      setFilteredPatients(filtered);
    }
  };

  /**
   * 渲染患者项
   */
  const renderPatientItem = ({ item }) => {
    const getStatusColor = () => {
      if (item.status === 'attention') return '#FF9800';  // 橙色：需关注
      if (item.status === 'pending') return '#F44336';   // 红色：待审批
      return '#4CAF50';  // 绿色：正常
    };

    const getStatusText = () => {
      if (item.status === 'attention') return '需关注';
      if (item.status === 'pending') return '待审批';
      return '正常';
    };

    return (
      <TouchableOpacity
        style={styles.patientItem}
        onPress={() => navigation.navigate('PatientDetail', { patientId: item.id })}
      >
        {/* 状态指示点 */}
        <View style={[styles.statusDot, { backgroundColor: getStatusColor() }]} />

        <View style={styles.patientInfo}>
          <View style={styles.patientHeader}>
            <Text style={styles.patientName}>{item.name}</Text>
            <Text style={styles.patientAge}>{item.age}岁</Text>
          </View>

          <Text style={styles.patientDisease}>{item.disease}</Text>

          <View style={styles.patientMeta}>
            <Text style={styles.lastTreatment}>
              上次治疗: {item.lastTreatment}
            </Text>
            <View style={styles.scoreBadge}>
              <Text style={styles.scoreText}>UPDRS: {item.symptomsScore}/4</Text>
            </View>
          </View>

          {/* 待审批标记 */}
          {item.hasPendingRequest && (
            <View style={styles.pendingBadge}>
              <Icon name="notifications-active" size={16} color="#FFF" />
              <Text style={styles.pendingText}>参数调整待审批</Text>
            </View>
          )}
        </View>

        <Icon name="chevron-right" size={24} color="#999" />
      </TouchableOpacity>
    );
  };

  /**
   * 渲染空状态
   */
  const renderEmptyState = () => (
    <View style={styles.emptyState}>
      <Icon name="people-outline" size={64} color="#E0E0E0" />
      <Text style={styles.emptyText}>暂无患者</Text>
      <Text style={styles.emptyHint}>
        {searchQuery ? '未找到匹配的患者' : '点击右上角 + 添加患者'}
      </Text>
    </View>
  );

  if (isLoading) {
    return (
      <View style={styles.loadingContainer}>
        <ActivityIndicator size="large" color="#2196F3" />
        <Text style={styles.loadingText}>加载中...</Text>
      </View>
    );
  }

  return (
    <View style={styles.container}>
      {/* 搜索栏 */}
      <View style={styles.searchBar}>
        <Icon name="search" size={20} color="#999" />
        <TextInput
          style={styles.searchInput}
          placeholder="搜索患者姓名或疾病"
          value={searchQuery}
          onChangeText={handleSearch}
        />
        {searchQuery ? (
          <TouchableOpacity onPress={() => handleSearch('')}>
            <Icon name="close" size={20} color="#999" />
          </TouchableOpacity>
        ) : null}
      </View>

      {/* 筛选标签 */}
      <View style={styles.filterTabs}>
        {[
          { key: 'all', label: '全部' },
          { key: 'normal', label: '正常' },
          { key: 'attention', label: '需关注' },
          { key: 'pending', label: '待审批' },
        ].map(tab => (
          <TouchableOpacity
            key={tab.key}
            style={[
              styles.filterTab,
              filterStatus === tab.key && styles.filterTabActive,
            ]}
            onPress={() => filterByStatus(tab.key)}
          >
            <Text
              style={[
                styles.filterTabText,
                filterStatus === tab.key && styles.filterTabTextActive,
              ]}
            >
              {tab.label}
            </Text>
          </TouchableOpacity>
        ))}
      </View>

      {/* 患者列表 */}
      <FlatList
        data={filteredPatients}
        renderItem={renderPatientItem}
        keyExtractor={item => item.id}
        contentContainerStyle={filteredPatients.length === 0 ? styles.emptyContainer : null}
        ListEmptyComponent={renderEmptyState}
        refreshControl={
          <RefreshControl
            refreshing={isRefreshing}
            onRefresh={handleRefresh}
            colors={['#2196F3']}
          />
        }
      />

      {/* 添加患者按钮 */}
      <TouchableOpacity
        style={styles.addButton}
        onPress={() => navigation.navigate('AddPatient')}
      >
        <Icon name="add" size={24} color="#FFF" />
      </TouchableOpacity>
    </View>
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
  searchBar: {
    flexDirection: 'row',
    alignItems: 'center',
    padding: 12,
    backgroundColor: '#FFF',
    borderBottomWidth: 1,
    borderBottomColor: '#E0E0E0',
  },
  searchInput: {
    flex: 1,
    marginLeft: 8,
    fontSize: 14,
    color: '#333',
  },
  filterTabs: {
    flexDirection: 'row',
    padding: 12,
    backgroundColor: '#FFF',
    borderBottomWidth: 1,
    borderBottomColor: '#E0E0E0',
  },
  filterTab: {
    flex: 1,
    paddingVertical: 8,
    alignItems: 'center',
    borderRadius: 16,
    marginHorizontal: 4,
    backgroundColor: '#F5F5F5',
  },
  filterTabActive: {
    backgroundColor: '#2196F3',
  },
  filterTabText: {
    fontSize: 12,
    color: '#666',
  },
  filterTabTextActive: {
    color: '#FFF',
    fontWeight: 'bold',
  },
  patientItem: {
    flexDirection: 'row',
    alignItems: 'center',
    padding: 16,
    backgroundColor: '#FFF',
    borderBottomWidth: 1,
    borderBottomColor: '#E0E0E0',
  },
  statusDot: {
    width: 12,
    height: 12,
    borderRadius: 6,
    marginRight: 12,
  },
  patientInfo: {
    flex: 1,
  },
  patientHeader: {
    flexDirection: 'row',
    alignItems: 'center',
    marginBottom: 4,
  },
  patientName: {
    fontSize: 16,
    fontWeight: 'bold',
    color: '#333',
    marginRight: 8,
  },
  patientAge: {
    fontSize: 12,
    color: '#999',
  },
  patientDisease: {
    fontSize: 14,
    color: '#666',
    marginBottom: 4,
  },
  patientMeta: {
    flexDirection: 'row',
    alignItems: 'center',
    justifyContent: 'space-between',
  },
  lastTreatment: {
    fontSize: 12,
    color: '#999',
  },
  scoreBadge: {
    backgroundColor: '#E3F2FD',
    paddingHorizontal: 8,
    paddingVertical: 4,
    borderRadius: 12,
  },
  scoreText: {
    fontSize: 12,
    color: '#2196F3',
    fontWeight: 'bold',
  },
  pendingBadge: {
    flexDirection: 'row',
    alignItems: 'center',
    marginTop: 8,
    padding: 8,
    backgroundColor: '#FFEBEE',
    borderRadius: 8,
  },
  pendingText: {
    marginLeft: 4,
    fontSize: 12,
    color: '#F44336',
    fontWeight: 'bold',
  },
  emptyContainer: {
    flex: 1,
  },
  emptyState: {
    flex: 1,
    alignItems: 'center',
    justifyContent: 'center',
    padding: 32,
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
    textAlign: 'center',
  },
  addButton: {
    position: 'absolute',
    right: 16,
    bottom: 16,
    width: 56,
    height: 56,
    borderRadius: 28,
    backgroundColor: '#2196F3',
    alignItems: 'center',
    justifyContent: 'center',
    elevation: 4,
  },
});

export default PatientListScreen;
