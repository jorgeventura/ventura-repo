EAPI=8

inherit cmake git-r3

DESCRIPTION="Intel NPU compiler userspace library (libnpu_driver_compiler)"
HOMEPAGE="https://github.com/intel/linux-npu-driver"

EGIT_REPO_URI="https://github.com/intel/linux-npu-driver.git"
EGIT_COMMIT="v${PV}"
EGIT_SUBMODULES=( '*' )

# Revisions pinned by upstream in compiler/compiler_source.cmake for v1.30.0
OPENVINO_REVISION="77501947b70f6c7996f07c068f2067e868e356ed"
NPU_COMPILER_OPENVINO_REVISION="4922c4955f9d5c457cf9d4ebbbc8bf6502167ada"
NPU_COMPILER_REVISION="e0af53718947347b12ffcb7c8b7499ac7f2e93bf"
NPU_COMPILER_TAG="npu_ud_2026_08_rc2"

SRC_URI="
	https://github.com/openvinotoolkit/openvino/archive/${OPENVINO_REVISION}.tar.gz
		-> openvino-${OPENVINO_REVISION}.tar.gz
	https://github.com/openvinotoolkit/openvino/archive/${NPU_COMPILER_OPENVINO_REVISION}.tar.gz
		-> openvino-${NPU_COMPILER_OPENVINO_REVISION}.tar.gz
	https://github.com/openvinotoolkit/npu_compiler/archive/${NPU_COMPILER_REVISION}.tar.gz
		-> npu_compiler-${NPU_COMPILER_REVISION}.tar.gz
"

LICENSE="MIT"
SLOT="0"
KEYWORDS="amd64"

IUSE="test"
RESTRICT="network-sandbox !test? ( test )"

RDEPEND="
	=sys-libs/intel-level-zero-npu-${PV}
	dev-cpp/tbb:=
"
DEPEND="${RDEPEND}"
BDEPEND="
	virtual/pkgconfig
"

DOCS=( README.md docs/overview.md )

src_unpack() {
	git-r3_src_unpack
	unpack ${A}
}

src_prepare() {
	# Replace upstream Git-fetching ExternalProject logic with local, pre-fetched
	# source trees so Portage does not need network access during compile.
	cat > compiler/compiler_source.cmake <<EOF || die
# Copyright (C) 2022-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT

if(TARGET npu_compiler_source)
	return()
endif()

if(DEFINED ENV{TARGET_DISTRO})
	set(TARGET_DISTRO \$ENV{TARGET_DISTRO})
else()
	set(TARGET_DISTRO \${CMAKE_SYSTEM_NAME})
endif()

include(ExternalProject)

set(OPENVINO_REPOSITORY https://github.com/openvinotoolkit/openvino.git)
set(OPENVINO_REVISION ${OPENVINO_REVISION})
set(NPU_COMPILER_TAG ${NPU_COMPILER_TAG})
set(NPU_COMPILER_REVISION ${NPU_COMPILER_REVISION})
set(NPU_COMPILER_OPENVINO_REVISION ${NPU_COMPILER_OPENVINO_REVISION})

set(OPENVINO_SOURCE_DIR "${WORKDIR}/openvino-${OPENVINO_REVISION}")
set(NPU_COMPILER_SOURCE_DIR "${WORKDIR}/npu_compiler-${NPU_COMPILER_REVISION}")

if(NOT NPU_COMPILER_OPENVINO_REVISION STREQUAL OPENVINO_REVISION)
	set(NPU_COMPILER_OPENVINO_SOURCE_DIR "${WORKDIR}/openvino-${NPU_COMPILER_OPENVINO_REVISION}")
	set(NPU_COMPILER_BUILD_DEPENDS npu_compiler_openvino_source)
	add_custom_target(npu_compiler_openvino_source)
else()
	set(NPU_COMPILER_OPENVINO_SOURCE_DIR \${OPENVINO_SOURCE_DIR})
	set(NPU_COMPILER_BUILD_DEPENDS openvino_source)
endif()

add_custom_target(openvino_source)
add_custom_target(npu_compiler_source)
EOF

	cmake_src_prepare
}

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
		"${ED}"/usr/lib*/libtbbbind*.so* \
		"${ED}"/usr/lib*/libopenvino*.so* \
		"${ED}"/usr/lib*/libonnxruntime*.so*

	# Remove test/validation binaries from the image.
	rm -f \
		"${ED}"/usr/bin/npu-umd-test \
		"${ED}"/usr/bin/npu-kmd-test

	# Remove firmware payload if upstream installed it as part of the full install.
	rm -rf \
		"${ED}"/lib/firmware/intel/vpu \
		"${ED}"/usr/lib/firmware/intel/vpu \
		"${ED}"/usr/share/npu_compiler \
		"${ED}"/usr/include

	# Fail the install if the compiler library did not make it into the image.
	compgen -G "${ED}/usr/lib*/libnpu_driver_compiler.so*" > /dev/null \
		|| die "libnpu_driver_compiler.so was not installed"

	einstalldocs
}

src_test() {
	cmake_src_test
}

pkg_postinst() {
	einfo "Installed Intel NPU compiler library: libnpu_driver_compiler.so"
	einfo "Quick verification:"
	einfo "  ldconfig -p | grep libnpu_driver_compiler"
}
