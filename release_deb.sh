#!/bin/bash

# Step 1: Create the build folder and run CMake
echo "Creating build directory..."
mkdir -p build
cd build

echo "Running CMake..."
cmake ..

echo "Building the project..."
make

# Step 3: Prepare the release directory
RELEASE_DIR="release"

if [ -d "$RELEASE_DIR" ]; then
    echo "Removing existing release directory..."
    rm -rf "$RELEASE_DIR"
fi

echo "Creating new release directory..."
mkdir -p "$RELEASE_DIR"

TEMP_DIR="temp_package"
mkdir -p "$TEMP_DIR/DEBIAN"
mkdir -p "$TEMP_DIR/usr/bin"
mkdir -p "$TEMP_DIR/usr/lib"
mkdir -p "$TEMP_DIR/etc"

# Copy the built binaries and libraries
echo "Copying files to package..."
cp httppostclient/httppostclient "$TEMP_DIR/usr/bin/"
cp logservice/logservice "$TEMP_DIR/usr/bin/"

# Step 2: Check all library dependencies for the binaries
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
echo "Copying libraries to package..."
for lib in $all_deps; do
    if [ -f "$lib" ]; then
        echo "Copying library: $lib"
        cp "$lib" "$TEMP_DIR/usr/lib/"
    else
        echo "Library not found: $lib"
    fi
done
# Create control file
cat <<EOF > "$TEMP_DIR/DEBIAN/control"
Package: hansol-inticube-poc
Version: 1.0
Section: base
Priority: optional
Architecture: all
Depends: libc6 (>= 2.34)
Maintainer: Your Name <you@example.com>
Description: A brief description of the package
 This is a longer description of the package.
EOF

# Build the .deb package
echo "Building the .deb package..."
dpkg-deb --build "$TEMP_DIR" "$RELEASE_DIR/hansol_inticube_poc_packet.deb"

# Clean up
echo "Cleaning up..."
rm -rf "$TEMP_DIR"

echo "Release package created at $RELEASE_DIR/hansol_inticube_poc_packet.deb"
