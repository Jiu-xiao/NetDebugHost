#include "NetDebugHost.hpp"
#include "libxr_rw.hpp"

#include "libxr.hpp"
#include "logger.hpp"
#include "message.hpp"
#include "thread.hpp"
#include "uart.hpp"
#include <csignal>
#include <cstdint>
#include <fcntl.h>
#include <pty.h>
#include <spawn.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>

static pid_t pid;

using namespace LibXR;

static constexpr const char *SHELL_PATH = "/bin/bash";

void KillShell() { kill(pid, SIGKILL); }

void NetDebugHost::SpawnShell() {
  void (*daemon_thread_fun)(NetDebugHost *) = [](NetDebugHost *self) {
    while (true) {
      static bool inited = false;
      if (!inited) {
        inited = true;
      } else {
        int status = 0;
        waitpid(pid, &status, 0);
        XR_LOG_INFO("Shell exited with status %d", status);
      }
      int pty_fd;
      int pty_master_fd;
      pid_t shell_pid;
      pid = forkpty(&pty_fd, nullptr, nullptr, nullptr);
      if (pid == 0) {
        execl(SHELL_PATH, "bash", "-c", "cd $HOME; exec bash", nullptr);
        _exit(1);
      }

      shell_pid = pid;
      pty_master_fd = pty_fd;
      fcntl(pty_master_fd, F_SETFL, O_NONBLOCK);

      self->shell_stdin_fd_ = pty_master_fd;
      self->shell_stdout_fd_ = pty_master_fd;

      XR_LOG_INFO("Shell started with PID %d", pid);

      self->shell_sem_.Post();
    }
  };

  shell_daemon_thread_.Create(this, daemon_thread_fun, "shell_daemon", 0,
                              Thread::Priority::MEDIUM);

  shell_sem_.Wait();
}

void NetDebugHost::ShellReadThread(NetDebugHost *self) {
  uint8_t buf[40960];
  uint8_t pack_buffer[40960 + LibXR::Topic::PACK_BASE_SIZE];
  LibXR::Semaphore sem;
  LibXR::WriteOperation op(sem);

  int fd = self->shell_stdout_fd_;

  while (true) {
    fd_set read_fds;
    FD_ZERO(&read_fds);
    FD_SET(fd, &read_fds);

    // 等待最多 1 秒（可选）
    struct timeval timeout;
    timeout.tv_sec = 10;
    timeout.tv_usec = 0;

    int ret = select(fd + 1, &read_fds, nullptr, nullptr, &timeout);
    if (ret > 0 && FD_ISSET(fd, &read_fds)) {
      ssize_t n = read(fd, buf, sizeof(buf));
      if (n > 0) {
        LibXR::Topic::PackData(
            LibXR::Topic::TopicHandle(self->uart_topic_)->data_.crc32,
            pack_buffer, LibXR::RawData{buf, (size_t)n});
        self->uart_->Write(
            {pack_buffer, (size_t)n + LibXR::Topic::PACK_BASE_SIZE}, op);
      }
    } else if (ret < 0) {
      XR_LOG_ERROR("select failed: %s", strerror(errno));
      LibXR::Thread::Sleep(1);
    }
  }
}

void NetDebugHost::ShellWriteThread(NetDebugHost *self) {
  uint8_t buf[40960];
  LibXR::Semaphore sem;
  LibXR::ReadOperation op(sem);
  LibXR::RawData read_buffer(buf);
  LibXR::Topic::Server server(40960);
  server.Register(self->uart_topic_);
  server.Register(self->wifi_config_topic_);
  server.Register(self->command_topic_);
  void (*shell_write_cb_fun)(bool in_isr, NetDebugHost *, RawData &) =
      [](bool, NetDebugHost *self, LibXR::RawData &data) {
        write(self->shell_stdin_fd_, data.addr_, data.size_);
      };
  auto shell_write_cb =
      LibXR::Topic::Callback::Create(shell_write_cb_fun, self);

  self->uart_topic_.RegisterCallback(shell_write_cb);

  while (true) {
    read_buffer.size_ =
        LibXR::min(sizeof(buf), self->uart_->read_port_->Size());
    self->uart_->Read(read_buffer, op);
    if (read_buffer.size_ == 0) {
      continue;
    }
    buf[read_buffer.size_] = 0;
    XR_LOG_DEBUG("Read %d bytes: %s", read_buffer.size_,
                 (char *)read_buffer.addr_);
    server.ParseData({buf, (size_t)read_buffer.size_});
  }
}
