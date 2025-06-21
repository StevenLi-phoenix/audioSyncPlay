# AudioSync Week 2 Testing Checklist

## Pre-Testing Setup

### ✅ Build Verification
- [ ] Run `cmake --build build --target sender`
- [ ] Run `cmake --build build --target receiver`
- [ ] Verify both executables are created in `build/` directory

### ✅ Basic Functionality Tests
- [ ] Test sender help: `build\sender.exe --help`
- [ ] Test receiver help: `build\receiver.exe --help`
- [ ] Verify help text displays correctly

## Network Communication Tests

### ✅ Local Loopback Test
- [ ] Start receiver: `build\receiver.exe --port 8889`
- [ ] Start sender: `build\sender.exe --ip 127.0.0.1 --port 8889`
- [ ] Verify receiver shows "Network receiver initialized successfully"
- [ ] Verify sender shows "Network sender initialized successfully"
- [ ] Play audio on sender machine
- [ ] Verify statistics show frames being sent/received
- [ ] Check latency is < 10ms (should be < 1ms locally)

### ✅ Statistics Verification
- [ ] Frame rate should be ~40-50 fps when audio is playing
- [ ] Packet loss should be 0% on local network
- [ ] Bytes sent/received should increase over time
- [ ] Average latency should be stable and low

### ✅ Error Handling Tests
- [ ] Try invalid IP address (should show error)
- [ ] Try invalid port number (should show error)
- [ ] Try using port that's already in use (should show error)
- [ ] Verify graceful shutdown with Ctrl+C

## Performance Tests

### ✅ Buffer Size Tests
- [ ] Test with small buffer (32768): `--buffer 32768`
- [ ] Test with large buffer (262144): `--buffer 262144`
- [ ] Verify no packet loss with different buffer sizes

### ✅ Timeout Tests
- [ ] Test with short timeout (100ms): `--timeout 100`
- [ ] Test with long timeout (5000ms): `--timeout 5000`
- [ ] Verify system remains stable

### ✅ Statistics Interval Tests
- [ ] Test with fast updates (500ms): `--stats-interval 500`
- [ ] Test with slow updates (5000ms): `--stats-interval 5000`
- [ ] Verify statistics update at correct intervals

## Audio Device Tests

### ✅ Audio Capture Test
- [ ] Run `build\test_audio_capture.exe`
- [ ] Verify audio devices are listed
- [ ] Verify audio capture starts successfully
- [ ] Check frame capture statistics

### ✅ Device Selection Test
- [ ] List available audio devices in sender output
- [ ] Try specifying a specific device: `--device "Device Name"`
- [ ] Verify correct device is selected

## Network Configuration Tests

### ✅ Port Configuration
- [ ] Test different receiver ports (8888, 9999, 12345)
- [ ] Verify sender connects to correct port
- [ ] Test port conflicts (should show error)

### ✅ IP Configuration
- [ ] Test localhost (127.0.0.1)
- [ ] Test local network IP (192.168.x.x)
- [ ] Test invalid IP addresses (should show error)

## Stress Tests

### ✅ Long Running Test
- [ ] Run sender and receiver for 5+ minutes
- [ ] Verify no memory leaks
- [ ] Check CPU usage remains stable
- [ ] Verify statistics remain accurate

### ✅ High Load Test
- [ ] Play high-bitrate audio
- [ ] Verify system handles increased load
- [ ] Check for any performance degradation

### ✅ Multiple Instances Test
- [ ] Try running multiple receivers (should work with UDP)
- [ ] Try running multiple senders (should work)
- [ ] Verify no conflicts

## Integration Tests

### ✅ End-to-End Test
- [ ] Complete audio capture → network → reception flow
- [ ] Verify audio data integrity
- [ ] Check timing accuracy
- [ ] Verify no data corruption

### ✅ Configuration File Test
- [ ] Create a configuration file
- [ ] Test loading configuration
- [ ] Verify command line overrides work

## Expected Results

### ✅ Success Indicators
- [ ] Latency < 10ms (local network)
- [ ] Packet loss < 1%
- [ ] Frame rate 40-50 fps
- [ ] CPU usage < 10%
- [ ] Memory usage < 50MB
- [ ] No crashes or hangs
- [ ] Graceful shutdown

### ✅ Error Indicators (Should NOT happen)
- [ ] "Failed to initialize audio capture"
- [ ] "Failed to initialize UDP sender/receiver"
- [ ] High packet loss (> 5%)
- [ ] Excessive latency (> 50ms)
- [ ] Memory leaks
- [ ] Application crashes

## Troubleshooting

### ❌ Common Issues and Solutions

**Issue: "Failed to initialize audio capture"**
- Solution: Check audio device permissions
- Solution: Try different audio device
- Solution: Restart audio service

**Issue: "Failed to initialize UDP sender/receiver"**
- Solution: Check if port is in use
- Solution: Run as administrator
- Solution: Check firewall settings

**Issue: High latency or packet loss**
- Solution: Check network quality
- Solution: Increase buffer size
- Solution: Reduce frame size

**Issue: Low frame rate**
- Solution: Check CPU usage
- Solution: Verify audio device performance
- Solution: Adjust statistics interval

## Test Scripts

### Quick Tests
```bash
# Run quick test
quick_test.bat

# Run advanced test suite
advanced_test.bat

# Run network test
test_network.bat
```

### Manual Tests
```bash
# Terminal 1
build\receiver.exe --port 8889

# Terminal 2
build\sender.exe --ip 127.0.0.1 --port 8889
```

## Success Criteria

✅ **Week 2 is considered successful if:**
- [ ] All basic functionality tests pass
- [ ] Network communication works reliably
- [ ] Performance meets targets (latency < 10ms, packet loss < 1%)
- [ ] Error handling works correctly
- [ ] System is stable under normal load
- [ ] Ready for Week 3 (Jitter Buffer) implementation

---

**Test Date:** ___________
**Tester:** ___________
**Results:** ___________
**Notes:** ___________
