#pragma once

#include "../../template/include/IPage.h"
#include "../../tools/include/SerialPortService.h"

#include <memory>
#include <string>
#include <vector>

class SerialAssistantPage : public IPage {
public:
    SerialAssistantPage() = default;
    ~SerialAssistantPage() override = default;

    const char* GetPageName() const override { return "Serial Assistant"; }
    const char* GetIcon() const override { return "S"; }

    void RenderSidePanel() override;
    void RenderMainWorkspace() override;

private:
    int port_index_ = 0;
    int baud_index_ = 1;
    int data_bits_index_ = 3;
    int stop_bits_index_ = 0;
    int parity_index_ = 0;
    bool opened_ = false;

    bool hex_send_ = false;
    bool hex_recv_ = false;
    bool auto_scroll_ = true;

    std::unique_ptr<SerialPortService> serial_service_ = std::make_unique<SerialPortService>();
    char recv_log_[16384] = "串口日志输出...\n";
    char send_buf_[4096] = "";

private:
    SerialPortConfig BuildCurrentConfig(const char* selected_port) const;
    bool ParseHexBytes(const char* input, std::vector<std::uint8_t>* out_bytes, std::string* error) const;
    std::string BytesToHex(const std::vector<std::uint8_t>& bytes) const;
    void AppendLogLine(const std::string& line);
    void PumpReceivedDataToLog();
};
