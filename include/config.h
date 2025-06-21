#pragma once

#include <string>
#include <cstdint>

// Audio configuration
struct AudioConfig
{
    int sample_rate = 48000;
    int channels = 2;
    int bits_per_sample = 32;
    int frame_size = 1024;
    int target_latency_ms = 100;
    int max_latency_ms = 200;
    std::string network_interface = "auto";
    std::string default_audio_device = "";
    std::string device_name = "default";
    float volume = 1.0f;
    bool use_native_format = true;
    bool enable_format_conversion = true;
};

// Network configuration
struct NetworkConfig
{
    std::string sender_ip = "127.0.0.1";
    uint16_t sender_port = 8888;
    uint16_t receiver_port = 8889;
    int buffer_size = 65536;
    int timeout_ms = 1000;
    bool enable_multicast = false;
    std::string multicast_group = "239.255.255.250";
};

// Application configuration
struct AppConfig
{
    AudioConfig audio;
    NetworkConfig network;

    // Logging
    std::string log_level = "INFO";
    std::string log_file = "audiosync.log";
    bool enable_console_log = true;

    // Performance
    int thread_priority = 0; // 0=normal, 1=above_normal, 2=high
    bool enable_statistics = true;
    int stats_interval_ms = 1000;
};

// Global configuration instance
extern AppConfig g_config;

// Configuration management functions
bool LoadConfig(const std::string &filename);
bool SaveConfig(const std::string &filename);
void SetDefaultConfig();
