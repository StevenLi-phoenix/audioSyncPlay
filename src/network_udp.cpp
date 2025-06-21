#include "network_udp.h"
#include "logger.h"
#include <iostream>
#include <chrono>

bool NetworkUDP::s_winsockInitialized = false;

NetworkUDP::NetworkUDP()
    : m_senderSocket(INVALID_SOCKET), m_receiverSocket(INVALID_SOCKET), m_senderInitialized(false), m_receiverInitialized(false), m_bufferSize(65536), m_timeoutMs(1000)
{
    memset(&m_destAddr, 0, sizeof(m_destAddr));
    memset(&m_stats, 0, sizeof(m_stats));
}

NetworkUDP::~NetworkUDP()
{
    CloseSender();
    CloseReceiver();
    CleanupWinsock();
}

bool NetworkUDP::InitializeWinsock()
{
    if (s_winsockInitialized)
    {
        return true;
    }

    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0)
    {
        LOG_ERROR_FMT("WSAStartup failed: %d", result);
        return false;
    }

    s_winsockInitialized = true;
    LOG_INFO("Winsock initialized successfully");
    return true;
}

void NetworkUDP::CleanupWinsock()
{
    if (s_winsockInitialized)
    {
        WSACleanup();
        s_winsockInitialized = false;
        LOG_INFO("Winsock cleaned up");
    }
}

bool NetworkUDP::InitUDPSender(const std::string &dst_ip, uint16_t port)
{
    if (m_senderInitialized)
    {
        LOG_WARNING("UDP sender already initialized");
        return true;
    }

    if (!InitializeWinsock())
    {
        return false;
    }

    LOG_INFO_FMT("Initializing UDP sender to %s:%d", dst_ip.c_str(), port);

    // Create socket
    m_senderSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (m_senderSocket == INVALID_SOCKET)
    {
        LOG_ERROR_FMT("Failed to create sender socket: %d", WSAGetLastError());
        return false;
    }

    // Set socket options
    int optval = m_bufferSize;
    if (setsockopt(m_senderSocket, SOL_SOCKET, SO_SNDBUF, (char *)&optval, sizeof(optval)) == SOCKET_ERROR)
    {
        LOG_WARNING_FMT("Failed to set send buffer size: %d", WSAGetLastError());
    }

    // Set timeout
    DWORD timeout = m_timeoutMs;
    if (setsockopt(m_senderSocket, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout, sizeof(timeout)) == SOCKET_ERROR)
    {
        LOG_WARNING_FMT("Failed to set send timeout: %d", WSAGetLastError());
    }

    // Setup destination address
    m_destAddr.sin_family = AF_INET;
    m_destAddr.sin_port = htons(port);
    m_destAddr.sin_addr.s_addr = inet_addr(dst_ip.c_str());

    if (m_destAddr.sin_addr.s_addr == INADDR_NONE)
    {
        LOG_ERROR_FMT("Invalid destination IP address: %s", dst_ip.c_str());
        closesocket(m_senderSocket);
        m_senderSocket = INVALID_SOCKET;
        return false;
    }

    m_senderInitialized = true;
    LOG_INFO("UDP sender initialized successfully");
    return true;
}

bool NetworkUDP::SendFrame(const uint8_t *data, size_t size, uint64_t timestamp)
{
    if (!m_senderInitialized)
    {
        LOG_ERROR("UDP sender not initialized");
        return false;
    }

    // Create packet with timestamp
    struct PacketHeader
    {
        uint64_t timestamp;
        uint32_t data_size;
        uint32_t sequence_number;
    };

    static uint32_t sequence_number = 0;
    PacketHeader header;
    header.timestamp = timestamp;
    header.data_size = static_cast<uint32_t>(size);
    header.sequence_number = ++sequence_number;

    // Combine header and data
    std::vector<uint8_t> packet;
    packet.reserve(sizeof(header) + size);
    packet.insert(packet.end(), (uint8_t *)&header, (uint8_t *)&header + sizeof(header));
    packet.insert(packet.end(), data, data + size);

    // Send packet
    int result = sendto(m_senderSocket, (char *)packet.data(), static_cast<int>(packet.size()),
                        0, (sockaddr *)&m_destAddr, sizeof(m_destAddr));

    if (result == SOCKET_ERROR)
    {
        LOG_ERROR_FMT("Failed to send packet: %d", WSAGetLastError());
        return false;
    }

    // Update statistics
    m_stats.packets_sent++;
    m_stats.bytes_sent += static_cast<uint32_t>(packet.size());

    return true;
}

void NetworkUDP::CloseSender()
{
    if (m_senderSocket != INVALID_SOCKET)
    {
        closesocket(m_senderSocket);
        m_senderSocket = INVALID_SOCKET;
        m_senderInitialized = false;
        LOG_INFO("UDP sender closed");
    }
}

bool NetworkUDP::InitUDPReceiver(uint16_t listen_port)
{
    if (m_receiverInitialized)
    {
        LOG_WARNING("UDP receiver already initialized");
        return true;
    }

    if (!InitializeWinsock())
    {
        return false;
    }

    LOG_INFO_FMT("Initializing UDP receiver on port %d", listen_port);

    // Create socket
    m_receiverSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (m_receiverSocket == INVALID_SOCKET)
    {
        LOG_ERROR_FMT("Failed to create receiver socket: %d", WSAGetLastError());
        return false;
    }

    // Set socket options
    int optval = m_bufferSize;
    if (setsockopt(m_receiverSocket, SOL_SOCKET, SO_RCVBUF, (char *)&optval, sizeof(optval)) == SOCKET_ERROR)
    {
        LOG_WARNING_FMT("Failed to set receive buffer size: %d", WSAGetLastError());
    }

    // Set timeout
    DWORD timeout = m_timeoutMs;
    if (setsockopt(m_receiverSocket, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout)) == SOCKET_ERROR)
    {
        LOG_WARNING_FMT("Failed to set receive timeout: %d", WSAGetLastError());
    }

    // Bind socket
    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(listen_port);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(m_receiverSocket, (sockaddr *)&addr, sizeof(addr)) == SOCKET_ERROR)
    {
        LOG_ERROR_FMT("Failed to bind receiver socket: %d", WSAGetLastError());
        closesocket(m_receiverSocket);
        m_receiverSocket = INVALID_SOCKET;
        return false;
    }

    m_receiverInitialized = true;
    LOG_INFO("UDP receiver initialized successfully");
    return true;
}

bool NetworkUDP::ReceiveFrame(std::vector<uint8_t> &frame_out, uint64_t &timestamp)
{
    if (!m_receiverInitialized)
    {
        LOG_ERROR("UDP receiver not initialized");
        return false;
    }

    // Receive packet
    std::vector<uint8_t> packet(m_bufferSize);
    sockaddr_in senderAddr;
    int senderAddrLen = sizeof(senderAddr);

    int result = recvfrom(m_receiverSocket, (char *)packet.data(), static_cast<int>(packet.size()),
                          0, (sockaddr *)&senderAddr, &senderAddrLen);

    if (result == SOCKET_ERROR)
    {
        int error = WSAGetLastError();
        if (error != WSAETIMEDOUT)
        {
            LOG_ERROR_FMT("Failed to receive packet: %d", error);
        }
        return false;
    }

    if (result < static_cast<int>(sizeof(uint64_t) + sizeof(uint32_t) * 2))
    {
        LOG_WARNING("Received packet too small");
        return false;
    }

    // Parse packet header
    struct PacketHeader
    {
        uint64_t timestamp;
        uint32_t data_size;
        uint32_t sequence_number;
    };

    PacketHeader *header = (PacketHeader *)packet.data();
    size_t headerSize = sizeof(PacketHeader);
    size_t dataSize = header->data_size;

    if (result < static_cast<int>(headerSize + dataSize))
    {
        LOG_WARNING("Packet size mismatch");
        return false;
    }

    // Extract frame data
    frame_out.assign(packet.begin() + headerSize, packet.begin() + headerSize + dataSize);
    timestamp = header->timestamp;

    // Update statistics
    m_stats.packets_received++;
    m_stats.bytes_received += static_cast<uint32_t>(result);

    // Calculate latency (simplified)
    auto now = std::chrono::steady_clock::now();
    auto packet_time = std::chrono::steady_clock::time_point(std::chrono::nanoseconds(timestamp));
    auto latency = std::chrono::duration_cast<std::chrono::microseconds>(now - packet_time);
    m_stats.avg_latency_ms = (m_stats.avg_latency_ms * (m_stats.packets_received - 1) + latency.count() / 1000.0f) / m_stats.packets_received;

    return true;
}

void NetworkUDP::CloseReceiver()
{
    if (m_receiverSocket != INVALID_SOCKET)
    {
        closesocket(m_receiverSocket);
        m_receiverSocket = INVALID_SOCKET;
        m_receiverInitialized = false;
        LOG_INFO("UDP receiver closed");
    }
}

NetworkUDP::NetworkStats NetworkUDP::GetStats() const
{
    return m_stats;
}

void NetworkUDP::ResetStats()
{
    memset(&m_stats, 0, sizeof(m_stats));
    LOG_INFO("Network statistics reset");
}

void NetworkUDP::SetBufferSize(int buffer_size)
{
    m_bufferSize = buffer_size;
    LOG_INFO_FMT("Buffer size set to %d bytes", buffer_size);
}

void NetworkUDP::SetTimeout(int timeout_ms)
{
    m_timeoutMs = timeout_ms;
    LOG_INFO_FMT("Timeout set to %d ms", timeout_ms);
}
