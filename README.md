# 🎵 Spotify 多设备同步音响 — CMake + VSCode 工程

A real-time audio synchronization system for multi-device Spotify playback using C++ with CMake build system.

## 📋 项目概述

This project enables synchronized audio playback across multiple devices by capturing Spotify audio from a sender device and streaming it to receiver devices over UDP network protocol.

### ✨ 核心特性

- **实时音频捕获**: WASAPI Loopback 技术捕获系统音频
- **低延迟传输**: UDP 协议实现毫秒级延迟
- **抖动缓冲**: 智能缓冲算法确保播放稳定性
- **多设备同步**: 支持多个接收端设备同时播放
- **跨平台支持**: Windows 优先，可扩展至其他平台

## 🏗️ 项目结构

```
SpotifySyncAudio/
├── CMakeLists.txt              # CMake 构建配置
├── src/                        # 源代码目录
│   ├── sender.cpp              # 发送端主程序
│   ├── receiver.cpp            # 接收端主程序
│   ├── audio_capture.cpp       # 音频捕获模块
│   ├── audio_playback.cpp      # 音频播放模块
│   ├── network_udp.cpp         # UDP 网络通信
│   ├── jitter_buffer.cpp       # 抖动缓冲器
│   └── main.cpp                # 程序入口
├── include/                    # 头文件目录
│   ├── audio_capture.h
│   ├── audio_playback.h
│   ├── network_udp.h
│   └── jitter_buffer.h
├── third_party/                # 第三方库
│   ├── libopus/               # 音频编码库（可选）
│   └── portaudio/             # 音频接口库（可选）
├── build/                     # 构建输出目录
├── README.md                  # 项目说明文档
├── plan.md                    # 开发计划文档
├── LICENSE                    # 开源许可证
└── .vscode/                   # VSCode 配置
    ├── settings.json
    ├── launch.json
    └── tasks.json
```

## 🚀 快速开始

### 环境要求

- **操作系统**: Windows 10/11
- **编译器**: Visual Studio 2019+ 或 MinGW-w64
- **构建工具**: CMake 3.15+
- **IDE**: Visual Studio Code (推荐)
- **网络**: 局域网连接

### 安装依赖

1. **安装 CMake**
   ```bash
   # 从官网下载或使用包管理器
   # https://cmake.org/download/
   ```

2. **安装 VSCode 扩展**
   - C/C++ Extension Pack
   - CMake Tools
   - CMake

### 构建项目

```bash
# 克隆项目
git clone <repository-url>
cd SpotifySyncAudio

# 配置构建
cmake -S . -B build -G "Ninja"

# 编译项目
cmake --build build

# 或使用 VSCode CMake Tools
# Ctrl+Shift+P -> CMake: Configure
# Ctrl+Shift+P -> CMake: Build
```

### 运行程序

```bash
# 发送端（音频源设备）
./build/sender.exe

# 接收端（播放设备）
./build/receiver.exe
```

## 📖 使用说明

### 发送端配置

发送端负责捕获系统音频并发送到网络：

```cpp
// 示例配置
AudioCapture capture;
capture.InitAudioCapture("Default Audio Device");
capture.StartCapture();

NetworkUDP sender;
sender.InitUDPSender("192.168.1.100", 8080);

// 发送音频帧
std::vector<uint8_t> frame;
while (capture.GetNextFrame(frame)) {
    sender.SendFrame(frame.data(), frame.size());
}
```

### 接收端配置

接收端负责接收音频数据并播放：

```cpp
// 示例配置
NetworkUDP receiver;
receiver.InitUDPReceiver(8080);

JitterBuffer buffer;
buffer.SetTargetLatencyMs(100);

AudioPlayback playback;
playback.InitAudioPlayback("Default Audio Device");
playback.StartPlayback();

// 接收并播放音频帧
std::vector<uint8_t> frame;
while (receiver.ReceiveFrame(frame)) {
    buffer.PushFrame(frame, GetCurrentTimestamp());
    
    std::vector<uint8_t> playFrame;
    if (buffer.GetFrame(playFrame)) {
        playback.PlayFrame(playFrame);
    }
}
```

## 🔧 开发指南

### 核心模块

#### 1. 音频捕获 (audio_capture.h/cpp)
- **功能**: 使用 WASAPI Loopback 捕获系统音频
- **API**: `InitAudioCapture()`, `StartCapture()`, `GetNextFrame()`

#### 2. 网络传输 (network_udp.h/cpp)
- **功能**: UDP 数据包发送和接收
- **特性**: 时间戳封装，错误检测

#### 3. 抖动缓冲 (jitter_buffer.h/cpp)
- **功能**: 缓冲音频帧，控制播放延迟
- **算法**: 自适应缓冲大小，丢包补偿

#### 4. 音频播放 (audio_playback.h/cpp)
- **功能**: WASAPI 音频设备播放
- **API**: `InitAudioPlayback()`, `PlayFrame()`, `StopPlayback()`

### 调试配置

VSCode 调试配置已预设：
- **Launch Sender**: 调试发送端程序
- **Launch Receiver**: 调试接收端程序

### 性能优化

- **音频格式**: 16-bit PCM, 44.1kHz
- **缓冲区大小**: 可配置，默认 100ms
- **网络协议**: UDP 多播支持
- **编码压缩**: 可选的 Opus 编码

## 📊 技术规格

| 参数 | 规格 |
|------|------|
| 音频格式 | PCM 16-bit, 44.1kHz |
| 网络协议 | UDP |
| 延迟目标 | < 100ms |
| 支持设备数 | 理论无限制 |
| 网络带宽 | ~1.4 Mbps (未压缩) |

## 🤝 贡献指南

1. Fork 项目
2. 创建功能分支 (`git checkout -b feature/AmazingFeature`)
3. 提交更改 (`git commit -m 'Add some AmazingFeature'`)
4. 推送到分支 (`git push origin feature/AmazingFeature`)
5. 开启 Pull Request

## 📄 许可证

本项目采用 MIT 许可证 - 查看 [LICENSE](LICENSE) 文件了解详情。

## 🙏 致谢

- WASAPI 音频接口
- Windows Socket API
- CMake 构建系统
- Visual Studio Code

## 📞 联系方式

- 项目主页: [GitHub Repository]
- 问题反馈: [Issues]
- 讨论区: [Discussions]

---

**注意**: 本项目仅供学习和研究使用，请遵守相关法律法规和 Spotify 服务条款。 