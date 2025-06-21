#include "config.h"
#include "logger.h"
#include <fstream>
#include <iostream>

// Global configuration instance
AppConfig g_config;

// Set default configuration values
void SetDefaultConfig()
{
    // Audio configuration defaults
    g_config.audio.sample_rate = 44100;
    g_config.audio.channels = 2;
    g_config.audio.bits_per_sample = 16;
    g_config.audio.frame_size = 1024;
    g_config.audio.target_latency_ms = 100;
    g_config.audio.max_latency_ms = 200;
    g_config.audio.network_interface = "auto";
    g_config.audio.default_audio_device = "";

    // Network configuration defaults
    g_config.network.sender_ip = "127.0.0.1";
    g_config.network.sender_port = 8888;
    g_config.network.receiver_port = 8889;
    g_config.network.buffer_size = 65536;
    g_config.network.timeout_ms = 1000;
    g_config.network.enable_multicast = false;
    g_config.network.multicast_group = "239.255.255.250";

    // Application configuration defaults
    g_config.log_level = "INFO";
    g_config.log_file = "audiosync.log";
    g_config.enable_console_log = true;
    g_config.thread_priority = 0; // normal
    g_config.enable_statistics = true;
    g_config.stats_interval_ms = 1000;
}

// Load configuration from file
bool LoadConfig(const std::string &filename)
{
    std::ifstream file(filename);
    if (!file.is_open())
    {
        LOG_WARNING_FMT("Could not open config file: {}, using defaults", filename);
        SetDefaultConfig();
        return false;
    }

    LOG_INFO_FMT("Loading configuration from: {}", filename);

    // Simple INI-style parser
    std::string line;
    std::string currentSection;

    while (std::getline(file, line))
    {
        // Skip empty lines and comments
        if (line.empty() || line[0] == '#' || line[0] == ';')
            continue;

        // Remove leading/trailing whitespace
        line.erase(0, line.find_first_not_of(" \t"));
        line.erase(line.find_last_not_of(" \t") + 1);

        // Check for section headers
        if (line[0] == '[' && line[line.length() - 1] == ']')
        {
            currentSection = line.substr(1, line.length() - 2);
            continue;
        }

        // Parse key-value pairs
        size_t pos = line.find('=');
        if (pos != std::string::npos)
        {
            std::string key = line.substr(0, pos);
            std::string value = line.substr(pos + 1);

            // Remove whitespace from key and value
            key.erase(0, key.find_first_not_of(" \t"));
            key.erase(key.find_last_not_of(" \t") + 1);
            value.erase(0, value.find_first_not_of(" \t"));
            value.erase(value.find_last_not_of(" \t") + 1);

            // Apply configuration based on section and key
            if (currentSection == "audio")
            {
                if (key == "sample_rate")
                    g_config.audio.sample_rate = std::stoi(value);
                else if (key == "channels")
                    g_config.audio.channels = std::stoi(value);
                else if (key == "bits_per_sample")
                    g_config.audio.bits_per_sample = std::stoi(value);
                else if (key == "frame_size")
                    g_config.audio.frame_size = std::stoi(value);
                else if (key == "target_latency_ms")
                    g_config.audio.target_latency_ms = std::stoi(value);
                else if (key == "max_latency_ms")
                    g_config.audio.max_latency_ms = std::stoi(value);
                else if (key == "default_audio_device")
                    g_config.audio.default_audio_device = value;
            }
            else if (currentSection == "network")
            {
                if (key == "sender_ip")
                    g_config.network.sender_ip = value;
                else if (key == "sender_port")
                    g_config.network.sender_port = static_cast<uint16_t>(std::stoi(value));
                else if (key == "receiver_port")
                    g_config.network.receiver_port = static_cast<uint16_t>(std::stoi(value));
                else if (key == "buffer_size")
                    g_config.network.buffer_size = std::stoi(value);
                else if (key == "timeout_ms")
                    g_config.network.timeout_ms = std::stoi(value);
                else if (key == "enable_multicast")
                    g_config.network.enable_multicast = (value == "true" || value == "1");
                else if (key == "multicast_group")
                    g_config.network.multicast_group = value;
            }
            else if (currentSection == "app")
            {
                if (key == "log_level")
                    g_config.log_level = value;
                else if (key == "log_file")
                    g_config.log_file = value;
                else if (key == "enable_console_log")
                    g_config.enable_console_log = (value == "true" || value == "1");
                else if (key == "thread_priority")
                    g_config.thread_priority = std::stoi(value);
                else if (key == "enable_statistics")
                    g_config.enable_statistics = (value == "true" || value == "1");
                else if (key == "stats_interval_ms")
                    g_config.stats_interval_ms = std::stoi(value);
            }
        }
    }

    file.close();
    LOG_INFO("Configuration loaded successfully");
    return true;
}

// Save configuration to file
bool SaveConfig(const std::string &filename)
{
    std::ofstream file(filename);
    if (!file.is_open())
    {
        LOG_ERROR_FMT("Could not create config file: {}", filename);
        return false;
    }

    LOG_INFO_FMT("Saving configuration to: {}", filename);

    // Write audio configuration
    file << "[audio]\n";
    file << "sample_rate = " << g_config.audio.sample_rate << "\n";
    file << "channels = " << g_config.audio.channels << "\n";
    file << "bits_per_sample = " << g_config.audio.bits_per_sample << "\n";
    file << "frame_size = " << g_config.audio.frame_size << "\n";
    file << "target_latency_ms = " << g_config.audio.target_latency_ms << "\n";
    file << "max_latency_ms = " << g_config.audio.max_latency_ms << "\n";
    file << "default_audio_device = " << g_config.audio.default_audio_device << "\n";
    file << "\n";

    // Write network configuration
    file << "[network]\n";
    file << "sender_ip = " << g_config.network.sender_ip << "\n";
    file << "sender_port = " << g_config.network.sender_port << "\n";
    file << "receiver_port = " << g_config.network.receiver_port << "\n";
    file << "buffer_size = " << g_config.network.buffer_size << "\n";
    file << "timeout_ms = " << g_config.network.timeout_ms << "\n";
    file << "enable_multicast = " << (g_config.network.enable_multicast ? "true" : "false") << "\n";
    file << "multicast_group = " << g_config.network.multicast_group << "\n";
    file << "\n";

    // Write application configuration
    file << "[app]\n";
    file << "log_level = " << g_config.log_level << "\n";
    file << "log_file = " << g_config.log_file << "\n";
    file << "enable_console_log = " << (g_config.enable_console_log ? "true" : "false") << "\n";
    file << "thread_priority = " << g_config.thread_priority << "\n";
    file << "enable_statistics = " << (g_config.enable_statistics ? "true" : "false") << "\n";
    file << "stats_interval_ms = " << g_config.stats_interval_ms << "\n";

    file.close();
    LOG_INFO("Configuration saved successfully");
    return true;
}

// Print current configuration
void PrintConfig()
{
    LOG_INFO("=== Current Configuration ===");
    LOG_INFO("Audio:");
    LOG_INFO_FMT("  Sample Rate: {} Hz", g_config.audio.sample_rate);
    LOG_INFO_FMT("  Channels: {}", g_config.audio.channels);
    LOG_INFO_FMT("  Bits per Sample: {}", g_config.audio.bits_per_sample);
    LOG_INFO_FMT("  Frame Size: {} samples", g_config.audio.frame_size);
    LOG_INFO_FMT("  Target Latency: {} ms", g_config.audio.target_latency_ms);
    LOG_INFO_FMT("  Max Latency: {} ms", g_config.audio.max_latency_ms);
    LOG_INFO_FMT("  Default Device: {}", g_config.audio.default_audio_device);

    LOG_INFO("Network:");
    LOG_INFO_FMT("  Sender IP: {}", g_config.network.sender_ip);
    LOG_INFO_FMT("  Sender Port: {}", g_config.network.sender_port);
    LOG_INFO_FMT("  Receiver Port: {}", g_config.network.receiver_port);
    LOG_INFO_FMT("  Buffer Size: {}", g_config.network.buffer_size);
    LOG_INFO_FMT("  Timeout: {} ms", g_config.network.timeout_ms);
    LOG_INFO_FMT("  Multicast: {}", g_config.network.enable_multicast ? "enabled" : "disabled");
    LOG_INFO_FMT("  Multicast Group: {}", g_config.network.multicast_group);

    LOG_INFO("Application:");
    LOG_INFO_FMT("  Log Level: {}", g_config.log_level);
    LOG_INFO_FMT("  Log File: {}", g_config.log_file);
    LOG_INFO_FMT("  Console Log: {}", g_config.enable_console_log ? "enabled" : "disabled");
    LOG_INFO_FMT("  Thread Priority: {}", g_config.thread_priority);
    LOG_INFO_FMT("  Statistics: {}", g_config.enable_statistics ? "enabled" : "disabled");
    LOG_INFO_FMT("  Stats Interval: {} ms", g_config.stats_interval_ms);
    LOG_INFO("=============================");
}
