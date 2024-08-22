#!/bin/bash

set -e

# Define variables for AppImage creation
APP_NAME="HansolClientApp"
VERSION="1.0"
APP_DIR="${APP_NAME}AppDir"
APP_BINARY="httppostclient"
LOGSERVICE_BINARY="logservice"
APP_DESKTOP_FILE="${APP_NAME}.desktop"
APP_ICON="hansol.png"
OUTPUT_DIR="release"
OUTPUT_FILE="${OUTPUT_DIR}/${APP_NAME}-${VERSION}-x86_64.AppImage"
APPIMAGE_TOOL_PATH="../appimagetool/appimagetool"

# Step 1: Create the build folder and run CMake
echo "Creating build directory..."
mkdir -p build
cd build

echo "Running CMake..."
cmake ..

echo "Building the project..."
make

# Step 2: Create AppDir structure
echo "Creating AppDir structure..."
rm -rf "$APP_DIR"
mkdir -p "$APP_DIR/usr/bin"
mkdir -p "$APP_DIR/usr/lib"
mkdir -p "$APP_DIR/usr/share/icons/hicolor/256x256/apps"
mkdir -p "$APP_DIR/usr/share/applications"

# Copy binaries
echo "Copying binaries to AppDir..."
cp httppostclient/httppostclient "$APP_DIR/usr/bin/"
cp logservice/logservice "$APP_DIR/usr/bin/"

# Check and copy library dependencies
echo "Checking library dependencies for httppostclient..."
httppostclient_deps=$(ldd httppostclient/httppostclient | grep '=>' | awk '{print $3}')

echo "Checking library dependencies for logservice..."
logservice_deps=$(ldd logservice/logservice | grep '=>' | awk '{print $3}')

# Combine all dependencies and remove duplicates
all_deps=$(echo "$httppostclient_deps $logservice_deps" | tr ' ' '\n' | sort | uniq)

# Print out the list of all dependencies
echo "All library dependencies:"
echo "$all_deps"

# Copy the libraries needed for the binaries
echo "Copying libraries to AppDir..."
for lib in $all_deps; do
    if [ -f "$lib" ]; then
        echo "Copying library: $lib"
        cp "$lib" "$APP_DIR/usr/lib/"
    else
        echo "Library not found: $lib"
    fi
done

# Create AppRun script
echo "Creating AppRun script..."
cat > "$APP_DIR/AppRun" << EOF
#!/bin/bash
HERE="\$(dirname "\$(readlink -f "\$0")")"
exec "\$HERE/usr/bin/$APP_BINARY" "\$@"
EOF
chmod +x "$APP_DIR/AppRun"

# Create .desktop file
echo "Creating .desktop file..."
cat > "$APP_DIR/usr/share/applications/$APP_DESKTOP_FILE" << EOF
[Desktop Entry]
Name=$APP_NAME
Exec=httppostclient
Icon=hansol
Type=Application
Categories=Utility;
EOF

# Copy application icon
echo "Copying application icon..."
cp ../$APP_ICON "$APP_DIR/"
cp ../$APP_ICON "$APP_DIR/usr/share/icons/hicolor/256x256/apps/"

# Create symlink for .desktop file
echo "Creating symlink for .desktop file..."
ln -s "usr/share/applications/$APP_DESKTOP_FILE" "$APP_DIR/$APP_DESKTOP_FILE"

# Create AppImage
echo "Creating AppImage..."
mkdir -p "$OUTPUT_DIR"
$APPIMAGE_TOOL_PATH "$APP_DIR" "$OUTPUT_FILE"

# Clean up
echo "Cleaning up..."
rm -rf "$APP_DIR"

echo "AppImage created at $OUTPUT_FILE"
