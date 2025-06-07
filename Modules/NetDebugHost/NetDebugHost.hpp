#pragma once

// clang-format off
/* === MODULE MANIFEST ===
module_name: NetDebugHost
module_description: No description provided
constructor_args:
required_hardware: uart_cdc wifi_client
=== END MANIFEST === */
// clang-format on

#include "app_framework.hpp"
#include "libxr.hpp"
#include "logger.hpp"
#include "net/wifi_client.hpp"
#include "power.hpp"
#include "semaphore.hpp"
#include "thread.hpp"
#include "uart.hpp"

class NetDebugHost : public LibXR::Application {
public:
  class Command {
  public:
    enum class Type : uint8_t {
      PING = 0,
      REMOTE_PING = 1,
      REBOOT = 2,
      RENAME = 3,
      CONFIG_UART = 4,
    };

    Type type;
    union {
      char device_name[32];
      struct {
        uint8_t uart_index;
        LibXR::UART::Configuration uart_config;
      } uart_config;
    } data;
  };

  NetDebugHost(LibXR::HardwareContainer &hw, LibXR::ApplicationManager &app)
      : uart_topic_("uart_cdc", 0x100000),
        wifi_config_topic_("wifi_config", sizeof(LibXR::WifiClient::Config)),
        command_topic_("command", sizeof(Command)) {
    uart_ = hw.template FindOrExit<LibXR::UART>({"uart_cdc"});
    wifi_client_ = hw.template FindOrExit<LibXR::WifiClient>({"wifi_client"});
    power_manager_ =
        hw.template FindOrExit<LibXR::PowerManager>({"power_manager"});

    wifi_client_->Enable();

    if (wifi_client_->IsConnected()) {
      XR_LOG_INFO("Wifi connected");
    } else {
      XR_LOG_INFO("Wifi not connected");
    }

    SpawnShell();

    void (*wifi_config_cb_fun)(bool in_isr, NetDebugHost *,
                               LibXR::RawData &) = [](bool, NetDebugHost *self,
                                                      LibXR::RawData &data) {
      LibXR::WifiClient::Config *config =
          (LibXR::WifiClient::Config *)data.addr_;
      XR_LOG_INFO("Wifi config: SSID: %s, Password: %s", config->ssid,
                  config->password);
      memcpy(&self->wifi_config_, config, sizeof(LibXR::WifiClient::Config));
      self->wifi_connect_sem_.Post();
    };

    auto wifi_config_cb =
        LibXR::Topic::Callback::Create(wifi_config_cb_fun, this);

    wifi_config_topic_.RegisterCallback(wifi_config_cb);

    shell_read_thread_.Create(this, ShellReadThread, "ShellReadThread", 512,
                              LibXR::Thread::Priority::MEDIUM);

    shell_write_thread_.Create(this, ShellWriteThread, "ShellWriteThread", 512,
                               LibXR::Thread::Priority::MEDIUM);

    wifi_connect_thread_.Create(this, WifiConnectThread, "WifiConnectThread",
                                512, LibXR::Thread::Priority::MEDIUM);

    void (*command_cb_fun)(bool in_isr, NetDebugHost *, LibXR::RawData &) =
        [](bool, NetDebugHost *self, LibXR::RawData &data) {
          Command *cmd = (Command *)data.addr_;
          switch (cmd->type) {
          case Command::Type::PING: {
            static LibXR::Topic::PackedData<Command> buf;
            static LibXR::WriteOperation op(self->uart_write_sem_, 10);
            LibXR::Topic::PackData(
                LibXR::Topic::TopicHandle(self->command_topic_)->data_.crc32,
                buf, Command::Type::REMOTE_PING);
            self->uart_->Write(buf, op);
            XR_LOG_DEBUG("Ping");
            break;
          }
          case Command::Type::REBOOT:
            XR_LOG_INFO("Rebooting...");
            self->power_manager_->Reset();
            break;
          default:
            break;
          }
        };

    auto command_cb = LibXR::Topic::Callback::Create(command_cb_fun, this);

    command_topic_.RegisterCallback(command_cb);

    app.Register(*this);
  }

  void SpawnShell();

  void KillShell();

  static void ShellReadThread(NetDebugHost *self);

  static void ShellWriteThread(NetDebugHost *self);

  static void WifiConnectThread(NetDebugHost *self) {
    while (true) {
      if (self->wifi_connect_sem_.Wait() == ErrorCode::OK) {
        XR_LOG_INFO("Start wifi connect...");
        auto ans = self->wifi_client_->Connect(self->wifi_config_);
        if (ans == LibXR::WifiClient::WifiError::NONE) {
          XR_LOG_INFO("Wifi connected");
        } else {
          XR_LOG_INFO("Wifi connect failed");
        }
      }
    }
  }

  void OnMonitor() override {}

private:
  LibXR::UART *uart_;
  LibXR::WifiClient *wifi_client_;
  LibXR::PowerManager *power_manager_;

  LibXR::Topic uart_topic_;
  LibXR::Topic wifi_config_topic_;
  LibXR::Topic command_topic_;

  int shell_stdin_fd_;
  int shell_stdout_fd_;

  LibXR::WifiClient::Config wifi_config_;

  LibXR::Semaphore shell_sem_;
  LibXR::Semaphore wifi_connect_sem_;
  LibXR::Semaphore uart_write_sem_;

  LibXR::Thread shell_read_thread_;
  LibXR::Thread shell_write_thread_;
  LibXR::Thread shell_daemon_thread_;
  LibXR::Thread wifi_connect_thread_;
};
