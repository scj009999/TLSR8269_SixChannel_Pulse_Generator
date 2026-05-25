/**
 * TLSR8269 六通道脉冲发生器 - React Native APP
 * 
 * 导航结构：
 * - 患者端：首页、记录、我的
 * - 医生端：患者列表、审批、我的
 * 
 * 安全设计：
 * - 患者端只能选择固定模式
 * - 医生端可以调整参数
 */

import React, { useState, useEffect } from 'react';
import { NavigationContainer } from '@react-navigation/native';
import { createBottomTabNavigator } from '@react-navigation/bottom-tabs';
import { createStackNavigator } from '@react-navigation/stack';
import Icon from 'react-native-vector-icons/MaterialIcons';

import BluetoothService from './src/services/BluetoothService';

// 导入屏幕
import HomeScreen from './src/screens/Patient/HomeScreen';
import DeviceScanScreen from './src/screens/Patient/DeviceScanScreen';
import HelpRequestScreen from './src/screens/Patient/HelpRequestScreen';
import PatientListScreen from './src/screens/Doctor/PatientListScreen';
import PatientDetailScreen from './src/screens/Doctor/PatientDetailScreen';

const Tab = createBottomTabNavigator();
const Stack = createStackNavigator();

/**
 * 患者端底部导航
 */
const PatientTabs = () => {
  return (
    <Tab.Navigator
      screenOptions={({ route }) => ({
        tabBarIcon: ({ focused, color, size }) => {
          let iconName;

          if (route.name === 'Home') {
            iconName = 'home';
          } else if (route.name === 'Records') {
            iconName = 'assessment';
          } else if (route.name === 'Profile') {
            iconName = 'person';
          }

          return <Icon name={iconName} size={size} color={color} />;
        },
        tabBarActiveTintColor: '#2196F3',
        tabBarInactiveTintColor: 'gray',
      })}
    >
      <Tab.Screen 
        name="Home" 
        component={HomeScreen}
        options={{ title: '治疗' }}
      />
      <Tab.Screen 
        name="Records" 
        component={RecordsScreen}
        options={{ title: '记录' }}
      />
      <Tab.Screen 
        name="Profile" 
        component={ProfileScreen}
        options={{ title: '我的' }}
      />
    </Tab.Navigator>
  );
};

/**
 * 医生端底部导航
 */
const DoctorTabs = () => {
  return (
    <Tab.Navigator
      screenOptions={({ route }) => ({
        tabBarIcon: ({ focused, color, size }) => {
          let iconName;

          if (route.name === 'Patients') {
            iconName = 'people';
          } else if (route.name === 'Approvals') {
            iconName = 'approval';
          } else if (route.name === 'DoctorProfile') {
            iconName = 'person';
          }

          return <Icon name={iconName} size={size} color={color} />;
        },
        tabBarActiveTintColor: '#4CAF50',
        tabBarInactiveTintColor: 'gray',
      })}
    >
      <Tab.Screen 
        name="Patients" 
        component={PatientListScreen}
        options={{ title: '患者' }}
      />
      <Tab.Screen 
        name="Approvals" 
        component={ApprovalScreen}
        options={{ title: '审批' }}
      />
      <Tab.Screen 
        name="DoctorProfile" 
        component={DoctorProfileScreen}
        options={{ title: '我的' }}
      />
    </Tab.Navigator>
  );
};

/**
 * 主应用组件
 */
const App = () => {
  const [isDoctor, setIsDoctor] = useState(false);
  const [isAuthenticated, setIsAuthenticated] = useState(false);

  useEffect(() => {
    // 检查用户角色（从存储或服务器）
    checkUserRole();
  }, []);

  const checkUserRole = async () => {
    // TODO: 从 AsyncStorage 或 API 获取用户角色
    // 示例：const role = await AsyncStorage.getItem('userRole');
    // setIsDoctor(role === 'doctor');
    
    // 暂时默认为患者端
    setIsDoctor(false);
    setIsAuthenticated(true);
  };

  const toggleRole = () => {
    setIsDoctor(!isDoctor);
  };

  if (!isAuthenticated) {
    return (
      <View style={styles.loadingContainer}>
        <ActivityIndicator size="large" color="#2196F3" />
        <Text>加载中...</Text>
      </View>
    );
  }

  return (
    <NavigationContainer>
      <Stack.Navigator>
        {isDoctor ? (
          // 医生端
          <Stack.Screen 
            name="DoctorHome" 
            options={{ headerShown: false }}
          >
            {() => <DoctorTabs />}
          </Stack.Screen>
        ) : (
          // 患者端
          <Stack.Screen 
            name="PatientHome" 
            options={{ headerShown: false }}
          >
            {() => <PatientTabs />}
          </Stack.Screen>
        )}

        {/* 公共屏幕 */}
        <Stack.Screen 
          name="DeviceScan" 
          component={DeviceScanScreen}
          options={{ title: '连接设备' }}
        />
        <Stack.Screen 
          name="HelpRequest" 
          component={HelpRequestScreen}
          options={{ title: '求助申请' }}
        />
        <Stack.Screen 
          name="PatientDetail" 
          component={PatientDetailScreen}
          options={{ title: '患者详情' }}
        />
      </Stack.Navigator>
    </NavigationContainer>
  );
};

/**
 * 占位屏幕（记录、个人资料）
 */
const RecordsScreen = ({ navigation }) => {
  return (
    <View style={styles.container}>
      <Text style={styles.title}>治疗记录</Text>
      <Text>功能开发中...</Text>
      <Button title="返回" onPress={() => navigation.goBack()} />
    </View>
  );
};

const ProfileScreen = ({ navigation }) => {
  const handleLogout = () => {
    // TODO: 清除登录状态
    Alert.alert('提示', '登出功能开发中');
  };

  return (
    <View style={styles.container}>
      <Text style={styles.title}>个人设置</Text>
      <Button title="切换角色（测试）" onPress={() => navigation.navigate('DoctorHome')} />
      <Button title="登出" onPress={handleLogout} color="#F44336" />
    </View>
  );
};

const ApprovalScreen = ({ navigation }) => {
  return (
    <View style={styles.container}>
      <Text style={styles.title}>参数调整审批</Text>
      <Text>功能开发中...</Text>
      <Button title="返回" onPress={() => navigation.goBack()} />
    </View>
  );
};

const DoctorProfileScreen = ({ navigation }) => {
  return (
    <View style={styles.container}>
      <Text style={styles.title}>医生设置</Text>
      <Button title="切换角色（测试）" onPress={() => navigation.navigate('PatientHome')} />
      <Button title="登出" onPress={() => {}} color="#F44336" />
    </View>
  );
};

const styles = StyleSheet.create({
  container: {
    flex: 1,
    justifyContent: 'center',
    alignItems: 'center',
  },
  title: {
    fontSize: 24,
    fontWeight: 'bold',
    marginBottom: 20,
  },
  loadingContainer: {
    flex: 1,
    justifyContent: 'center',
    alignItems: 'center',
  },
});

export default App;
