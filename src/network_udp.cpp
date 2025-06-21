#include "network_udp.h"
#include <iostream>
#include <algorithm>
#include <cstring>

NetworkUDP::NetworkUDP()
    : socket_(INVALID_SOCKET), isInitialized_(false), isSender_(false), sequenceNumber_(0)
{
}

NetworkUDP::~NetworkUDP()
{
    Close();
}

bool NetworkUDP::InitUDPSender(const std::string &dst_ip, uint16_t port, const NetworkConfig &config)
{
    if (isInitialized_)
    {
        std::cerr << "Network already initialized\n";
        return false;
    }

    config_ = config;
    config_.remoteIP = dst_ip;
    config_.remotePort = port;

    if (!InitializeWinsock())
    {
        std::cerr << "Failed to initialize Winsock\n";
        return false;
    }

    if (!CreateSocket())
    {
        std::cerr << "Failed to create socket\n";
        return false;
    }

    // Set up remote address
    remoteAddr_.sin_family = AF_INET;
    remoteAddr_.sin_port = htons(port);
    remoteAddr_.sin_addr.s_addr = inet_addr(dst_ip.c_str());

    isSender_ = true;
    isInitialized_ = true;

    std::cout << "UDP sender initialized: " << dst_ip << ":" << port << std::endl;
    return true;
}

bool NetworkUDP::InitUDPReceiver(uint16_t listen_port, const NetworkConfig &config)
{
    if (isInitialized_)
    {
        std::cerr << "Network already initialized\n";
        return false;
    }

    config_ = config;
    config_.localPort = listen_port;

    if (!InitializeWinsock())
    {
        std::cerr << "Failed to initialize Winsock\n";
        return false;
    }

    if (!CreateSocket())
    {
        std::cerr << "Failed to create socket\n";
        return false;
    }

    if (!BindSocket())
    {
        std::cerr << "Failed to bind socket\n";
        return false;
    }

    // Start receive thread
    isReceiving_ = true;
    receiveThread_ = std::thread(&NetworkUDP::ReceiveThread, this);

    isSender_ = false;
    isInitialized_ = true;

    std::cout << "UDP receiver initialized on port " << listen_port << std::endl;
    return true;
}

bool NetworkUDP::SendFrame(const uint8_t *data, size_t size, uint64_t timestamp)
{
    if (!isInitialized_ || !isSender_)
    {
        return false;
    }

    PacketHeader header;
    header.magic = MAGIC_NUMBER;
    header.sequence = sequenceNumber_++;
    header.timestamp = timestamp;
    header.payloadSize = static_cast<uint32_t>(size);
    header.checksum = CalculateChecksum(data, size);

    return SendPacket(header, data);
}

bool NetworkUDP::ReceiveFrame(std::vector<uint8_t> &frame_out, uint64_t &timestamp_out)
{
    if (!isInitialized_ || isSender_)
    {
        return false;
    }

    std::lock_guard<std::mutex> lock(receiveMutex_);

    if (receiveQueue_.empty())
    {
        return false;
    }

    frame_out = receiveQueue_.front().first;
    timestamp_out = receiveQueue_.front().second;
    receiveQueue_.pop();

    return true;
}

NetworkUDP::NetworkStats NetworkUDP::GetStats() const
{
    return stats_;
}

void NetworkUDP::ResetStats()
{
    stats_ = NetworkStats();
}

void NetworkUDP::SetConfig(const NetworkConfig &config)
{
    config_ = config;
}

bool NetworkUDP::IsInitialized() const
{
    return isInitialized_;
}

void NetworkUDP::Close()
{
    if (!isInitialized_)
        return;

    isReceiving_ = false;

    if (receiveThread_.joinable())
    {
        receiveThread_.join();
    }

    if (socket_ != INVALID_SOCKET)
    {
        closesocket(socket_);
        socket_ = INVALID_SOCKET;
    }

    CleanupWinsock();
    isInitialized_ = false;
}

// Private methods implementation
bool NetworkUDP::InitializeWinsock()
{
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    return result == 0;
}

bool NetworkUDP::CreateSocket()
{
    socket_ = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (socket_ == INVALID_SOCKET)
    {
        std::cerr << "Failed to create socket: " << WSAGetLastError() << std::endl;
        return false;
    }

    // Set socket options
    int bufferSize = config_.bufferSize;
    setsockopt(socket_, SOL_SOCKET, SO_RCVBUF, (char *)&bufferSize, sizeof(bufferSize));
    setsockopt(socket_, SOL_SOCKET, SO_SNDBUF, (char *)&bufferSize, sizeof(bufferSize));

    // Set timeout
    DWORD timeout = config_.timeoutMs;
    setsockopt(socket_, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout));

    return true;
}

bool NetworkUDP::BindSocket()
{
    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(config_.localPort);
    addr.sin_addr.s_addr = INADDR_ANY;

    int result = bind(socket_, (sockaddr *)&addr, sizeof(addr));
    if (result == SOCKET_ERROR)
    {
        std::cerr << "Failed to bind socket: " << WSAGetLastError() << std::endl;
        return false;
    }

    return true;
}

bool NetworkUDP::SetupMulticast()
{
    if (!config_.enableMulticast)
    {
        return true;
    }

    // This would set up multicast if needed
    return true;
}

void NetworkUDP::CleanupWinsock()
{
    WSACleanup();
}

bool NetworkUDP::SendPacket(const PacketHeader &header, const uint8_t *payload)
{
    // Send header
    int headerSize = sizeof(PacketHeader);
    int sent = sendto(socket_, (char *)&header, headerSize, 0,
                      (sockaddr *)&remoteAddr_, sizeof(remoteAddr_));

    if (sent != headerSize)
    {
        return false;
    }

    // Send payload
    sent = sendto(socket_, (char *)payload, header.payloadSize, 0,
                  (sockaddr *)&remoteAddr_, sizeof(remoteAddr_));

    if (sent != static_cast<int>(header.payloadSize))
    {
        return false;
    }

    stats_.packetsSent++;
    stats_.bytesSent += headerSize + header.payloadSize;

    return true;
}

bool NetworkUDP::ReceivePacket(PacketHeader &header, std::vector<uint8_t> &payload)
{
    // Receive header
    int headerSize = sizeof(PacketHeader);
    sockaddr_in fromAddr;
    int fromAddrSize = sizeof(fromAddr);

    int received = recvfrom(socket_, (char *)&header, headerSize, 0,
                            (sockaddr *)&fromAddr, &fromAddrSize);

    if (received != headerSize)
    {
        return false;
    }

    // Validate header
    if (header.magic != MAGIC_NUMBER)
    {
        return false;
    }

    // Receive payload
    payload.resize(header.payloadSize);
    received = recvfrom(socket_, (char *)payload.data(), header.payloadSize, 0,
                        (sockaddr *)&fromAddr, &fromAddrSize);

    if (received != static_cast<int>(header.payloadSize))
    {
        return false;
    }

    // Validate checksum
    if (!ValidatePacket(header, payload.data()))
    {
        return false;
    }

    stats_.packetsReceived++;
    stats_.bytesReceived += headerSize + header.payloadSize;

    return true;
}

uint32_t NetworkUDP::CalculateChecksum(const uint8_t *data, size_t size)
{
    uint32_t checksum = 0;
    for (size_t i = 0; i < size; i++)
    {
        checksum += data[i];
    }
    return checksum;
}

bool NetworkUDP::ValidatePacket(const PacketHeader &header, const uint8_t *payload)
{
    uint32_t calculatedChecksum = CalculateChecksum(payload, header.payloadSize);
    return calculatedChecksum == header.checksum;
}

uint64_t NetworkUDP::GetCurrentTimestamp() const
{
    auto now = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch());
    return duration.count();
}

std::string NetworkUDP::GetLocalIP() const
{
    return "127.0.0.1"; // Simplified implementation
}

bool NetworkUDP::IsValidIP(const std::string &ip) const
{
    // Simple IP validation
    return !ip.empty() && ip != "0.0.0.0";
}

void NetworkUDP::ReceiveThread()
{
    PacketHeader header;
    std::vector<uint8_t> payload;

    while (isReceiving_)
    {
        if (ReceivePacket(header, payload))
        {
            std::lock_guard<std::mutex> lock(receiveMutex_);

            if (receiveQueue_.size() < MAX_QUEUE_SIZE)
            {
                receiveQueue_.push({payload, header.timestamp});
            }
            else
            {
                stats_.packetsLost++;
            }
        }
        else
        {
            // No packet received, small delay
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }
}
