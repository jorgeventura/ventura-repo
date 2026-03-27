EAPI=8

inherit cmake git-r3 flag-o-matic

DESCRIPTION="Intel NPU compiler userspace library (libnpu_driver_compiler)"
HOMEPAGE="https://github.com/intel/linux-npu-driver"
EGIT_REPO_URI="https://github.com/intel/linux-npu-driver.git"
EGIT_COMMIT="v${PV}"
EGIT_SUBMODULES=( '*' )

LICENSE="MIT"
SLOT="0"
KEYWORDS="amd64"
IUSE="test"
RESTRICT="network-sandbox !test? ( test )"

# Local-overlay ebuild for a fixed upstream tag.
# Upstream ships the compiler as a separate package named intel-driver-compiler-npu,
# but it is built from the same source tree as the userspace NPU driver.
#
# This ebuild intentionally keeps only libnpu_driver_compiler.so* from the install
# image and removes the Level Zero loader, NPU driver library, firmware blobs, and
# test binaries to avoid file collisions with other packages.
#
# Upstream documents that compiler build requires OpenVINO runtime. Depending on how
# OpenVINO is installed on the system, CMake may need it available in a standard
# prefix or via CMAKE_PREFIX_PATH/OpenVINO_DIR.

RDEPEND="
	=sys-libs/intel-level-zero-npu-${PV}
	dev-cpp/tbb:=
"
DEPEND="${RDEPEND}"
BDEPEND="
	dev-vcs/git-lfs
	virtual/pkgconfig
	test? ( dev-cpp/gtest )
"

src_configure() {
	# Append the GCC 15 fix to system flags
	local mycmakeargs=(
		-DCMAKE_BUILD_TYPE=Release
		-DENABLE_NPU_COMPILER_BUILD=ON
		-DCMAKE_C_FLAGS="-fcf-protection=none"
		-DCMAKE_CXX_FLAGS="-fcf-protection=none"
		-DBUILD_TESTING=$(usex test ON OFF)
		-DCMAKE_POLICY_VERSION_MINIMUM=3.5
	)

	cmake_src_configure
}


src_compile() {
	# 2. Trigger the JIT downloads
	# These targets create the nested directories
	cmake_build npu_compiler_source
	cmake_build npu_compiler_openvino_source
	cmake_build openvino_source

	# 3. Apply patches from the 'files' directory
	einfo "Patching NPU Compiler..."
	pushd "${BUILD_DIR}/compiler/src/npu_compiler" > /dev/null || die
	if ! patch -p1 -R --dry-run < "${FILESDIR}/intel-driver-compiler-npu-1.30.0-npu-compiler-fixes.patch" > /dev/null 2>&1; then
	    eapply "${FILESDIR}/intel-driver-compiler-npu-1.30.0-npu-compiler-fixes.patch"
	fi
	popd > /dev/null

	einfo "Patching nested GTest dependencies..."
	# Path 00: Protobuf GTest
	pushd "${BUILD_DIR}/compiler/src/openvino/thirdparty/protobuf/protobuf/third_party/googletest" > /dev/null || die
	if ! patch -p1 -R --dry-run < "${FILESDIR}/intel-driver-compiler-npu-1.30.0-npu-gtest-00.patch" > /dev/null 2>&1; then
	    eapply "${FILESDIR}/intel-driver-compiler-npu-1.30.0-npu-gtest-00.patch"
	fi
	popd > /dev/null

	# Path 01,02: Direct OpenVino and Compiler GTest
	local gtest_dirs=(
	    "compiler/src/openvino/thirdparty/gtest/gtest"
	    "compiler/src/npu_compiler_openvino/thirdparty/gtest/gtest"
	)
	for d in "${gtest_dirs[@]}"; do
	    pushd "${BUILD_DIR}/${d}" > /dev/null || die
	    if ! patch -p1 -R --dry-run < "${FILESDIR}/intel-driver-compiler-npu-1.30.0-npu-gtest-01.patch" > /dev/null 2>&1; then
	        eapply "${FILESDIR}/intel-driver-compiler-npu-1.30.0-npu-gtest-01.patch"
		fi
	    popd > /dev/null
	done

	einfo "Patching nested vpucostmodel dependencies..."
	# Patch 03: vpucostmodel
	pushd "${BUILD_DIR}/compiler/src/npu_compiler/thirdparty/vpucostmodel" > /dev/null || die
	if ! patch -p1 -R --dry-run < "${FILESDIR}/intel-driver-compiler-npu-1.30.0-npu-vpucostmodel.patch" > /dev/null 2>&1; then
	    eapply "${FILESDIR}/intel-driver-compiler-npu-1.30.0-npu-vpucostmodel.patch"
	fi
	popd > /dev/null

	cmake_src_compile
}


src_install() {
	# 1. Standard CMake installation to the image directory ${D}
	cmake_src_install

	# 2. Define the list of files that belong to other packages (collisions)
	# These are usually provided by sys-libs/intel-level-zero-npu or the loader.
	local to_remove=(
		"/usr/lib*/libze_loader.so*"
		"/usr/lib*/libze_tracing_layer.so*"
		"/usr/lib*/libze_validation_layer.so*"
		"/usr/lib*/libze_intel_npu.so*"
		"/usr/lib*/libze_intel_vpu.so*"
		"/usr/lib*/libtbbmalloc*.so*"
		"/usr/lib*/libtbbbind*.so*"
		"/usr/bin/npu-umd-test"
		"/usr/bin/npu-kmd-test"
	)

	einfo "Cleaning up files to avoid collisions with the base driver..."
	local item
	for item in "${to_remove[@]}"; do
		# Use rm -f to avoid errors if some targets weren't built
		rm -rf "${ED}"${item} || die "Failed to remove colliding file: ${item}"
	done

	# 3. Remove firmware blobs
	# Firmware is managed by the kernel/driver package, not the compiler library.
	if [[ -d "${ED}/lib/firmware" || -d "${ED}/usr/lib/firmware" ]]; then
		einfo "Removing firmware payload..."
		rm -rf "${ED}"/lib/firmware "${ED}"/usr/lib/firmware || die
	fi

	# 4. Critical Library Verification
	# If the compiler .so is missing, the build effectively failed.
	# We use nullglob to handle cases where the lib is in /usr/lib or /usr/lib64
	shopt -s nullglob
	local compiler_libs=( "${ED}"/usr/lib*/libnpu_driver_compiler.so* )
	shopt -u nullglob

	if [[ ${#compiler_libs[@]} -eq 0 ]]; then
		die "Failure: libnpu_driver_compiler.so was not found in the install image!"
	fi

	# 5. Documentation
	dodoc README.md docs/overview.md
}


src_test() {
	cmake_src_test
}

pkg_postinst() {
	einfo "Installed Intel NPU compiler library: libnpu_driver_compiler.so"
	einfo "This package complements intel-level-zero-npu and is typically needed"
	einfo "for compiling/executing models on the NPU through OpenVINO."
	einfo ""
	einfo "Quick verification:"
	einfo "  ldconfig -p | grep libnpu_driver_compiler"
	einfo ""
	einfo "If model compilation fails, ensure your OpenVINO runtime used for build"
	einfo "and the installed runtime on the system are compatible with this driver tag."
}
