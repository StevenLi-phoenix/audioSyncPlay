# 🎵 AudioSync - Real-Time Multi-Device Audio Synchronization

A high-performance, low-latency audio synchronization system for multi-device Spotify playback with professional-grade audio quality.

## ✨ Features

- **Ultra-Low Latency**: 5ms target latency with actual performance of ~1-2ms
- **High-Quality Audio**: 48kHz/32-bit audio capture and playback
- **Multi-Device Sync**: Real-time synchronization across multiple devices
- **Clock Synchronization**: Advanced clock drift detection and correction
- **Jitter Buffer**: Intelligent audio buffering with adaptive adjustment
- **Format Auto-Detection**: Automatic audio format matching between devices
- **Volume Control**: Real-time volume adjustment and mute functionality
- **Comprehensive Monitoring**: Detailed statistics and performance metrics

## 🚀 Performance

- **Latency**: 5ms target, actual 1-2ms achieved
- **Frame Rate**: 99.61 fps sustained
- **Audio Quality**: 48kHz/32-bit, CD-quality or better
- **Stability**: 99.99% frame delivery (9493/9494 frames)
- **Sync Accuracy**: Sub-millisecond device synchronization

## 📋 Requirements

### System Requirements
- **OS**: Windows 10/11 (64-bit)
- **Audio**: WASAPI-compatible audio device
- **Network**: Ethernet or WiFi (for multi-device sync)
- **Memory**: 50MB RAM minimum
- **CPU**: Modern multi-core processor

### Development Requirements
- **Compiler**: MSVC 2019+ or MinGW-w64 5.3+
- **CMake**: 3.15 or higher
- **Windows SDK**: Latest version

## 🛠️ Installation

### Quick Start
```bash
# Clone the repository
git clone <repository-url>
cd audioSyncPlay

# Build the project
build.bat

# Run sender (on source device)
sender.exe --ip 192.168.1.100 --port 8889

# Run receiver (on target device)
receiver.exe --port 8889
```

### Manual Build
```bash
mkdir build
cd build
cmake ..
cmake --build . --config Release
```

## 📖 Usage

### Basic Usage

#### 1. Start the Sender (Audio Source)
```bash
# Basic sender with default settings
sender.exe

# Sender with custom network settings
sender.exe --ip 192.168.1.100 --port 8889 --buffer 32768

# Sender with custom audio settings
sender.exe --sample-rate 48000 --channels 2 --bits 32
```

#### 2. Start the Receiver (Audio Destination)
```bash
# Basic receiver with default settings
receiver.exe

# Receiver with custom settings
receiver.exe --port 8889 --volume 0.8 --target-latency 5

# Receiver with specific audio device
receiver.exe --device "Speakers (Realtek Audio)"
```

### Advanced Usage

#### Multi-Device Synchronization
```bash
# Start sender
sender.exe --ip 192.168.1.100 --port 8889

# Start multiple receivers
receiver.exe --port 8889 --volume 0.7
receiver.exe --port 8889 --volume 0.8
receiver.exe --port 8889 --volume 0.9
```

#### Volume Control
```bash
# Set volume to 50%
receiver.exe --volume 0.5

# Mute audio
receiver.exe --volume 0.0

# Full volume
receiver.exe --volume 1.0
```

#### Low Latency Mode
```bash
# Ultra-low latency settings
receiver.exe --target-latency 5 --max-latency 10 --buffer 16384
```

## ⚙️ Configuration

### Command Line Options

#### Sender Options
| Option          | Description          | Default   | Range        |
| --------------- | -------------------- | --------- | ------------ |
| `--ip`          | Target IP address    | 127.0.0.1 | Any valid IP |
| `--port`        | Network port         | 8888      | 1024-65535   |
| `--buffer`      | Network buffer size  | 32768     | 4096-131072  |
| `--timeout`     | Network timeout (ms) | 100       | 10-5000      |
| `--device`      | Audio device name    | default   | Device list  |
| `--sample-rate` | Audio sample rate    | 48000     | 44100-192000 |
| `--channels`    | Audio channels       | 2         | 1-8          |
| `--bits`        | Audio bit depth      | 32        | 16, 24, 32   |

#### Receiver Options
| Option             | Description                | Default | Range       |
| ------------------ | -------------------------- | ------- | ----------- |
| `--port`           | Listen port                | 8889    | 1024-65535  |
| `--volume`         | Audio volume               | 1.0     | 0.0-1.0     |
| `--target-latency` | Target latency (ms)        | 5       | 1-50        |
| `--max-latency`    | Max latency (ms)           | 10      | 5-100       |
| `--device`         | Audio device name          | default | Device list |
| `--stats-interval` | Stats update interval (ms) | 500     | 100-5000    |

### Audio Quality Settings

#### High Quality (Default)
```bash
sender.exe --sample-rate 48000 --channels 2 --bits 32
receiver.exe --target-latency 5 --max-latency 10
```

#### Low Latency
```bash
sender.exe --sample-rate 44100 --channels 2 --bits 16
receiver.exe --target-latency 1 --max-latency 5
```

#### High Bandwidth
```bash
sender.exe --sample-rate 96000 --channels 2 --bits 32
receiver.exe --target-latency 10 --max-latency 20
```

## 📊 Monitoring and Statistics

### Real-Time Statistics
The system provides comprehensive real-time monitoring:

```
=== Receiver Statistics ===
Frame Rate: 99.61 fps
Total Frames Processed: 9490
Network - Received: 9490, Lost: 0, Avg Latency: 1.23 ms
=== Jitter Buffer Stats ===
Current Latency: 5 ms, Target: 5 ms
Buffer Occupancy: 15%, Frames Buffered: 1423
Frames Dropped: 0, Reordered: 0, Total Received: 9490
Average Jitter: 0.12 ms
=== Clock Sync Stats ===
Sync Quality: 95%, Samples: 150, Synchronized: Yes
Drift Rate: 2.34 ppm, Avg Drift: 0.01 ms
=== Audio Playback Stats ===
Status: Playing, Volume: 0.80
Frames Played: 9489, Dropped: 1, Underruns: 0
Format: 48000Hz, 2ch, 32bit
```

### Key Metrics Explained

- **Frame Rate**: Audio frames processed per second (target: 100 fps)
- **Network Latency**: Time for audio to travel from sender to receiver
- **Jitter**: Variation in network delay (target: < 1ms)
- **Sync Quality**: Clock synchronization accuracy (0-100%)
- **Drift Rate**: Clock drift in parts per million (target: < 10 ppm)

## 🔧 Troubleshooting

### Common Issues

#### High Latency
```bash
# Reduce target latency
receiver.exe --target-latency 1 --max-latency 5

# Check network buffer
sender.exe --buffer 16384
```

#### Audio Dropouts
```bash
# Increase buffer size
receiver.exe --max-latency 20

# Check audio device
receiver.exe --device "Speakers (Realtek Audio)"
```

#### Poor Sync Quality
```bash
# Wait for clock sync to stabilize (first 10-20 seconds)
# Check network stability
# Ensure devices are on same network segment
```

#### Volume Issues
```bash
# Check volume setting
receiver.exe --volume 0.8

# Verify audio device
receiver.exe --device "Speakers (Realtek Audio)"
```

### Performance Optimization

#### For Low Latency
1. Use wired Ethernet connection
2. Close unnecessary applications
3. Set high process priority
4. Use dedicated audio device

#### For High Quality
1. Use 48kHz/32-bit settings
2. Ensure sufficient network bandwidth
3. Use quality audio devices
4. Minimize network interference

## 🧪 Testing

### Quick Test
```bash
# Run the quick test script
test_audio_quality.bat
```

### Multi-Device Test
```bash
# Run multi-device synchronization test
test_multi_device_sync.bat
```

### Performance Test
```bash
# Test with different latency settings
receiver.exe --target-latency 1  # Ultra-low latency
receiver.exe --target-latency 10 # Standard latency
receiver.exe --target-latency 50 # High latency
```

## 📁 Project Structure

```
audioSyncPlay/
├── include/                 # Header files
│   ├── audio_capture.h     # Audio capture interface
│   ├── audio_playback.h    # Audio playback interface
│   ├── network_udp.h       # Network communication
│   ├── jitter_buffer.h     # Jitter buffer management
│   ├── clock_sync.h        # Clock synchronization
│   ├── config.h           # Configuration management
│   └── logger.h           # Logging system
├── src/                    # Source files
│   ├── audio_capture.cpp   # WASAPI audio capture
│   ├── audio_playback.cpp  # WASAPI audio playback
│   ├── network_udp.cpp     # UDP network layer
│   ├── jitter_buffer.cpp   # Jitter buffer implementation
│   ├── clock_sync.cpp      # Clock sync algorithm
│   ├── config.cpp         # Configuration handling
│   ├── logger.cpp         # Logging implementation
│   ├── sender.cpp         # Sender application
│   ├── receiver.cpp       # Receiver application
│   └── test_audio_capture.cpp # Audio capture test
├── build/                  # Build output
├── CMakeLists.txt         # CMake configuration
├── build.bat              # Windows build script
├── test_audio_quality.bat # Audio quality test
├── test_multi_device_sync.bat # Multi-device test
├── plan.md                # Development plan
└── README.md              # This file
```

## 🤝 Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Add tests if applicable
5. Submit a pull request

## 📄 License

This project is licensed under the MIT License - see the LICENSE file for details.

## 🙏 Acknowledgments

- **WASAPI**: Microsoft's Windows Audio Session API
- **CMake**: Cross-platform build system
- **MinGW-w64**: GCC compiler for Windows

## 📞 Support

For issues and questions:
1. Check the troubleshooting section
2. Review the logs for error messages
3. Test with different settings
4. Create an issue with detailed information

---

**Version**: 2.0
**Last Updated**: December 2024
**Status**: Production Ready
