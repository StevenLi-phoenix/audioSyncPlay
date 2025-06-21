#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <atomic>
#include <thread>
#include <mutex>
#include <queue>
#include <chrono>

// Windows headers
#include <winsock2.h>
#include <ws2tcpip.h>

/**
 * @brief UDP network communication class for audio streaming
 *
 * This class provides UDP-based network communication for streaming audio
 * data between sender and receiver devices with timestamp support.
 */
class NetworkUDP
{
public:
    // Network packet header
    struct PacketHeader
    {
        uint32_t magic;       // Magic number for packet identification
        uint32_t sequence;    // Sequence number
        uint64_t timestamp;   // Timestamp in microseconds
        uint32_t payloadSize; // Size of audio data
        uint32_t checksum;    // Simple checksum for error detection
    };

    // Network statistics
    struct NetworkStats
    {
        uint32_t packetsSent = 0;
        uint32_t packetsReceived = 0;
        uint32_t packetsLost = 0;
        uint32_t packetsOutOfOrder = 0;
        float avgLatencyMs = 0.0f;
        float maxLatencyMs = 0.0f;
        float minLatencyMs = 0.0f;
        uint64_t bytesSent = 0;
        uint64_t bytesReceived = 0;
    };

    // Configuration
    struct NetworkConfig
    {
        std::string localIP = "0.0.0.0";
        std::string remoteIP = "127.0.0.1";
        uint16_t localPort = 0;
        uint16_t remotePort = 8080;
        int bufferSize = 65536;
        int timeoutMs = 1000;
        bool enableMulticast = false;
        std::string multicastGroup = "239.255.255.250";
    };

    NetworkUDP();
    ~NetworkUDP();

    /**
     * @brief Initialize UDP sender
     * @param dst_ip Destination IP address
     * @param port Destination port
     * @param config Network configuration
     * @return true if initialization successful
     */
    bool InitUDPSender(const std::string &dst_ip, uint16_t port,
                       const NetworkConfig &config = NetworkConfig());

    /**
     * @brief Initialize UDP receiver
     * @param listen_port Port to listen on
     * @param config Network configuration
     * @return true if initialization successful
     */
    bool InitUDPReceiver(uint16_t listen_port,
                         const NetworkConfig &config = NetworkConfig());

    /**
     * @brief Send audio frame
     * @param data Audio data buffer
     * @param size Size of audio data
     * @param timestamp Timestamp for the frame
     * @return true if sent successfully
     */
    bool SendFrame(const uint8_t *data, size_t size, uint64_t timestamp = 0);

    /**
     * @brief Receive audio frame
     * @param frame_out Output buffer for received frame
     * @param timestamp_out Output timestamp
     * @return true if received successfully
     */
    bool ReceiveFrame(std::vector<uint8_t> &frame_out, uint64_t &timestamp_out);

    /**
     * @brief Get network statistics
     * @return Current network statistics
     */
    NetworkStats GetStats() const;

    /**
     * @brief Reset network statistics
     */
    void ResetStats();

    /**
     * @brief Set network configuration
     * @param config Network configuration
     */
    void SetConfig(const NetworkConfig &config);

    /**
     * @brief Check if network is initialized
     * @return true if network is ready
     */
    bool IsInitialized() const;

    /**
     * @brief Close network connection
     */
    void Close();

private:
    // Private methods
    bool InitializeWinsock();
    bool CreateSocket();
    bool BindSocket();
    bool SetupMulticast();
    void CleanupWinsock();

    // Packet handling
    bool SendPacket(const PacketHeader &header, const uint8_t *payload);
    bool ReceivePacket(PacketHeader &header, std::vector<uint8_t> &payload);
    uint32_t CalculateChecksum(const uint8_t *data, size_t size);
    bool ValidatePacket(const PacketHeader &header, const uint8_t *payload);

    // Helper methods
    uint64_t GetCurrentTimestamp() const;
    std::string GetLocalIP() const;
    bool IsValidIP(const std::string &ip) const;

    // Member variables
    SOCKET socket_;
    sockaddr_in remoteAddr_;
    sockaddr_in localAddr_;

    NetworkConfig config_;
    NetworkStats stats_;

    // Threading for receiver
    std::atomic<bool> isReceiving_;
    std::thread receiveThread_;
    std::mutex receiveMutex_;
    std::queue<std::pair<std::vector<uint8_t>, uint64_t>> receiveQueue_;

    // State
    bool isInitialized_;
    bool isSender_;
    uint32_t sequenceNumber_;

    // Constants
    static constexpr uint32_t MAGIC_NUMBER = 0x53504F54; // "SPOT"
    static constexpr size_t MAX_PACKET_SIZE = 65507;     // UDP max payload
    static constexpr size_t MAX_QUEUE_SIZE = 100;        // Max queued packets
};
