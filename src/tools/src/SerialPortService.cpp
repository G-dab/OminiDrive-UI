#include "../include/SerialPortService.h"

#include <array>
#include <chrono>
#include <cctype>
#include <cstring>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <cerrno>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#endif

namespace {
#ifndef _WIN32
speed_t ToPosixBaud(const unsigned int baud_rate) {
    switch (baud_rate) {
        case 50: return B50;
        case 75: return B75;
        case 110: return B110;
        case 134: return B134;
        case 150: return B150;
        case 200: return B200;
        case 300: return B300;
        case 600: return B600;
        case 1200: return B1200;
        case 1800: return B1800;
        case 2400: return B2400;
        case 4800: return B4800;
        case 9600: return B9600;
        case 19200: return B19200;
        case 38400: return B38400;
        case 57600: return B57600;
#ifdef B115200
        case 115200: return B115200;
#endif
#ifdef B230400
        case 230400: return B230400;
#endif
        default: return 0;
    }
}
#endif
}  // namespace

SerialPortService::SerialPortService() = default;

SerialPortService::~SerialPortService() {
    Close();
}

bool SerialPortService::Open(const SerialPortConfig& config, std::string* error) {
    Close();

    const std::string normalized_port = NormalizePortName(config.port_name);
#ifdef _WIN32
    HANDLE handle = CreateFileA(
        normalized_port.c_str(),
        GENERIC_READ | GENERIC_WRITE,
        0,
        nullptr,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        nullptr
    );
    if (handle == INVALID_HANDLE_VALUE) {
        if (error) {
            *error = "打开串口失败，错误码: " + std::to_string(GetLastError());
        }
        return false;
    }

    DCB dcb = {0};
    dcb.DCBlength = sizeof(DCB);
    if (!GetCommState(handle, &dcb)) {
        if (error) {
            *error = "读取串口配置失败，错误码: " + std::to_string(GetLastError());
        }
        CloseHandle(handle);
        return false;
    }

    dcb.BaudRate = config.baud_rate;
    dcb.ByteSize = static_cast<BYTE>(config.data_bits);
    dcb.Parity = NOPARITY;
    if (config.parity == SerialParity::kOdd) {
        dcb.Parity = ODDPARITY;
    } else if (config.parity == SerialParity::kEven) {
        dcb.Parity = EVENPARITY;
    }
    dcb.StopBits = ONESTOPBIT;
    if (config.stop_bits == SerialStopBits::kOnePointFive) {
        dcb.StopBits = ONE5STOPBITS;
    } else if (config.stop_bits == SerialStopBits::kTwo) {
        dcb.StopBits = TWOSTOPBITS;
    }
    dcb.fBinary = TRUE;
    dcb.fParity = (dcb.Parity != NOPARITY);

    if (!SetCommState(handle, &dcb)) {
        if (error) {
            *error = "应用串口配置失败，错误码: " + std::to_string(GetLastError());
        }
        CloseHandle(handle);
        return false;
    }

    COMMTIMEOUTS timeouts = {0};
    timeouts.ReadIntervalTimeout = 20;
    timeouts.ReadTotalTimeoutConstant = 20;
    timeouts.ReadTotalTimeoutMultiplier = 0;
    timeouts.WriteTotalTimeoutConstant = 100;
    timeouts.WriteTotalTimeoutMultiplier = 0;
    if (!SetCommTimeouts(handle, &timeouts)) {
        if (error) {
            *error = "设置串口超时失败，错误码: " + std::to_string(GetLastError());
        }
        CloseHandle(handle);
        return false;
    }

    {
        std::lock_guard<std::mutex> lock(mutex_);
        native_handle_ = handle;
    }
#else
    const int fd = open(normalized_port.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (fd < 0) {
        if (error) {
            *error = "打开串口失败: " + std::string(std::strerror(errno));
        }
        return false;
    }

    termios tty{};
    if (tcgetattr(fd, &tty) != 0) {
        if (error) {
            *error = "读取串口配置失败: " + std::string(std::strerror(errno));
        }
        close(fd);
        return false;
    }

    speed_t speed = ToPosixBaud(config.baud_rate);
    if (speed == 0) {
        if (error) {
            *error = "不支持的波特率: " + std::to_string(config.baud_rate);
        }
        close(fd);
        return false;
    }
    cfsetispeed(&tty, speed);
    cfsetospeed(&tty, speed);

    tty.c_cflag &= ~CSIZE;
    switch (config.data_bits) {
        case 5: tty.c_cflag |= CS5; break;
        case 6: tty.c_cflag |= CS6; break;
        case 7: tty.c_cflag |= CS7; break;
        default: tty.c_cflag |= CS8; break;
    }

    tty.c_cflag &= ~(PARENB | PARODD);
    if (config.parity == SerialParity::kOdd) {
        tty.c_cflag |= (PARENB | PARODD);
    } else if (config.parity == SerialParity::kEven) {
        tty.c_cflag |= PARENB;
    }

    tty.c_cflag &= ~CSTOPB;
    if (config.stop_bits == SerialStopBits::kTwo) {
        tty.c_cflag |= CSTOPB;
    }

    tty.c_cflag |= (CLOCAL | CREAD);
    tty.c_lflag = 0;
    tty.c_oflag = 0;
    tty.c_iflag = 0;
    tty.c_cc[VMIN] = 0;
    tty.c_cc[VTIME] = 1;

    if (tcsetattr(fd, TCSANOW, &tty) != 0) {
        if (error) {
            *error = "应用串口配置失败: " + std::string(std::strerror(errno));
        }
        close(fd);
        return false;
    }
    {
        std::lock_guard<std::mutex> lock(mutex_);
        native_fd_ = fd;
    }
#endif

    running_.store(true);
    read_thread_ = std::thread(&SerialPortService::ReadLoop, this);
    return true;
}

void SerialPortService::Close() {
    running_.store(false);

    {
        std::lock_guard<std::mutex> lock(mutex_);
#ifdef _WIN32
        if (native_handle_) {
            CloseHandle(static_cast<HANDLE>(native_handle_));
            native_handle_ = nullptr;
        }
#else
        if (native_fd_ >= 0) {
            close(native_fd_);
            native_fd_ = -1;
        }
#endif
    }

    if (read_thread_.joinable()) {
        read_thread_.join();
    }
}

bool SerialPortService::IsOpen() const {
    return running_.load() && IsNativeOpen();
}

bool SerialPortService::Write(const std::vector<std::uint8_t>& data, std::string* error) {
    if (data.empty()) {
        return true;
    }

    std::lock_guard<std::mutex> lock(mutex_);
    if (!IsNativeOpen()) {
        if (error) {
            *error = "串口未打开";
        }
        return false;
    }

#ifdef _WIN32
    DWORD written = 0;
    const BOOL ok = WriteFile(
        static_cast<HANDLE>(native_handle_),
        data.data(),
        static_cast<DWORD>(data.size()),
        &written,
        nullptr
    );
    if (!ok || written != data.size()) {
        if (error) {
            *error = "发送失败，错误码: " + std::to_string(GetLastError());
        }
        return false;
    }
#else
    ssize_t total = 0;
    while (total < static_cast<ssize_t>(data.size())) {
        const ssize_t n = write(native_fd_, data.data() + total, data.size() - total);
        if (n < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                std::this_thread::sleep_for(std::chrono::milliseconds(2));
                continue;
            }
            if (error) {
                *error = "发送失败: " + std::string(std::strerror(errno));
            }
            return false;
        }
        total += n;
    }
#endif

    return true;
}

std::vector<std::uint8_t> SerialPortService::PollReceived() {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<std::uint8_t> out;
    out.reserve(recv_queue_.size());
    while (!recv_queue_.empty()) {
        out.push_back(recv_queue_.front());
        recv_queue_.pop_front();
    }
    return out;
}

void SerialPortService::ReadLoop() {
    std::array<std::uint8_t, 512> buffer{};
    while (running_.load()) {
        std::size_t bytes_read = 0;
#ifdef _WIN32
        DWORD read_count = 0;
        HANDLE handle = nullptr;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (!native_handle_) {
                break;
            }
            handle = static_cast<HANDLE>(native_handle_);
        }

        const BOOL ok = ReadFile(handle, buffer.data(), static_cast<DWORD>(buffer.size()), &read_count, nullptr);
        if (!ok) {
            if (!running_.load()) {
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
            continue;
        }
        bytes_read = static_cast<std::size_t>(read_count);
#else
        int fd = -1;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (native_fd_ < 0) {
                break;
            }
            fd = native_fd_;
        }

        const ssize_t n = read(fd, buffer.data(), buffer.size());
        if (n < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                std::this_thread::sleep_for(std::chrono::milliseconds(2));
                continue;
            }
            if (!running_.load()) {
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
            continue;
        }
        bytes_read = static_cast<std::size_t>(n);
#endif

        if (bytes_read == 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
            continue;
        }

        std::lock_guard<std::mutex> lock(mutex_);
        for (std::size_t i = 0; i < bytes_read; ++i) {
            recv_queue_.push_back(buffer[i]);
            if (recv_queue_.size() > kMaxRecvQueueBytes) {
                recv_queue_.pop_front();
            }
        }
    }
}

std::string SerialPortService::NormalizePortName(const std::string& input) {
#ifdef _WIN32
    if (input.size() >= 4 && input.rfind("COM", 0) == 0) {
        bool all_digits = true;
        int port_number = 0;
        for (std::size_t i = 3; i < input.size(); ++i) {
            const unsigned char c = static_cast<unsigned char>(input[i]);
            if (!std::isdigit(c)) {
                all_digits = false;
                break;
            }
            port_number = port_number * 10 + (c - '0');
        }
        if (all_digits && port_number >= 10) {
            return "\\\\.\\" + input;
        }
    }
#endif
    return input;
}

bool SerialPortService::IsNativeOpen() const {
#ifdef _WIN32
    return native_handle_ != nullptr;
#else
    return native_fd_ >= 0;
#endif
}
