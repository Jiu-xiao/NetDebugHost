#!/bin/bash
set -e

APP_NAME="NetDebugHost"
SERVICE_NAME="netdebughost"
VERSION="1.0.0"
ARCH="amd64"
DEB_DIR="${APP_NAME}_${VERSION}"

# 清理旧目录
rm -rf "$DEB_DIR"
mkdir -p "$DEB_DIR/usr/bin"
mkdir -p "$DEB_DIR/lib/systemd/system"
mkdir -p "$DEB_DIR/DEBIAN"

# 1. 拷贝可执行文件
cp "build/bin/${APP_NAME}" "$DEB_DIR/usr/bin/"
chmod 755 "$DEB_DIR/usr/bin/${APP_NAME}"

# 2. 控制文件
cat >"$DEB_DIR/DEBIAN/control" <<EOF
Package: ${SERVICE_NAME}
Version: ${VERSION}
Section: utils
Priority: optional
Architecture: ${ARCH}
Maintainer: Xiao Jiu <you@example.com>
Depends: libc6 (>= 2.31), libudev1 (>= 249), libnm0 (>= 1.36.0)
Description: Lightweight ESP32 network debugging tool using LibXR.
 Auto-starts via systemd on boot.
EOF

# 3. systemd 服务文件
cat >"$DEB_DIR/lib/systemd/system/${SERVICE_NAME}.service" <<EOF
[Unit]
Description=NetDebugHost Network Debug Bridge
After=network.target

[Service]
ExecStart=/usr/bin/${APP_NAME}
Restart=always
RestartSec=3

[Install]
WantedBy=multi-user.target
EOF

# 4. 安装后执行脚本：注册并启动服务
cat >"$DEB_DIR/DEBIAN/postinst" <<EOF
#!/bin/bash
set -e
systemctl daemon-reload
systemctl enable ${SERVICE_NAME}.service
systemctl start ${SERVICE_NAME}.service
echo "${APP_NAME} installed and service started."
EOF
chmod +x "$DEB_DIR/DEBIAN/postinst"

# 5. 卸载后执行脚本：关闭并清理服务
cat >"$DEB_DIR/DEBIAN/postrm" <<EOF
#!/bin/bash
set -e
systemctl stop ${SERVICE_NAME}.service || true
systemctl disable ${SERVICE_NAME}.service || true
systemctl daemon-reload
echo "${APP_NAME} service removed."
EOF
chmod +x "$DEB_DIR/DEBIAN/postrm"

# 6. 构建 .deb 包
dpkg-deb --build "$DEB_DIR"
