# Copyright 2026 Gentoo Authors
# Distributed under the terms of the GNU General Public License v2

# Copyright 2026 George Ventura
# Distributed under the terms of the MIT License

EAPI=8

inherit cmake git-r3 flag-o-matic

DESCRIPTION="Intel User-Mode Driver for NPU (Level Zero implementation)"
HOMEPAGE="https://github.com/intel/linux-npu-driver"
EGIT_REPO_URI="https://github.com/intel/linux-npu-driver.git"

LICENSE="MIT"
SLOT="0"
IUSE="+compiler"

# Dependencies
# We specifically need gcc:12 to avoid ABI/Warning issues
BDEPEND="
	dev-vcs/git-lfs
	>=dev-build/cmake-3.5
	sys-devel/gcc:12
"
DEPEND="
	>=dev-libs/level-zero-1.17.6
	dev-cpp/tbb
"
RDEPEND="${DEPEND}"

src_unpack() {
	git-r3_src_unpack
	cd "${S}" || die
	# Pull the LFS binaries (Firmware/Compiler artifacts)
	git lfs install --local || die
	git lfs pull || die
}

src_configure() {
	# 1. Force GCC 12 to avoid the stringop-overflow and ABI mismatch
	export CC="gcc-12"
	export CXX="g++-12"

	# 2. Add flags to prevent warnings from becoming fatal errors
	append-cxxflags -Wno-error=stringop-overflow -Wno-error=maybe-uninitialized -Wno-error=array-bounds

	# 3. Handle ABI mismatch for system libraries (OpenCV, etc)
	# This helps when the system was built with a newer GCC ABI
	append-cxxflags -D_GLIBCXX_USE_CXX11_ABI=1

	local mycmakeargs=(
	    -DENABLE_NPU_COMPILER_BUILD=$(usex compiler ON OFF)
	    -DCMAKE_BUILD_TYPE=Release
	    -DCMAKE_POLICY_VERSION_MINIMUM=3.5
	)
	cmake_src_configure
}

src_install() {
	cmake_src_install
}
