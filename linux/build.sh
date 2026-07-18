#!/bin/bash
# ============================================
#  Build script for Linux (g++)
# ============================================

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJ_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
BUILD_DIR="$SCRIPT_DIR/build"

echo "Cleaning build directory..."
rm -rf "$BUILD_DIR"
mkdir -p "$BUILD_DIR"

echo ""
echo "============================================"
echo " Configuring with qmake..."
echo "============================================"
cd "$BUILD_DIR"
qmake "$SCRIPT_DIR/Statistiques.pro" CONFIG+=release
if [ $? -ne 0 ]; then
    echo "qmake failed."
    exit 1
fi

echo ""
echo "============================================"
echo " Building with make..."
echo "============================================"
make -j$(nproc)
if [ $? -ne 0 ]; then
    echo "Build failed."
    exit 1
fi

echo ""
echo "============================================"
echo " Build complete!"
echo " Binary: $BUILD_DIR/Statistiques"
echo "============================================"

cd "$PROJ_DIR"
