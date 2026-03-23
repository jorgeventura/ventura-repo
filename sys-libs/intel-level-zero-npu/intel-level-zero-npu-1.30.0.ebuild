EAPI=8

inherit cmake git-r3

DESCRIPTION="Intel NPU Level Zero userspace driver (libze_intel_npu)"
HOMEPAGE="https://github.com/intel/linux-npu-driver"
EGIT_REPO_URI="https://github.com/intel/linux-npu-driver.git"
EGIT_COMMIT="v${PV}"
EGIT_SUBMODULES=( '*' )

LICENSE="MIT"
SLOT="0"
KEYWORDS="amd64"
IUSE="test"
RESTRICT="!test? ( test )"

# This is intended for a local overlay.  Upstream builds from git with submodules.
# The compiler-in-driver component is packaged separately; this ebuild only installs
# the Level Zero NPU userspace driver.

RDEPEND="
	dev-libs/level-zero
	virtual/libudev
"
DEPEND="${RDEPEND}"
BDEPEND="
	dev-vcs/git-lfs
	dev-cpp/tbb
	virtual/pkgconfig
	test? (
		dev-cpp/gtest
	)
"

src_configure() {
	local mycmakeargs=(
		-DCMAKE_BUILD_TYPE=Release
		-DENABLE_NPU_COMPILER_BUILD=OFF
		-DCMAKE_C_FLAGS=-fcf-protection=none
		-DCMAKE_CXX_FLAGS=-fcf-protection=none

		# main point: tests follow USE=test
		-DBUILD_TESTING=$(usex test ON OFF)

		# keep compiler component out for now
		-DENABLE_NPU_COMPILER_BUILD=OFF

		-DCMAKE_POLICY_VERSION_MINIMUM=3.5
	)

	# Upstream builds a lot of tests by default. If the project honors BUILD_TESTING,
	# let the user decide; otherwise this is harmless.
	mycmakeargs+=( -DBUILD_TESTING=$(usex test ON OFF) )

	cmake_src_configure
}

src_install() {
	cmake_src_install

	# This package is meant to provide the Intel NPU Level Zero driver library.
	# Do not overwrite the system Level Zero loader package if upstream happens to
	# install the loader or optional layers from bundled submodules.
	rm -f \
		"${ED}"/usr/lib*/libze_loader.so* \
		"${ED}"/usr/lib*/libze_tracing_layer.so* \
		"${ED}"/usr/lib*/libze_validation_layer.so*

	# Keep only the NPU userspace runtime from this package.
	dodoc README.md docs/overview.md
}

pkg_postinst() {
	einfo "The Intel NPU kernel module is provided by the kernel as intel_vpu."
	einfo "After first install, reload it so the runtime and firmware are picked up:"
	einfo "  modprobe -r intel_vpu && modprobe intel_vpu"
	einfo ""
	einfo "Then verify detection with:"
	einfo "  ldconfig -p | grep libze_intel_npu"
	einfo "  python3 -c 'from openvino import Core; print(Core().available_devices)'"
	einfo ""
	einfo "Your user also needs access to /dev/accel/accel0, usually via the render group."
}
