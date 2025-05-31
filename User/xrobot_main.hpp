#include "app_framework.hpp"
#include "libxr.hpp"

// Module headers
#include "NetDebugHost.hpp"
#include "BlinkLED.hpp"

static void XRobotMain(LibXR::HardwareContainer &hw) {
  using namespace LibXR;
  ApplicationManager appmgr;

  // Auto-generated module instantiations
  static NetDebugHost netdebughost(hw, appmgr);

  while (true) {
    appmgr.MonitorAll();
    Thread::Sleep(1000);
  }
}