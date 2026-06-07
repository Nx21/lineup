#!/usr/bin/env bash
# setup.sh — Install all build & runtime dependencies for WorldCup Analyst
#
# Supports:
#   Ubuntu / Debian  (apt)
#   Fedora / RHEL    (dnf)
#   Arch Linux       (pacman)
#   macOS            (Homebrew)
#
# Usage:
#   chmod +x setup.sh
#   ./setup.sh

set -euo pipefail

# ── Colours ───────────────────────────────────────────────────────────────────
RED='\033[0;31m'; GREEN='\033[0;32m'; YELLOW='\033[1;33m'; NC='\033[0m'
info()    { echo -e "${GREEN}[INFO]${NC}  $*"; }
warn()    { echo -e "${YELLOW}[WARN]${NC}  $*"; }
error()   { echo -e "${RED}[ERROR]${NC} $*"; exit 1; }

# ── Detect OS ─────────────────────────────────────────────────────────────────
detect_os() {
    if [[ "$OSTYPE" == "darwin"* ]]; then
        echo "macos"
    elif [ -f /etc/os-release ]; then
        # shellcheck source=/dev/null
        source /etc/os-release
        case "$ID" in
            ubuntu|debian|linuxmint|pop)  echo "debian" ;;
            fedora|rhel|centos|rocky)      echo "fedora" ;;
            arch|manjaro|endeavouros)      echo "arch"   ;;
            *)                             echo "unknown" ;;
        esac
    else
        echo "unknown"
    fi
}

OS=$(detect_os)
info "Detected OS: $OS"

# ── Install Qt6 + build tools ─────────────────────────────────────────────────
install_qt_debian() {
    info "Updating package index…"
    sudo apt-get update -qq

    info "Installing Qt6 and build dependencies…"
    sudo apt-get install -y \
        qt6-base-dev \
        qt6-base-dev-tools \
        qt6-tools-dev \
        qt6-tools-dev-tools \
        libqt6sql6-sqlite \
        qt6-l10n-tools \
        libqt6svg6-dev \
        libqt6network6 \
        cmake \
        ninja-build \
        g++ \
        make \
        git \
        python3 \
        python3-pip \
        libgl1-mesa-dev \
        libglu1-mesa-dev

    # Qt6 Test module (for unit tests)
    sudo apt-get install -y libqt6test6 qt6-base-dev || true
}

install_qt_fedora() {
    info "Installing Qt6 and build dependencies via dnf…"
    sudo dnf install -y \
        qt6-qtbase-devel \
        qt6-qtsvg-devel \
        qt6-qttools-devel \
        cmake \
        ninja-build \
        gcc-c++ \
        make \
        git \
        python3 \
        python3-pip
}

install_qt_arch() {
    info "Installing Qt6 and build dependencies via pacman…"
    sudo pacman -Sy --noconfirm \
        qt6-base \
        qt6-svg \
        qt6-tools \
        cmake \
        ninja \
        gcc \
        make \
        git \
        python \
        python-pip
}

install_qt_macos() {
    if ! command -v brew &>/dev/null; then
        warn "Homebrew not found. Installing…"
        /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
    fi
    info "Installing Qt6 and build dependencies via Homebrew…"
    brew install qt@6 cmake ninja python3 git
    # Add Qt to PATH for this session
    QT_PREFIX="$(brew --prefix qt@6)"
    export PATH="$QT_PREFIX/bin:$PATH"
    export CMAKE_PREFIX_PATH="$QT_PREFIX"
    info "Add the following to your shell profile (~/.zshrc or ~/.bash_profile):"
    echo "  export PATH=\"$QT_PREFIX/bin:\$PATH\""
    echo "  export CMAKE_PREFIX_PATH=\"$QT_PREFIX\""
}

case "$OS" in
    debian)  install_qt_debian ;;
    fedora)  install_qt_fedora ;;
    arch)    install_qt_arch   ;;
    macos)   install_qt_macos  ;;
    *)
        warn "Unrecognised OS. Please install Qt6 manually from https://www.qt.io/download"
        warn "Required Qt modules: Core, Widgets, Network, Sql, Gui, Svg, Test"
        warn "Required tools: CMake >= 3.20, GCC/Clang with C++17 support"
        ;;
esac

# ── Python dependencies (for squad scraper) ───────────────────────────────────
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REQ_FILE="$SCRIPT_DIR/requirements.txt"

if [ -f "$REQ_FILE" ]; then
    info "Installing Python dependencies from requirements.txt…"
    python3 -m pip install --break-system-packages -r "$REQ_FILE" 2>/dev/null \
        || python3 -m pip install -r "$REQ_FILE"
else
    warn "requirements.txt not found — skipping Python deps."
fi

# ── Verify versions ───────────────────────────────────────────────────────────
echo ""
info "Verifying installed tools:"
cmake --version | head -1
g++ --version   | head -1 || clang++ --version | head -1
qmake6 --version 2>/dev/null || qmake --version 2>/dev/null || true
python3 --version

echo ""
info "All dependencies installed successfully!"
info "To build the project run:"
echo ""
echo "  cd $(basename "$SCRIPT_DIR")"
echo "  cmake -B build -DCMAKE_BUILD_TYPE=Release"
echo "  cmake --build build --parallel"
echo "  ./build/WorldCupAnalyst"
echo ""
