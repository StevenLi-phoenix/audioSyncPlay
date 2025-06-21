#include <iostream>
#include <string>
#include <cstdlib>
#include <csignal>
#include <memory>

// Forward declarations
class Sender;
class Receiver;

// Global variables for signal handling
std::atomic<bool> g_running(true);
std::unique_ptr<Sender> g_sender;
std::unique_ptr<Receiver> g_receiver;

/**
 * @brief Signal handler for graceful shutdown
 */
void SignalHandler(int signal)
{
    std::cout << "\nReceived signal " << signal << ", shutting down..." << std::endl;
    g_running = false;
}

/**
 * @brief Print usage information
 */
void PrintUsage(const char *programName)
{
    std::cout << "Spotify Sync Audio - Multi-device audio synchronization\n\n";
    std::cout << "Usage: " << programName << " [MODE] [OPTIONS]\n\n";
    std::cout << "Modes:\n";
    std::cout << "  sender     Run as audio sender (captures and streams audio)\n";
    std::cout << "  receiver   Run as audio receiver (receives and plays audio)\n\n";
    std::cout << "Sender Options:\n";
    std::cout << "  --ip <IP>          Destination IP address (default: 127.0.0.1)\n";
    std::cout << "  --port <PORT>      Destination port (default: 8080)\n";
    std::cout << "  --device <DEVICE>  Audio capture device name\n";
    std::cout << "  --format <RATE>    Sample rate (default: 44100)\n\n";
    std::cout << "Receiver Options:\n";
    std::cout << "  --port <PORT>      Listen port (default: 8080)\n";
    std::cout << "  --device <DEVICE>  Audio playback device name\n";
    std::cout << "  --latency <MS>     Target latency in milliseconds (default: 100)\n";
    std::cout << "  --volume <0.0-1.0> Playback volume (default: 1.0)\n\n";
    std::cout << "General Options:\n";
    std::cout << "  --help             Show this help message\n";
    std::cout << "  --version          Show version information\n\n";
    std::cout << "Examples:\n";
    std::cout << "  " << programName << " sender --ip 192.168.1.100 --port 8080\n";
    std::cout << "  " << programName << " receiver --port 8080 --latency 150\n";
}

/**
 * @brief Print version information
 */
void PrintVersion()
{
    std::cout << "Spotify Sync Audio v1.0.0\n";
    std::cout << "Build: " << __DATE__ << " " << __TIME__ << "\n";
    std::cout << "Platform: Windows\n";
    std::cout << "Compiler: " << __VERSION__ << "\n";
}

/**
 * @brief Parse command line arguments
 */
struct ProgramArgs
{
    std::string mode;
    std::string ip = "127.0.0.1";
    uint16_t port = 8080;
    std::string device = "";
    int sampleRate = 44100;
    int latency = 100;
    float volume = 1.0f;
    bool help = false;
    bool version = false;
};

ProgramArgs ParseArguments(int argc, char *argv[])
{
    ProgramArgs args;

    if (argc < 2)
    {
        args.help = true;
        return args;
    }

    args.mode = argv[1];

    for (int i = 2; i < argc; i++)
    {
        std::string arg = argv[i];

        if (arg == "--help" || arg == "-h")
        {
            args.help = true;
        }
        else if (arg == "--version" || arg == "-v")
        {
            args.version = true;
        }
        else if (arg == "--ip" && i + 1 < argc)
        {
            args.ip = argv[++i];
        }
        else if (arg == "--port" && i + 1 < argc)
        {
            args.port = static_cast<uint16_t>(std::stoi(argv[++i]));
        }
        else if (arg == "--device" && i + 1 < argc)
        {
            args.device = argv[++i];
        }
        else if (arg == "--format" && i + 1 < argc)
        {
            args.sampleRate = std::stoi(argv[++i]);
        }
        else if (arg == "--latency" && i + 1 < argc)
        {
            args.latency = std::stoi(argv[++i]);
        }
        else if (arg == "--volume" && i + 1 < argc)
        {
            args.volume = std::stof(argv[++i]);
        }
        else
        {
            std::cerr << "Unknown argument: " << arg << std::endl;
            args.help = true;
        }
    }

    return args;
}

/**
 * @brief Main function
 */
int main(int argc, char *argv[])
{
    // Parse command line arguments
    ProgramArgs args = ParseArguments(argc, argv);

    // Handle help and version
    if (args.help)
    {
        PrintUsage(argv[0]);
        return 0;
    }

    if (args.version)
    {
        PrintVersion();
        return 0;
    }

    // Validate mode
    if (args.mode != "sender" && args.mode != "receiver")
    {
        std::cerr << "Error: Invalid mode '" << args.mode << "'\n";
        std::cerr << "Use 'sender' or 'receiver'\n\n";
        PrintUsage(argv[0]);
        return 1;
    }

    // Set up signal handling
    signal(SIGINT, SignalHandler);
    signal(SIGTERM, SignalHandler);

    try
    {
        if (args.mode == "sender")
        {
            std::cout << "Starting Spotify Sync Audio Sender...\n";
            std::cout << "Destination: " << args.ip << ":" << args.port << "\n";
            std::cout << "Sample Rate: " << args.sampleRate << " Hz\n";
            if (!args.device.empty())
            {
                std::cout << "Device: " << args.device << "\n";
            }
            std::cout << "Press Ctrl+C to stop\n\n";

            // TODO: Initialize and run sender
            // g_sender = std::make_unique<Sender>(args);
            // g_sender->Run();
        }
        else if (args.mode == "receiver")
        {
            std::cout << "Starting Spotify Sync Audio Receiver...\n";
            std::cout << "Listen Port: " << args.port << "\n";
            std::cout << "Target Latency: " << args.latency << " ms\n";
            std::cout << "Volume: " << args.volume << "\n";
            if (!args.device.empty())
            {
                std::cout << "Device: " << args.device << "\n";
            }
            std::cout << "Press Ctrl+C to stop\n\n";

            // TODO: Initialize and run receiver
            // g_receiver = std::make_unique<Receiver>(args);
            // g_receiver->Run();
        }

        // Main loop
        while (g_running)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    std::cout << "Shutdown complete.\n";
    return 0;
}
