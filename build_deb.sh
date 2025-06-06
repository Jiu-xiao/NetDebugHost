#!/bin/bash
set -e

APP_NAME="NetDebugHost"
VERSION="${VERSION:-1.0.0}"
# 如果 VERSION 不以数字开头（比如 "master"），则使用 fallback
if [[ ! "$VERSION" =~ ^[0-9] ]]; then
    VERSION="0.0.0+$(date +%Y%m%d)-${GITHUB_SHA:-local}"
fi
ARCH="${ARCH:-amd64}"
SERVICE_NAME="netdebughost"
DEB_DIR="${APP_NAME}_${VERSION}_${ARCH}"

echo "[*] Packaging $APP_NAME version $VERSION for $ARCH..."

# 1. 清理并创建目录结构
rm -rf "$DEB_DIR"
mkdir -p "$DEB_DIR/usr/bin"
mkdir -p "$DEB_DIR/lib/systemd/system"
mkdir -p "$DEB_DIR/DEBIAN"

# 2. 拷贝可执行文件
cp "build/bin/${APP_NAME}" "$DEB_DIR/usr/bin/"
chmod 755 "$DEB_DIR/usr/bin/${APP_NAME}"

# 3. control 文件
cat >"$DEB_DIR/DEBIAN/control" <<EOF
Package: ${SERVICE_NAME}
Version: ${VERSION}
Section: utils
Priority: optional
Architecture: ${ARCH}
Maintainer: Xiao Jiu <you@example.com>
Depends: libc6 (>= 2.31), libudev1 (>= 249), libnm0 (>= 1.36.0)
Description: Lightweight ESP32 network debugging tool using LibXR.
 Automatically starts via systemd at boot.
EOF

# 4. systemd 服务文件
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

# 5. 安装后脚本（注册 systemd 服务）
cat >"$DEB_DIR/DEBIAN/postinst" <<EOF
#!/bin/bash
set -e
systemctl daemon-reload
systemctl enable ${SERVICE_NAME}.service
systemctl start ${SERVICE_NAME}.service
echo "${APP_NAME} installed and service started."
EOF
chmod +x "$DEB_DIR/DEBIAN/postinst"

# 6. 卸载后脚本（清理服务）
cat >"$DEB_DIR/DEBIAN/postrm" <<EOF
#!/bin/bash
set -e
systemctl stop ${SERVICE_NAME}.service || true
systemctl disable ${SERVICE_NAME}.service || true
systemctl daemon-reload
echo "${APP_NAME} service removed."
EOF
chmod +x "$DEB_DIR/DEBIAN/postrm"

# 7. 构建 deb 包
dpkg-deb --build "$DEB_DIR"

# 8. 输出路径
echo "[+] Package built: ${DEB_DIR}.deb"
