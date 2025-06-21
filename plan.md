# 📅 Spotify 多设备同步音响 — 开发计划

## 🎯 项目目标

构建一个实时音频同步系统，实现多设备 Spotify 音乐的同步播放，延迟控制在 100ms 以内。

## 📊 开发周期规划

### 第 1 周：基础架构搭建
**目标**: 建立项目基础结构，实现基本的音频捕获功能

#### 任务清单
- [ ] 创建 CMake 项目结构
- [ ] 配置 VSCode 开发环境
- [ ] 实现 WASAPI 音频捕获模块
- [ ] 创建基本的 UDP 网络通信框架
- [ ] 编写单元测试框架

#### 技术要点
```cpp
// audio_capture.h 核心接口
class AudioCapture {
public:
    bool InitAudioCapture(const std::string& deviceName);
    bool StartCapture();
    void StopCapture();
    bool GetNextFrame(std::vector<uint8_t>& frame_out);
    void SetCallback(std::function<void(const std::vector<uint8_t>&)> callback);
private:
    // WASAPI 相关成员
};
```

#### 验收标准
- [ ] 能够捕获系统音频（WASAPI Loopback）
- [ ] 音频格式：PCM 16-bit, 44.1kHz
- [ ] 帧大小：1024 samples (约 23ms)
- [ ] 无音频丢失或卡顿

---

### 第 2 周：网络传输层
**目标**: 实现可靠的 UDP 音频流传输

#### 任务清单
- [ ] 完善 UDP 发送端实现
- [ ] 实现 UDP 接收端
- [ ] 添加数据包时间戳
- [ ] 实现简单的错误检测
- [ ] 网络性能测试

#### 技术要点
```cpp
// network_udp.h 核心接口
class NetworkUDP {
public:
    // 发送端
    bool InitUDPSender(const std::string& dst_ip, uint16_t port);
    bool SendFrame(const uint8_t* data, size_t size, uint64_t timestamp);
    
    // 接收端
    bool InitUDPReceiver(uint16_t listen_port);
    bool ReceiveFrame(std::vector<uint8_t>& frame_out, uint64_t& timestamp);
    
    // 统计信息
    struct NetworkStats {
        uint32_t packets_sent;
        uint32_t packets_received;
        uint32_t packets_lost;
        float avg_latency_ms;
    };
};
```

#### 验收标准
- [ ] 局域网内延迟 < 10ms
- [ ] 丢包率 < 1%
- [ ] 支持多接收端
- [ ] 网络统计信息准确

---

### 第 3 周：抖动缓冲器
**目标**: 实现智能音频缓冲，确保播放稳定性

#### 任务清单
- [ ] 设计抖动缓冲器架构
- [ ] 实现自适应缓冲大小
- [ ] 添加丢包补偿机制
- [ ] 实现音频帧重排序
- [ ] 缓冲器性能优化

#### 技术要点
```cpp
// jitter_buffer.h 核心接口
class JitterBuffer {
public:
    void PushFrame(const std::vector<uint8_t>& frame, uint64_t timestamp);
    bool GetFrame(std::vector<uint8_t>& frame_out);
    void SetTargetLatencyMs(int latency);
    void SetMaxLatencyMs(int max_latency);
    
    struct BufferStats {
        int current_latency_ms;
        int target_latency_ms;
        int buffer_occupancy_percent;
        uint32_t frames_dropped;
        uint32_t frames_reordered;
    };
    
    BufferStats GetStats() const;
private:
    // 环形缓冲区实现
    // 自适应算法
};
```

#### 验收标准
- [ ] 目标延迟可配置（50-200ms）
- [ ] 自适应缓冲大小调整
- [ ] 丢包时平滑过渡
- [ ] 缓冲器统计信息准确

---

### 第 4 周：音频播放模块
**目标**: 实现高质量的音频播放功能

#### 任务清单
- [ ] 实现 WASAPI 音频播放
- [ ] 音频格式转换
- [ ] 音量控制
- [ ] 播放状态管理
- [ ] 音频设备选择

#### 技术要点
```cpp
// audio_playback.h 核心接口
class AudioPlayback {
public:
    bool InitAudioPlayback(const std::string& deviceName);
    bool StartPlayback();
    bool PlayFrame(const std::vector<uint8_t>& frame);
    void StopPlayback();
    void SetVolume(float volume); // 0.0 - 1.0
    
    struct PlaybackStats {
        uint32_t frames_played;
        uint32_t frames_dropped;
        float current_volume;
        bool is_playing;
    };
    
    PlaybackStats GetStats() const;
};
```

#### 验收标准
- [ ] 音频播放无卡顿
- [ ] 支持音量调节
- [ ] 支持多音频设备
- [ ] 播放统计信息准确

---

### 第 5 周：同步优化与集成
**目标**: 优化同步精度，完善整体系统

#### 任务清单
- [ ] 实现时钟同步算法
- [ ] 优化延迟控制
- [ ] 添加配置管理
- [ ] 实现日志系统
- [ ] 性能测试与优化

#### 技术要点
```cpp
// 时钟同步
class ClockSync {
public:
    void SyncWithSender(uint64_t sender_timestamp);
    uint64_t GetAdjustedTimestamp();
    float GetDriftRate() const;
};

// 配置管理
struct AudioConfig {
    int sample_rate = 44100;
    int channels = 2;
    int bits_per_sample = 16;
    int frame_size = 1024;
    int target_latency_ms = 100;
    std::string network_interface = "auto";
};
```

#### 验收标准
- [ ] 多设备同步误差 < 50ms
- [ ] 配置文件支持
- [ ] 完整的日志记录
- [ ] 系统稳定性测试通过

---

## 🔧 技术栈选择

### 核心库
- **音频接口**: Windows WASAPI
- **网络通信**: Windows Socket API (Winsock2)
- **构建系统**: CMake 3.15+
- **编译器**: MSVC 或 MinGW-w64
- **IDE**: Visual Studio Code

### 可选扩展
- **音频编码**: Opus (压缩传输)
- **音频处理**: PortAudio (跨平台)
- **配置解析**: JSON (nlohmann/json)
- **日志系统**: spdlog

## 📈 性能指标

### 目标指标
| 指标 | 目标值 | 测量方法 |
|------|--------|----------|
| 端到端延迟 | < 100ms | 音频输入到输出时间差 |
| 同步精度 | < 50ms | 多设备间播放时间差 |
| CPU 使用率 | < 10% | 系统资源监控 |
| 内存使用 | < 50MB | 进程内存占用 |
| 网络带宽 | < 2Mbps | 网络流量监控 |

### 测试环境
- **网络**: 千兆局域网
- **设备**: Windows 10/11 PC
- **音频设备**: 标准音频接口
- **测试工具**: 音频分析软件

## 🐛 调试策略

### 开发阶段调试
1. **单元测试**: 每个模块独立测试
2. **集成测试**: 模块间接口测试
3. **性能测试**: 延迟和吞吐量测试
4. **压力测试**: 长时间运行稳定性

### 调试工具
- **VSCode 调试器**: 断点调试
- **网络抓包**: Wireshark
- **音频分析**: Audacity
- **性能分析**: Windows Performance Toolkit

## 📝 代码规范

### 命名规范
- **类名**: PascalCase (如 `AudioCapture`)
- **函数名**: PascalCase (如 `InitAudioCapture`)
- **变量名**: camelCase (如 `frameBuffer`)
- **常量**: UPPER_CASE (如 `MAX_FRAME_SIZE`)

### 代码风格
- 使用 C++17 标准
- 4 空格缩进
- 大括号换行风格
- 详细的注释和文档

## 🚀 部署计划

### 开发环境
- 本地开发：VSCode + CMake
- 版本控制：Git
- 持续集成：GitHub Actions (可选)

### 发布版本
- 可执行文件打包
- 配置文件模板
- 用户手册
- 安装脚本

## 📋 风险评估

### 技术风险
| 风险 | 概率 | 影响 | 缓解措施 |
|------|------|------|----------|
| WASAPI 兼容性 | 中 | 高 | 多设备测试，降级方案 |
| 网络延迟过高 | 中 | 中 | 本地网络优化，缓冲调整 |
| 音频质量损失 | 低 | 中 | 高质量音频格式，无损传输 |

### 项目风险
| 风险 | 概率 | 影响 | 缓解措施 |
|------|------|------|----------|
| 开发时间超期 | 中 | 中 | 优先级调整，功能简化 |
| 性能不达标 | 中 | 高 | 早期性能测试，优化迭代 |
| 用户接受度 | 低 | 中 | 用户反馈收集，功能调整 |

---

**更新时间**: 2024年12月
**版本**: v1.0
**负责人**: 开发团队 