#include "../include/SerialAssistantPage.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdio>
#include <cstring>

namespace {
constexpr std::array<const char*, 18> kPorts = {
    "COM1",
    "COM2",
    "COM3",
    "COM4",
    "COM5",
    "COM6",
    "COM7",
    "COM8",
    "COM9",
    "COM10",
    "/dev/ttyUSB0",
    "/dev/ttyUSB1",
    "/dev/ttyACM0",
    "/dev/ttyACM1",
    "/dev/ttyS0",
    "/dev/ttyS1",
    "/dev/cu.usbserial",
    "/dev/cu.usbmodem",
};

constexpr std::array<const char*, 6> kBaud = {"9600", "19200", "38400", "57600", "115200", "230400"};
constexpr std::array<const char*, 4> kDataBits = {"5", "6", "7", "8"};
constexpr std::array<const char*, 3> kStopBits = {"1", "1.5", "2"};
constexpr std::array<const char*, 3> kParity = {"无", "奇校验", "偶校验"};
}  // namespace

void SerialAssistantPage::RenderSidePanel() {
    ImGui::TextUnformatted("串口配置");
    ImGui::Separator();
    ImGui::Combo("端口", &port_index_, kPorts.data(), static_cast<int>(kPorts.size()));
    ImGui::Combo("波特率", &baud_index_, kBaud.data(), static_cast<int>(kBaud.size()));
    ImGui::Combo("数据位", &data_bits_index_, kDataBits.data(), static_cast<int>(kDataBits.size()));
    ImGui::Combo("停止位", &stop_bits_index_, kStopBits.data(), static_cast<int>(kStopBits.size()));
    ImGui::Combo("校验位", &parity_index_, kParity.data(), static_cast<int>(kParity.size()));

    ImGui::Spacing();
    if (!opened_) {
        if (ImGui::Button("打开串口", ImVec2(-1, 36))) {
            std::string error;
            const char* selected_port = kPorts[std::clamp(port_index_, 0, static_cast<int>(kPorts.size()) - 1)];
            const SerialPortConfig config = BuildCurrentConfig(selected_port);
            if (serial_service_ && serial_service_->Open(config, &error)) {
                opened_ = true;
                AppendLogLine(std::string("[信息] 串口已打开: ") + selected_port);
            } else {
                AppendLogLine(std::string("[错误] ") + (error.empty() ? "打开串口失败" : error));
            }
        }
    } else {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.65f, 0.2f, 0.2f, 1.0f));
        if (ImGui::Button("关闭串口", ImVec2(-1, 36))) {
            if (serial_service_) {
                serial_service_->Close();
            }
            opened_ = false;
            AppendLogLine("[信息] 串口已关闭");
        }
        ImGui::PopStyleColor();
    }
}

void SerialAssistantPage::RenderMainWorkspace() {
    if (serial_service_ && !serial_service_->IsOpen() && opened_) {
        opened_ = false;
        AppendLogLine("[警告] 串口连接已断开");
    }
    PumpReceivedDataToLog();

    ImGui::TextUnformatted("接收区");
    ImGui::Checkbox("十六进制接收", &hex_recv_);
    ImGui::SameLine();
    ImGui::Checkbox("自动滚动", &auto_scroll_);

    const float top_h = ImGui::GetContentRegionAvail().y * 0.58f;
    ImGui::InputTextMultiline(
        "##rxlog",
        recv_log_,
        sizeof(recv_log_),
        ImVec2(-1.0f, top_h),
        ImGuiInputTextFlags_ReadOnly
    );
    if (auto_scroll_ && ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
        ImGui::SetScrollHereY(1.0f);
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::TextUnformatted("发送区");
    ImGui::Checkbox("十六进制发送", &hex_send_);
    ImGui::InputTextMultiline("##txbuf", send_buf_, sizeof(send_buf_), ImVec2(-1.0f, -40.0f));
    if (ImGui::Button("发送", ImVec2(120, 32))) {
        if (!opened_ || !serial_service_) {
            AppendLogLine("[错误] 串口未打开，无法发送");
            return;
        }

        std::vector<std::uint8_t> tx_bytes;
        std::string parse_error;
        if (hex_send_) {
            if (!ParseHexBytes(send_buf_, &tx_bytes, &parse_error)) {
                AppendLogLine(std::string("[错误] HEX 发送内容无效: ") + parse_error);
                return;
            }
        } else {
            const std::string text(send_buf_);
            tx_bytes.assign(text.begin(), text.end());
        }

        std::string write_error;
        if (!serial_service_->Write(tx_bytes, &write_error)) {
            AppendLogLine(std::string("[错误] ") + (write_error.empty() ? "发送失败" : write_error));
            return;
        }

        if (hex_send_) {
            AppendLogLine(std::string("[发送] ") + BytesToHex(tx_bytes));
        } else {
            const char* msg = (std::strlen(send_buf_) == 0) ? "(空)" : send_buf_;
            AppendLogLine(std::string("[发送] ") + msg);
        }
    }
}

SerialPortConfig SerialAssistantPage::BuildCurrentConfig(const char* selected_port) const {
    SerialPortConfig config;
    config.port_name = selected_port;
    config.baud_rate = static_cast<unsigned int>(std::stoi(kBaud[std::clamp(baud_index_, 0, static_cast<int>(kBaud.size()) - 1)]));
    config.data_bits = std::stoi(kDataBits[std::clamp(data_bits_index_, 0, static_cast<int>(kDataBits.size()) - 1)]);

    const int parity_index = std::clamp(parity_index_, 0, static_cast<int>(kParity.size()) - 1);
    if (parity_index == 1) {
        config.parity = SerialParity::kOdd;
    } else if (parity_index == 2) {
        config.parity = SerialParity::kEven;
    } else {
        config.parity = SerialParity::kNone;
    }

    const int stop_bits_index = std::clamp(stop_bits_index_, 0, static_cast<int>(kStopBits.size()) - 1);
    if (stop_bits_index == 1) {
        config.stop_bits = SerialStopBits::kOnePointFive;
    } else if (stop_bits_index == 2) {
        config.stop_bits = SerialStopBits::kTwo;
    } else {
        config.stop_bits = SerialStopBits::kOne;
    }
    return config;
}

bool SerialAssistantPage::ParseHexBytes(
    const char* input,
    std::vector<std::uint8_t>* out_bytes,
    std::string* error
) const {
    out_bytes->clear();
    std::string token;
    for (const char* p = input; *p != '\0'; ++p) {
        const unsigned char c = static_cast<unsigned char>(*p);
        if (std::isspace(c) || *p == ',') {
            if (!token.empty()) {
                if (token.size() > 2) {
                    if (error) {
                        *error = "每个 HEX 字节最多 2 位";
                    }
                    return false;
                }
                char* end_ptr = nullptr;
                const unsigned long value = std::strtoul(token.c_str(), &end_ptr, 16);
                if (end_ptr == token.c_str() || *end_ptr != '\0' || value > 0xFFUL) {
                    if (error) {
                        *error = "包含非法十六进制字符";
                    }
                    return false;
                }
                out_bytes->push_back(static_cast<std::uint8_t>(value));
                token.clear();
            }
            continue;
        }
        token.push_back(static_cast<char>(std::toupper(c)));
    }

    if (!token.empty()) {
        if (token.size() > 2) {
            if (error) {
                *error = "每个 HEX 字节最多 2 位";
            }
            return false;
        }
        char* end_ptr = nullptr;
        const unsigned long value = std::strtoul(token.c_str(), &end_ptr, 16);
        if (end_ptr == token.c_str() || *end_ptr != '\0' || value > 0xFFUL) {
            if (error) {
                *error = "包含非法十六进制字符";
            }
            return false;
        }
        out_bytes->push_back(static_cast<std::uint8_t>(value));
    }

    if (out_bytes->empty()) {
        if (error) {
            *error = "输入为空";
        }
        return false;
    }
    return true;
}

std::string SerialAssistantPage::BytesToHex(const std::vector<std::uint8_t>& bytes) const {
    if (bytes.empty()) {
        return "(空)";
    }
    static const char* kHex = "0123456789ABCDEF";
    std::string out;
    out.reserve(bytes.size() * 3);
    for (std::size_t i = 0; i < bytes.size(); ++i) {
        const std::uint8_t b = bytes[i];
        out.push_back(kHex[(b >> 4) & 0x0F]);
        out.push_back(kHex[b & 0x0F]);
        if (i + 1 < bytes.size()) {
            out.push_back(' ');
        }
    }
    return out;
}

void SerialAssistantPage::AppendLogLine(const std::string& line) {
    const std::string suffix = line + "\n";
    const std::size_t cur_len = std::strlen(recv_log_);
    const std::size_t need_len = cur_len + suffix.size();
    if (need_len >= sizeof(recv_log_)) {
        const std::size_t drop = std::min(cur_len, suffix.size() + 512);
        std::memmove(recv_log_, recv_log_ + drop, cur_len - drop + 1);
    }
    const std::size_t dst_len = std::strlen(recv_log_);
    const std::size_t remain = sizeof(recv_log_) - dst_len - 1;
    if (remain > 0) {
        std::strncat(recv_log_, suffix.c_str(), remain);
    }
}

void SerialAssistantPage::PumpReceivedDataToLog() {
    if (!serial_service_ || !opened_) {
        return;
    }

    const std::vector<std::uint8_t> incoming = serial_service_->PollReceived();
    if (incoming.empty()) {
        return;
    }

    if (hex_recv_) {
        AppendLogLine(std::string("[接收] ") + BytesToHex(incoming));
    } else {
        std::string text;
        text.reserve(incoming.size());
        for (std::uint8_t byte : incoming) {
            if (byte == '\r') {
                continue;
            }
            if (byte == '\n' || (byte >= 32 && byte <= 126)) {
                text.push_back(static_cast<char>(byte));
            } else {
                text.push_back('.');
            }
        }
        AppendLogLine(std::string("[接收] ") + text);
    }
}
