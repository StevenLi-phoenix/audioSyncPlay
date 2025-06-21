# 🚀 Spotify Sync Audio - 快速开始指南

## 📋 项目概述

这是一个实时音频同步系统，可以将 Spotify 音频从一台设备流式传输到多台设备，实现同步播放。

## ⚡ 快速开始

### 1. 环境检查
```bash
# 运行环境检查脚本
test.bat
```

### 2. 开发环境设置
```bash
# 检查开发工具
setup.bat
```

### 3. 编译项目
```bash
# 自动编译项目
build.bat
```

## 🎯 使用方法

### 发送端（音频源设备）
```bash
# 基本用法
sender.exe --ip 192.168.1.100 --port 8080

# 指定音频设备
sender.exe --ip 192.168.1.100 --port 8080 --device "Speakers"

# 自定义采样率
sender.exe --ip 192.168.1.100 --port 8080 --format 48000
```

### 接收端（播放设备）
```bash
# 基本用法
receiver.exe --port 8080

# 指定延迟
receiver.exe --port 8080 --latency 150

# 调整音量
receiver.exe --port 8080 --volume 0.8

# 指定音频设备
receiver.exe --port 8080 --device "Headphones"
```

## 🔧 开发环境要求

### 必需工具
- **Visual Studio 2019+** 或 **Visual Studio Build Tools**
- **CMake 3.15+**
- **Windows 10/11**

### 推荐工具
- **Ninja** (更快的构建速度)
- **Visual Studio Code** (开发环境)
- **Git** (版本控制)

## 📁 项目结构

```
SpotifySyncAudio/
├── src/                    # 源代码
│   ├── main.cpp           # 程序入口
│   ├── sender.cpp         # 发送端实现
│   ├── receiver.cpp       # 接收端实现
│   ├── audio_capture.cpp  # 音频捕获
│   ├── network_udp.cpp    # 网络通信
│   ├── jitter_buffer.cpp  # 抖动缓冲
│   └── audio_playback.cpp # 音频播放
├── include/               # 头文件
│   ├── audio_capture.h
│   ├── network_udp.h
│   ├── jitter_buffer.h
│   └── audio_playback.h
├── .vscode/              # VSCode 配置
├── build/                # 构建输出
├── CMakeLists.txt        # CMake 配置
├── README.md             # 详细文档
├── plan.md               # 开发计划
├── setup.bat             # 环境检查
├── build.bat             # 构建脚本
└── test.bat              # 项目测试
```

## 🎵 技术特性

### 核心功能
- ✅ **实时音频捕获**: WASAPI Loopback 技术
- ✅ **低延迟传输**: UDP 协议，毫秒级延迟
- ✅ **智能缓冲**: 自适应抖动缓冲器
- ✅ **多设备支持**: 支持多个接收端
- ✅ **音量控制**: 实时音量调节
- ✅ **统计信息**: 详细的性能监控

### 性能指标
- **延迟**: < 100ms
- **同步精度**: < 50ms
- **音频格式**: PCM 16-bit, 44.1kHz
- **网络协议**: UDP
- **支持设备数**: 理论无限制

## 🐛 故障排除

### 常见问题

#### 1. 编译失败
```bash
# 检查环境
setup.bat

# 确保安装了 Visual Studio 和 CMake
```

#### 2. 网络连接问题
```bash
# 检查防火墙设置
# 确保端口 8080 未被占用
# 验证 IP 地址正确性
```

#### 3. 音频设备问题
```bash
# 列出可用设备
sender.exe --help
receiver.exe --help

# 使用默认设备
sender.exe --ip 127.0.0.1 --port 8080
```

## 📊 性能优化

### 网络优化
- 使用千兆局域网
- 减少网络拥塞
- 调整缓冲区大小

### 音频优化
- 选择合适的采样率
- 调整延迟设置
- 监控 CPU 使用率

## 🔄 开发流程

### 1. 获取代码
```bash
git clone <repository-url>
cd SpotifySyncAudio
```

### 2. 环境准备
```bash
# 安装开发工具
# 运行环境检查
setup.bat
```

### 3. 编译测试
```bash
# 编译项目
build.bat

# 运行测试
test.bat
```

### 4. 开发调试
```bash
# 使用 VSCode 打开项目
code .

# 设置断点调试
# 使用 VSCode 调试配置
```

## 📞 支持

### 文档
- [README.md](README.md) - 详细项目文档
- [plan.md](plan.md) - 开发计划和技术规格

### 工具
- `setup.bat` - 环境检查
- `build.bat` - 自动构建
- `test.bat` - 项目验证

### 联系方式
- 项目主页: [GitHub Repository]
- 问题反馈: [Issues]
- 讨论区: [Discussions]

---

**注意**: 本项目仅供学习和研究使用，请遵守相关法律法规和 Spotify 服务条款。
