EAPI=8

DESCRIPTION="Binary Intel NPU Driver Compiler (Manual Build for Yoga)"
HOMEPAGE="https://github.com/intel/linux-npu-driver"

# Point to your manual tarball
SRC_URI="intel-driver-compiler-npu-1.30.0-bin.tar.xz"

LICENSE="GPL-2"
SLOT="0"
KEYWORDS="amd64"
IUSE="test"

# Tell Portage these are pre-compiled to avoid QA warnings
QA_PREBUILT="usr/lib64/libnpu_driver_compiler.so
	usr/bin/compilerTest
	usr/bin/loaderTest
	usr/bin/profilingTest
	usr/bin/vpuxCompilerL0Test"

# Runtime dependencies on the Yoga
RDEPEND="
	dev-cpp/tbb
	sys-libs/intel-level-zero-npu
	sys-libs/zlib
	app-arch/zstd
"

# No build-time dependencies needed for a binary move
DEPEND=""

S="${WORKDIR}"

# Portage will not be able to download this from the web
RESTRICT="fetch"

pkg_nofetch() {
	einfo "Please ensure that 'intel-driver-compiler-npu-1.30.0-bin.tar.xz'"
	einfo "is present in your /var/cache/distfiles/ directory."
	einfo "This tarball should contain the usr/lib64 and usr/bin structures"
	einfo "created during your manual build on the riscv-devel machine."
}

src_unpack() {
	# Standard extraction of the tarball
	default
}

src_install() {
	# 1. Install the compiler library to the system lib64
	# 'insinto' handles the destination, 'doins' copies the file
	insinto /usr/lib64
	doins usr/lib64/libnpu_driver_compiler.so

	# 2. Install test binaries only if USE=test is enabled
	if use test; then
		einfo "Installing NPU compiler diagnostic tools..."
		dobin usr/bin/compilerTest
		dobin usr/bin/loaderTest
		dobin usr/bin/profilingTest
		dobin usr/bin/vpuxCompilerL0Test
	fi
}
