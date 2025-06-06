# NetDebugHost

**NetDebugHost** 是一个运行于 Linux 系统的网络调试网桥程序，主要用于连接和管理 ESP32 上的 `NetDebugLink` 模块，实现远程串口桥接、WiFi 配置、命令执行等功能。

本项目基于 [LibXR](https://github.com/Jiu-xiao/libxr) 跨平台嵌入式框架构建，提供 `.deb` 安装包、自动 systemd 启动、以及命令行终端回环机制。

---

## 🔧 功能特点

- 自动识别接入的 ESP32 虚拟串口（通过 VID/PID）
- 启动本地 `bash` 终端作为调试 shell
- 串口数据自动封包并转发至 ESP32
- 支持远程命令（如 REBOOT、PING、UART 配置）
- 支持远程设置 WiFi 配置并发起连接
- GitHub Actions 自动构建 `.deb` 包并发布

---

## 🧱 架构设计

```text
+--------------------------+
|       NetDebugHost       |
+--------------------------+
|                          |
|  ┌──────────────┐        |
|  │ ShellManager │ <==╮   |
|  └──────────────┘    ║   |
|        /\            ║   |
|        || PTY        ║   |
|        \/            ║   |
|  ┌──────────────┐    ║   |
|  │ UART Wrapper │ ===╯   |
|  └──────────────┘        |
|        /\                |
|        ||                |
|        \/                |
|  ┌──────────────┐        |
|  │  Topic Bus   │◄─►ESP32|
|  └──────────────┘        |
|        ||                |
|        \/                |
|  ┌──────────────┐        |
|  │ WifiManager  │        |
|  └──────────────┘        |
|                          |
+--------------------------+
```

---

## 🚀 快速开始

### 安装预构建包

提供amd64和arm64的预构建包，请从 [Releases 页面](https://github.com/Jiu-xiao/NetDebugHost/releases) 下载并安装：

```bash
sudo dpkg -i NetDebugHost_<version>_<arch>.deb
```

### 查看服务状态

```bash
systemctl status netdebughost
journalctl -u netdebughost -f
```

---

## 🛠️ 本地构建方法

### 1. 安装依赖（Ubuntu 22.04）

```bash
sudo apt update
sudo apt install -y python3-pip build-essential cmake ninja-build libudev-dev libnm-dev libwpa-client-dev python3-pip git curl zip unzip pkg-config libeigen3-dev

pip install --upgrade pip
pip install xrobot
```

### 2. 克隆项目和 `libxr`

```bash
git clone https://github.com/Jiu-xiao/NetDebugHost.git
cd NetDebugHost
git clone https://github.com/Jiu-xiao/libxr.git
xrobot_setup
```

### 3. 编译可执行文件

```bash
mkdir -p build
cd build
cmake .. -GNinja -DCMAKE_BUILD_TYPE=Release
ninja NetDebugHost
cd ..
```

### 4. 打包为 .deb 安装包

```bash
VERSION=1.0.0 ARCH=amd64 ./build_deb.sh
```

成功后会生成类似：

```bash
NetDebugHost_1.0.0_amd64.deb
```

---

## 🧪 示例用法

安装deb包后无需特殊配置。通过 USB 连接 ESP32-C3 后，程序会自动识别串口并：

- 启动本地 shell，串口与 shell 双向绑定
- 接收 ESP32 发来的 WiFi 配置信息并尝试连接
- 处理远程发来的命令：如 `REBOOT`, `PING`, `RENAME`
- 断线后自动重连

---

## 📁 目录结构

```bash
NetDebugHost/
├── build/                    # 构建输出目录
│   ├── bin                   # 编译后的可执行文件目录
├── build_deb.sh              # .deb 打包脚本
├── CMakeLists.txt            # 项目的 CMake 构建配置
├── libxr/                    # LibXR 库
├── Modules/                  # 项目模块
│   ├── BlinkLED              # BlinkLED 模块
│   ├── NetDebugHost          # NetDebugHost 模块
│   ├── CMakeLists.txt        # 模块 CMake 配置
│   └── modules.yaml          # 模块配置文件
├── README.md                 # 项目简介
└── User/                     # 用户代码文件夹
    ├── main.cpp              # 项目入口
    ├── xrobot_main.hpp       # 主程序头文件
    └── xrobot.yaml           # 配置文件
```

---

## 📄 License

MIT License © 2025 Jiu-xiao
