#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

enum class SerialParity {
    kNone = 0,
    kOdd,
    kEven,
};

enum class SerialStopBits {
    kOne = 0,
    kOnePointFive,
    kTwo,
};

struct SerialPortConfig {
    std::string port_name;
    unsigned int baud_rate = 115200;
    int data_bits = 8;
    SerialParity parity = SerialParity::kNone;
    SerialStopBits stop_bits = SerialStopBits::kOne;
};

class SerialPortService {
public:
    SerialPortService();
    ~SerialPortService();

    bool Open(const SerialPortConfig& config, std::string* error);
    void Close();

    bool IsOpen() const;
    bool Write(const std::vector<std::uint8_t>& data, std::string* error);
    std::vector<std::uint8_t> PollReceived();

private:
    void ReadLoop();
    static std::string NormalizePortName(const std::string& input);
    bool IsNativeOpen() const;

private:
    mutable std::mutex mutex_;
#ifdef _WIN32
    void* native_handle_ = nullptr;
#else
    int native_fd_ = -1;
#endif
    std::thread read_thread_;
    std::atomic<bool> running_{false};
    std::deque<std::uint8_t> recv_queue_;
    static constexpr std::size_t kMaxRecvQueueBytes = 1024 * 1024;
};
