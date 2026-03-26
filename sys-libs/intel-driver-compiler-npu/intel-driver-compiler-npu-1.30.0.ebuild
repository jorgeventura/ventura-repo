EAPI=8

inherit cmake git-r3

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
	append-cflags "-fcf-protection=none"
	append-cxxflags "-fcf-protection=none"

	local mycmakeargs=(
	    -DCMAKE_BUILD_TYPE=Release
	    -DENABLE_NPU_COMPILER_BUILD=ON
	    -DBUILD_TESTING=$(usex test ON OFF)
	    -DCMAKE_POLICY_VERSION_MINIMUM=3.5
	)
	cmake_src_configure
}


src_compile() {
	# 1. First, configure to create the build structure
	cmake_src_configure

	# 2. Trigger the JIT downloads
	# These targets create the nested directories
	cmake_src_make npu_compiler_source
	cmake_src_make npu_compiler_openvino_source
	cmake_src_make openvino_source

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
	eapply "${FILESDIR}/intel-driver-compiler-npu-1.30.0-npu-gtest-00.patch"
	popd > /dev/null

	# Path 01: Direct OpenVino and Compiler GTest
	local gtest_dirs=(
	    "compiler/src/openvino/thirdparty/gtest/gtest"
	    "compiler/src/npu_compiler_openvino/thirdparty/gtest/gtest"
	)
	for d in "${gtest_dirs[@]}"; do
	    pushd "${BUILD_DIR}/${d}" > /dev/null || die
	    eapply "${FILESDIR}/intel-driver-compiler-npu-1.30.0-npu-gtest-01.patch"
	    popd > /dev/null
	done

	# 4. Final parallel build
	cmake_src_compile
}

src_install() {
	cmake_src_install

	# This package should ship only the compiler library.
	rm -f \
		"${ED}"/usr/lib*/libze_loader.so* \
		"${ED}"/usr/lib*/libze_tracing_layer.so* \
		"${ED}"/usr/lib*/libze_validation_layer.so* \
		"${ED}"/usr/lib*/libze_intel_npu.so* \
		"${ED}"/usr/lib*/libze_intel_vpu.so* \
		"${ED}"/usr/lib*/libtbbmalloc*.so* \
		"${ED}"/usr/lib*/libtbbbind*.so*

	# Remove test/validation binaries from the image. They are useful in-tree but not
	# required as runtime payload for the compiler package.
	rm -f \
		"${ED}"/usr/bin/npu-umd-test \
		"${ED}"/usr/bin/npu-kmd-test

	# Remove firmware payload if upstream installed it as part of the full install.
	rm -rf \
		"${ED}"/lib/firmware/intel/vpu \
		"${ED}"/usr/lib/firmware/intel/vpu

	# Fail the install if the compiler library did not make it into the image.
	shopt -s nullglob
	local compiler_libs=( "${ED}"/usr/lib*/libnpu_driver_compiler.so* )
	shopt -u nullglob
	[[ ${#compiler_libs[@]} -gt 0 ]] || die "libnpu_driver_compiler.so was not installed"

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
