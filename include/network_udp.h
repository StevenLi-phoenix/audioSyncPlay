#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <winsock2.h>
#include <ws2tcpip.h>

class NetworkUDP
{
public:
    NetworkUDP();
    ~NetworkUDP();

    // Sender interface
    bool InitUDPSender(const std::string &dst_ip, uint16_t port);
    bool SendFrame(const uint8_t *data, size_t size, uint64_t timestamp);
    void CloseSender();

    // Receiver interface
    bool InitUDPReceiver(uint16_t listen_port);
    bool ReceiveFrame(std::vector<uint8_t> &frame_out, uint64_t &timestamp);
    void CloseReceiver();

    // Statistics
    struct NetworkStats
    {
        uint32_t packets_sent;
        uint32_t packets_received;
        uint32_t packets_lost;
        float avg_latency_ms;
        uint32_t bytes_sent;
        uint32_t bytes_received;
    };
    NetworkStats GetStats() const;
    void ResetStats();

    // Configuration
    void SetBufferSize(int buffer_size);
    void SetTimeout(int timeout_ms);

private:
    // Sender members
    SOCKET m_senderSocket;
    sockaddr_in m_destAddr;
    bool m_senderInitialized;

    // Receiver members
    SOCKET m_receiverSocket;
    bool m_receiverInitialized;

    // Statistics
    mutable NetworkStats m_stats;

    // Configuration
    int m_bufferSize;
    int m_timeoutMs;

    // Helper methods
    bool InitializeWinsock();
    void CleanupWinsock();
    static bool s_winsockInitialized;
};
