================================================================================
PROCEDURE: INTEL NPU COMPILER MANUAL BUILD AND GENTOO BINARY PACKAGING
================================================================================
Target Hardware: Intel Core Ultra 5 (Meteor Lake) - Lenovo Yoga
Build Hardware:  riscv-devel (or any x86_64 build host)
Version:         v1.30.0
Date:            March 28, 2026
--------------------------------------------------------------------------------

1. BUILD PROCEDURE
------------------
This section describes the compilation and patching of the NPU compiler source.
Assumption: All patch files are located one level above the 'linux-npu-driver' 
cloned directory.

# Clone and setup
git clone https://github.com/intel/linux-npu-driver.git
cd linux-npu-driver
git checkout v1.30.0
git submodule update --init --recursive

# Initial Configuration
cmake -B build -S . \
    -DCMAKE_BUILD_TYPE=Release \
    -DENABLE_NPU_COMPILER_BUILD=ON \
    -DBUILD_TESTING=OFF \
    -DCMAKE_C_FLAGS="-fcf-protection=none" \
    -DCMAKE_CXX_FLAGS="-fcf-protection=none" \
    -DCMAKE_POLICY_VERSION_MINIMUM=3.5

# Extract intermediary source files (JIT generation)
cmake --build build --target npu_compiler_source
cmake --build build --target npu_compiler_openvino_source

# Apply Patches to extracted sources
pushd build/compiler/src/npu_compiler
patch -p1 < ../../../../../intel-driver-compiler-npu-1.30.0-npu-compiler-fixes.patch
popd

pushd build/compiler/src/openvino/thirdparty/protobuf/protobuf/third_party/googletest
patch -p1 < ../../../../../../../../../../intel-driver-compiler-npu-1.30.0-npu-gtest-00.patch
popd

pushd build/compiler/src/openvino/thirdparty/gtest/gtest
patch -p1 < ../../../../../../../../intel-driver-compiler-npu-1.30.0-npu-gtest-01.patch
popd

pushd build/compiler/src/npu_compiler_openvino/thirdparty/gtest/gtest
patch -p1 < ../../../../../../../../intel-driver-compiler-npu-1.30.0-npu-gtest-01.patch
popd

# Final Compilation
cmake --build build --parallel $(nproc)


2. STAGING THE PACKAGE
----------------------
Create a clean directory structure to hold the files before compression.

mkdir -p ~/npu-bin-pkg/usr/lib64
mkdir -p ~/npu-bin-pkg/usr/bin

# Copy the Compiler Library (Found in the specific Release sub-directory)
cp ./build/compiler/build_npu_compiler/bin/intel64/Release/libnpu_driver_compiler.so ~/npu-bin-pkg/usr/lib64/

# Copy the Diagnostic/Test Binaries (Optional, for USE=test)
cd ./build/compiler/build_npu_compiler/bin/intel64/Release/
cp compilerTest loaderTest profilingTest vpuxCompilerL0Test ~/npu-bin-pkg/usr/bin/


3. FIXING THE LIBTBB / RPATH ISSUE
----------------------------------
The build system bakes a local 'home' path into the library. This must be removed
to ensure it uses the system dev-cpp/tbb library on the Yoga.

# Navigate to staging library
cd ~/npu-bin-pkg/usr/lib64/

# Remove the hardcoded RPATH (using patchelf)
patchelf --remove-rpath libnpu_driver_compiler.so

# Verify: 'libtbb.so.12' should no longer point to a /home/ path
ldd libnpu_driver_compiler.so | grep tbb


4. PACKAGING AND GENTOO INTEGRATION
-----------------------------------
Final steps to make the binary available to Portage.

# Create the compressed tarball
cd ~/npu-bin-pkg
tar -cvJf intel-driver-compiler-npu-1.30.0-bin.tar.xz .

# Move to Portage Distfiles (on both build host and Yoga)
sudo cp intel-driver-compiler-npu-1.30.0-bin.tar.xz /var/cache/distfiles/

# Update the Manifest (Run inside your localrepo)
cd /var/db/repos/localrepo/sys-libs/intel-driver-compiler-npu/
ebuild intel-driver-compiler-npu-1.30.0.ebuild manifest


5. IMPORTANT NOTES & DEPENDENCIES
---------------------------------
To ensure the compiler runs on the Yoga, the ebuild MUST include the 
following RDEPEND:

RDEPEND="
    dev-cpp/tbb
    sys-libs/intel-level-zero-npu
    sys-libs/zlib
    app-arch/zstd
"

- The package 'sys-libs/intel-level-zero-npu' is required because the compiler 
  works in tandem with the Level Zero driver.
- The NPU is specifically optimized for INT4 models (OpenVINO IR format).
- Use 'Environment="OPENVINO_DEVICE=NPU"' in your systemd service to trigger 
  the use of this library.

================================================================================
