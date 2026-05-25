# TLSR8269 六路脉冲治疗仪 - Gerber文件生成指南

> **版本**：V3.0  
> **日期**：2026-05-14  
> **作者**：束长江

---

## 一、Gerber文件清单

### 1.1 必需文件

| 文件名 | 层 | 说明 | 格式 |
|--------|-----|------|------|
| `TLSR8269_PulseGenerator_v3-F_Cu.gbr` | F.Cu | 顶层铜箔 | RS-274X |
| `TLSR8269_PulseGenerator_v3-B_Cu.gbr` | B.Cu | 底层铜箔 | RS-274X |
| `TLSR8269_PulseGenerator_v3-F_Mask.gbr` | F.Mask | 顶层阻焊 | RS-274X |
| `TLSR8269_PulseGenerator_v3-B_Mask.gbr` | B.Mask | 底层阻焊 | RS-274X |
| `TLSR8269_PulseGenerator_v3-F_SilkS.gbr` | F.SilkS | 顶层丝印 | RS-274X |
| `TLSR8269_PulseGenerator_v3-B_SilkS.gbr` | B.SilkS | 底层丝印 | RS-274X |
| `TLSR8269_PulseGenerator_v3-Edge_Cuts.gbr` | Edge.Cuts | 板框 | RS-274X |

### 1.2 可选文件

| 文件名 | 层 | 说明 | 用途 |
|--------|-----|------|------|
| `TLSR8269_PulseGenerator_v3-F_Paste.gbr` | F.Paste | 顶层钢网 | SMT贴片 |
| `TLSR8269_PulseGenerator_v3-B_Paste.gbr` | B.Paste | 底层钢网 | SMT贴片 |
| `TLSR8269_PulseGenerator_v3-F_Fab.gbr` | F.Fab | 顶层制造图 | 生产参考 |
| `TLSR8269_PulseGenerator_v3-B_Fab.gbr` | B.Fab | 底层制造图 | 生产参考 |

### 1.3 钻孔文件

| 文件名 | 说明 | 格式 |
|--------|------|------|
| `TLSR8269_PulseGenerator_v3.drl` | 钻孔文件 | Excellon |
| `TLSR8269_PulseGenerator_v3-drl_map.pdf` | 钻孔图 | PDF |

---

## 二、生成参数

### 2.1 Gerber格式设置

```
格式：RS-274X
坐标精度：4.6 (Leading)
单位：毫米 (mm)
零省略：Leading
坐标模式：绝对坐标
```

### 2.2 层设置

```
包含层：
- F.Cu (顶层铜箔)
- B.Cu (底层铜箔)
- F.Mask (顶层阻焊)
- B.Mask (底层阻焊)
- F.SilkS (顶层丝印)
- B.SilkS (底层丝印)
- Edge.Cuts (板框)

不包含层：
- F.Adhes (顶层胶)
- B.Adhes (底层胶)
- F.CrtYd (顶层 courtyard)
- B.CrtYd (底层 courtyard)
- F.Fab (顶层制造)
- B.Fab (底层制造)
- Margin (边距)
```

---

## 三、制造要求

### 3.1 基本参数

| 参数 | 值 | 说明 |
|------|-----|------|
| 板尺寸 | 100mm × 80mm | 最大尺寸 |
| 板厚 | 1.6mm | 标准厚度 |
| 层数 | 2层 | 双层板 |
| 铜厚 | 1oz (35μm) | 标准铜厚 |
| 最小线宽 | 0.2mm | 设计规则 |
| 最小间距 | 0.2mm | 设计规则 |
| 最小孔径 | 0.3mm | 过孔 |
| 阻焊颜色 | 绿色 | 标准 |
| 丝印颜色 | 白色 | 标准 |
| 表面处理 | HASL无铅喷锡 | 或无铅沉金 |

### 3.2 特殊要求

```
1. 隔离带区域：
   - 禁止铺铜
   - 禁止走线
   - 爬电距离 ≥ 3mm

2. 光耦跨接：
   - 两侧网络分离
   - 隔离距离 ≥ 3mm

3. 安装孔：
   - 4个M2.2安装孔
   - 金属化孔
   - 连接GND

4. 测试点：
   - 保留测试点焊盘
   - 方便调试
```

---

## 四、提交华秋/嘉立创

### 4.1 文件打包

```bash
# 创建压缩包
cd hardware/gerber
zip TLSR8269_PulseGenerator_v3_Gerber.zip \
  TLSR8269_PulseGenerator_v3-F_Cu.gbr \
  TLSR8269_PulseGenerator_v3-B_Cu.gbr \
  TLSR8269_PulseGenerator_v3-F_Mask.gbr \
  TLSR8269_PulseGenerator_v3-B_Mask.gbr \
  TLSR8269_PulseGenerator_v3-F_SilkS.gbr \
  TLSR8269_PulseGenerator_v3-B_SilkS.gbr \
  TLSR8269_PulseGenerator_v3-Edge_Cuts.gbr \
  TLSR8269_PulseGenerator_v3.drl
```

### 4.2 在线下单参数

**华秋DFM** (https://www.huaqiu.com/)

```
板子尺寸：100mm × 80mm
板子数量：5
板子层数：2层
板子厚度：1.6mm
铜厚：1oz
阻焊颜色：绿色
丝印颜色：白色
表面处理：HASL无铅喷锡
最小孔径：0.3mm
最小线宽：0.2mm
最小间距：0.2mm
```

**嘉立创** (https://www.jlc.com/)

```
板子尺寸：100mm × 80mm
板子数量：5
板子层数：2层
板子厚度：1.6mm
铜厚：1oz
阻焊颜色：绿色
丝印颜色：白色
表面处理：HASL无铅喷锡
最小孔径：0.3mm
最小线宽：0.2mm
最小间距：0.2mm
```

---

## 五、BOM清单

### 5.1 文件位置

```
hardware/BOM.csv
```

### 5.2 物料汇总

| 类别 | 数量 | 说明 |
|------|------|------|
| 集成电路 | 9 | MCU、稳压器、光耦等 |
| 无源元件 | 28 | 电阻、电容、晶振 |
| 连接器 | 4 | 排针、插座 |
| 开关 | 2 | 按钮 |
| LED | 1 | 指示灯 |
| **总计** | **44** | 不含PCB和电极 |

### 5.3 成本估算

| 项目 | 单价(元) | 数量 | 小计(元) |
|------|----------|------|----------|
| 主控芯片 | 1.50 | 1 | 1.50 |
| 稳压器 | 0.35 | 1 | 0.35 |
| 电流检测 | 2.50 | 1 | 2.50 |
| 光耦 | 0.25 | 6 | 1.50 |
| 晶振 | 0.30 | 1 | 0.30 |
| 电位器 | 2.00 | 1 | 2.00 |
| 电阻电容 | 0.01-0.15 | 28 | 1.50 |
| 连接器 | 0.05-0.15 | 4 | 0.35 |
| 其他 | - | - | 0.50 |
| **元件总计** | | | **10.50** |
| PCB (5片) | 15.00 | 1批 | 15.00 |
| **总计** | | | **25.50** |

---

## 六、贴片坐标文件

### 6.1 文件位置

```
hardware/PickPlace.csv
```

### 6.2 格式说明

```csv
位号,元件型号,封装,X(mm),Y(mm),旋转角度,层,注释
```

### 6.3 使用说明

1. **SMT贴片**：导入贴片机
2. **手工焊接**：参考坐标定位
3. **检验**：核对元件位置

---

## 七、验证清单

### 7.1 文件完整性

- [ ] Gerber文件 (7个必需 + 4个可选)
- [ ] 钻孔文件 (.drl)
- [ ] BOM清单 (.csv)
- [ ] 贴片坐标 (.csv)
- [ ] 制造说明 (.md)

### 7.2 设计规则检查

- [ ] DRC无错误
- [ ] 无未连接网络
- [ ] 无短路
- [ ] 隔离带正确
- [ ] 安装孔位置正确

### 7.3 制造可行性

- [ ] 线宽 ≥ 0.2mm
- [ ] 间距 ≥ 0.2mm
- [ ] 孔径 ≥ 0.3mm
- [ ] 板厚 1.6mm
- [ ] 铜厚 1oz

---

## 八、注意事项

### 8.1 安全警告

⚠️ **本设计为开源学习项目，未经医疗认证**  
⚠️ **使用前请咨询专业医生**  
⚠️ **禁止用于商业用途**

### 8.2 制造建议

1. 选择有资质的PCB厂
2. 要求检测隔离距离
3. 进行电气安全测试
4. 保留测试记录

### 8.3 质量控制

1. 首件检验
2. 电气测试
3. 功能测试
4. 安全测试

---

## 九、版本历史

| 版本 | 日期 | 修改内容 |
|------|------|----------|
| V1.0 | 2026-05-10 | 初始版本 |
| V2.0 | 2026-05-12 | 增加CR8269模块 |
| V3.0 | 2026-05-14 | 增加5档电位器、完整隔离、Gerber文件 |

---

*本设计基于Telink TLSR8269官方资料，请以最新版Datasheet为准。*
*医疗级设计需通过专业机构认证，本设计仅供参考学习。*
