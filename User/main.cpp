#include "app_framework.hpp"
#include "libxr.hpp"
#include "linux_power.hpp"
#include "linux_uart.hpp"
#include "linux_wifi_client.hpp"
#include "power.hpp"
#include "xrobot_main.hpp"
#include <libudev.h>

bool find_usb_tty_by_vid_pid(const std::string &target_vid,
                             const std::string &target_pid,
                             std::string &tty_path);

int main() {
  LibXR::PlatformInit();

  std::string tty;
  std::string vid = "303a";
  std::string pid = "1001";

  while (!find_usb_tty_by_vid_pid(vid, pid, tty)) {
    XR_LOG_WARN("Cannot find tty, retrying...");
    LibXR::Thread::Sleep(100);
  }

  XR_LOG_PASS("Found tty: %s", tty.c_str());

  LibXR::LinuxUART uart(tty.c_str());

  LibXR::LinuxWifiClient wifi_client;

  LibXR::LinuxPowerManager power_manager;

  LibXR::HardwareContainer hw(
      LibXR::Entry<LibXR::UART>{uart, {"uart_cdc"}},
      LibXR::Entry<LibXR::WifiClient>{wifi_client, {"wifi_client"}},
      LibXR::Entry<LibXR::PowerManager>{power_manager, {"power_manager"}});

  XRobotMain(hw);

  return 0;
}

bool find_usb_tty_by_vid_pid(const std::string &target_vid,
                             const std::string &target_pid,
                             std::string &tty_path) {
  struct udev *udev = udev_new();
  if (!udev) {
    XR_LOG_ERROR("Cannot create udev context");
    return false;
  }

  struct udev_enumerate *enumerate = udev_enumerate_new(udev);
  udev_enumerate_add_match_subsystem(enumerate, "tty");
  udev_enumerate_scan_devices(enumerate);

  struct udev_list_entry *devices = udev_enumerate_get_list_entry(enumerate);
  struct udev_list_entry *entry;

  bool found = false;

  udev_list_entry_foreach(entry, devices) {
    const char *path = udev_list_entry_get_name(entry);
    struct udev_device *tty_dev = udev_device_new_from_syspath(udev, path);
    if (!tty_dev)
      continue;

    struct udev_device *usb_dev = udev_device_get_parent_with_subsystem_devtype(
        tty_dev, "usb", "usb_device");

    if (usb_dev) {
      const char *vid = udev_device_get_sysattr_value(usb_dev, "idVendor");
      const char *pid = udev_device_get_sysattr_value(usb_dev, "idProduct");

      if (vid && pid && target_vid == vid && target_pid == pid) {
        const char *devnode = udev_device_get_devnode(tty_dev);
        if (devnode) {
          tty_path = devnode;
          found = true;
          udev_device_unref(tty_dev);
          break;
        }
      }
    }

    udev_device_unref(tty_dev);
  }

  udev_enumerate_unref(enumerate);
  udev_unref(udev);
  return found;
}
