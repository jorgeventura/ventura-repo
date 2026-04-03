````md
# Ollama 0.20.0 bump in local Gentoo repo

This document records the steps used to add `sci-ml/ollama-0.20.0` to a local Gentoo repository cloned from GURU.

The goal is to keep the original package layout from GURU, add a new `0.20.0` ebuild in the local repo, verify that it builds, and then install it cleanly through Portage.

The local repo layout used here is:

```text
acct-group/
└── ollama
acct-user/
└── ollama
sci-ml/
└── ollama
````

The `acct-group/ollama` and `acct-user/ollama` ebuilds are kept as-is, while `sci-ml/ollama` is extended with a new `ollama-0.20.0.ebuild`. The existing `0.18.0` ebuild already provides the correct overall structure for fetching upstream source, using the Go module tarball, applying local patches, and depending on the `acct-group` and `acct-user` packages, so it is a good base for the version bump.  

---

## 1. Starting point

Inside the local repository, the package tree looked like this:

```text
acct-group/
└── ollama
    ├── metadata.xml
    └── ollama-0.ebuild

acct-user/
└── ollama
    ├── metadata.xml
    └── ollama-3.ebuild

sci-ml/
└── ollama
    ├── files
    │   ├── ollama-0.18.0-make-installing-runtime-deps-optional.patch
    │   ├── ollama-9999-make-installing-runtime-deps-optional.patch
    │   ├── ollama-9999-use-GNUInstallDirs.patch
    │   ├── ollama.confd
    │   ├── ollama.init
    │   └── ollama.service
    ├── Manifest
    ├── metadata.xml
    ├── ollama-0.17.7.ebuild
    ├── ollama-0.18.0.ebuild
    └── ollama-9999.ebuild
```

The package to use as the template was `sci-ml/ollama/ollama-0.18.0.ebuild`. That ebuild already contains the package logic for CUDA, ROCm, Vulkan, BLAS selection, install paths, service files, and user/group dependencies.  

---

## 2. Create the new ebuild

Copy the existing `0.18.0` ebuild to `0.20.0`:

```bash
cd /var/db/repos/localrepo/sci-ml/ollama

cp ollama-0.18.0.ebuild ollama-0.20.0.ebuild
```

Then update the patch reference in the new ebuild so it points to a `0.20.0`-specific runtime-deps patch:

```bash
sed -i \
  's/ollama-0.18.0-make-installing-runtime-deps-optional.patch/ollama-0.20.0-make-installing-runtime-deps-optional.patch/' \
  ollama-0.20.0.ebuild
```

The original `0.18.0` ebuild uses this patch set:

```bash
PATCHES=(
    "${FILESDIR}/${PN}-9999-use-GNUInstallDirs.patch"
    "${FILESDIR}/${PN}-0.18.0-make-installing-runtime-deps-optional.patch"
)
```

So the only intended change at first is the versioned runtime-deps patch filename. 

---

## 3. Create the new patch file

Copy the old versioned patch as the starting point:

```bash
cd /var/db/repos/localrepo/sci-ml/ollama/files

cp ollama-0.18.0-make-installing-runtime-deps-optional.patch \
   ollama-0.20.0-make-installing-runtime-deps-optional.patch
```

At this stage, the idea is not to redesign the package. The idea is to carry forward the same packaging logic and only adjust what is necessary for `0.20.0`.

---

## 4. Generate the Manifest

Back in the package directory:

```bash
cd /var/db/repos/localrepo/sci-ml/ollama
ebuild ollama-0.20.0.ebuild manifest
```

This step fetches the distfiles declared in `SRC_URI` and updates the `Manifest`.

The `0.18.0` ebuild fetches two important files for stable releases:

* the upstream GitHub release tarball
* the Gentoo Go dependency tarball

That structure is inherited unchanged by the `0.20.0` ebuild. 

---

## 5. Test the preparation phase

Run:

```bash
ebuild ollama-0.20.0.ebuild clean prepare
```

This checks that unpacking, patch application, and source tree edits all work.

The `src_prepare()` phase in the inherited ebuild does several important things:

* disables CMake CCACHE
* removes `-O3` from a Go source file
* rewrites library installation paths to use `$(get_libdir)`
* disables CPU backend variants that are not supported by selected `CPU_FLAGS_X86`
* runs `cuda_src_prepare` if CUDA is enabled
* strips problematic `-Werror` usage from Go files for ROCm builds

Those behaviors come directly from the original `0.18.0` ebuild and were preserved in the `0.20.0` bump. 

If `prepare` fails, the usual causes are:

* a patch no longer applies cleanly
* upstream changed file paths
* a local sed target moved or was renamed

If `prepare` succeeds, the bump is usually on the right track.

---

## 6. Test compilation

Run:

```bash
ebuild ollama-0.20.0.ebuild compile
```

This validates both the Go build and the CMake build.

The ebuild’s `src_compile()` exports version information into the resulting binary using Go linker flags, then runs both `ego build` and `cmake_src_compile`. That behavior was preserved unchanged for the `0.20.0` bump. 

If `compile` succeeds, the major version bump work is effectively proven.

---

## 7. Test installation into the image tree

After a successful compile:

```bash
ebuild ollama-0.20.0.ebuild install
```

This does not yet merge the package into the live filesystem. It performs the install phase into the temporary image area and validates that the install layout is correct.

The ebuild’s `src_install()` does the following:

* installs the `ollama` binary with `dobin`
* runs `cmake_src_install`
* installs the OpenRC init file
* installs the OpenRC confd file
* installs the systemd unit

It also prepares `/var/log/ollama` ownership and permissions in `pkg_preinst()`.  

If `install` fails, the issue is usually one of these:

* file layout mismatch
* changed install targets upstream
* runtime dependency installation behavior
* patch no longer correctly disabling an upstream install step

---

## 8. Merge the package

Once `install` succeeds, there are two normal ways to proceed.

### Option A: merge with ebuild directly

```bash
ebuild ollama-0.20.0.ebuild qmerge
```

### Option B: use Portage normally

```bash
emerge -av1 =sci-ml/ollama-0.20.0
```

The second option is the standard Gentoo workflow and is preferred once the ebuild is known to work.

---

## 9. Verify the result

After merge:

```bash
ollama --version
which ollama
```

If using the service:

```bash
systemctl status ollama.service
```

The ebuild also prints a post-install message showing a basic usage example and a note about CUDA requiring the runtime user to be in the `video` group. That message comes from `pkg_postinst()`. 

---

## 10. Notes about dependencies

The original package depends on:

* `acct-group/ollama`
* `acct-user/ollama-3[cuda?]`

and those were intentionally left unchanged for the `0.20.0` bump. 

This means there was no need to create a new `acct-user` or `acct-group` ebuild just for the Ollama version bump.

---

## 11. Notes about USE flags

The ebuild supports:

* `cuda`
* `rocm`
* `vulkan`
* `blas`
* `flexiblas`
* `blis`
* `mkl`
* `openblas`

and also dynamically extends `IUSE` with CPU feature flags for different optimized backends. That behavior was inherited directly from the original `0.18.0` ebuild and kept intact for `0.20.0`. 

No extra feature changes were required just to perform the version bump.

---

## 12. Practical workflow summary

The full workflow used for the bump is:

```bash
cd /var/db/repos/localrepo/sci-ml/ollama
cp ollama-0.18.0.ebuild ollama-0.20.0.ebuild

sed -i \
  's/ollama-0.18.0-make-installing-runtime-deps-optional.patch/ollama-0.20.0-make-installing-runtime-deps-optional.patch/' \
  ollama-0.20.0.ebuild

cd files
cp ollama-0.18.0-make-installing-runtime-deps-optional.patch \
   ollama-0.20.0-make-installing-runtime-deps-optional.patch

cd ..
ebuild ollama-0.20.0.ebuild manifest
ebuild ollama-0.20.0.ebuild clean prepare
ebuild ollama-0.20.0.ebuild compile
ebuild ollama-0.20.0.ebuild install
emerge -av1 =sci-ml/ollama-0.20.0
```

---

## 13. Outcome

The important checkpoint is that `compile` succeeded. Once that happens, the version bump is usually fundamentally correct. The remaining work is just validating install layout and then merging the package.

This makes the `0.20.0` bump a conservative extension of the existing GURU-derived package rather than a rewrite.

## 14. The full ebuild

```ebuild
# Copyright 2024-2026 Gentoo Authors
# Distributed under the terms of the GNU General Public License v2

EAPI=8

# supports ROCM/HIP >=5.5, but we define 6.1 due to the eclass
ROCM_VERSION="6.1"
inherit cuda rocm
inherit cmake
inherit flag-o-matic go-module linux-info systemd toolchain-funcs

DESCRIPTION="Get up and running with Llama 3, Mistral, Gemma, and other language models."
HOMEPAGE="https://ollama.com"

if [[ ${PV} == *9999* ]]; then
	inherit git-r3
	EGIT_REPO_URI="https://github.com/ollama/ollama.git"
else
	MY_PV="${PV/_rc/-rc}"
	MY_P="${PN}-${MY_PV}"
	SRC_URI="
		https://github.com/ollama/${PN}/archive/refs/tags/v${MY_PV}.tar.gz -> ${MY_P}.gh.tar.gz
		https://github.com/gentoo-golang-dist/${PN}/releases/download/v${MY_PV}/${MY_P}-deps.tar.xz
	"
	if [[ ${PV} != *_rc* ]]; then
		KEYWORDS="~amd64"
	fi
fi

LICENSE="MIT"
SLOT="0"

IUSE="cuda rocm vulkan"
# IUSE+=" opencl"

BLAS_BACKENDS="blis mkl openblas"
BLAS_REQUIRED_USE="blas? ( ?? ( ${BLAS_BACKENDS} ) )"

IUSE+=" blas flexiblas ${BLAS_BACKENDS}"
REQUIRED_USE+=" ${BLAS_REQUIRED_USE}"

declare -rgA CPU_FEATURES=(
	[AVX2]="x86"
	[AVX512F]="x86"
	[AVX512_VBMI]="x86;avx512vbmi"
	[AVX512_VNNI]="x86"
	[AVX]="x86"
	[AVX_VNNI]="x86"
	[BMI2]="x86"
	[F16C]="x86"
	[FMA]="x86;fma3"
	[SSE42]="x86;sse4_2"
)
add_cpu_features_use() {
	for flag in "${!CPU_FEATURES[@]}"; do
		IFS=$';' read -r arch use <<< "${CPU_FEATURES[${flag}]}"
		IUSE+=" cpu_flags_${arch}_${use:-${flag,,}}"
	done
}
add_cpu_features_use

RESTRICT="mirror test"

COMMON_DEPEND="
	blas? (
		blis? (
			sci-libs/blis:=
		)
		flexiblas? (
			sci-libs/flexiblas[blis?,mkl?,openblas?]
		)
		mkl? (
			sci-libs/mkl[llvm-openmp]
		)
		openblas? (
			sci-libs/openblas
		)
		virtual/blas[flexiblas=]
	)
	cuda? (
		dev-util/nvidia-cuda-toolkit:=
	)
	rocm? (
		>=dev-util/hip-${ROCM_VERSION}:=
		>=sci-libs/hipBLAS-${ROCM_VERSION}:=
		>=sci-libs/rocBLAS-${ROCM_VERSION}:=
	)
"

DEPEND="
	${COMMON_DEPEND}
	>=dev-lang/go-1.23.4
"
BDEPEND="
	vulkan? (
		dev-util/vulkan-headers
		media-libs/shaderc
	)
"

RDEPEND="
	${COMMON_DEPEND}
	acct-group/${PN}
	>=acct-user/${PN}-3[cuda?]
"

PATCHES=(
	"${FILESDIR}/${PN}-9999-use-GNUInstallDirs.patch"
	"${FILESDIR}/${PN}-0.20.0-make-installing-runtime-deps-optional.patch"
)

pkg_pretend() {
	if use amd64; then
		if use cpu_flags_x86_f16c && use cpu_flags_x86_avx2 && use cpu_flags_x86_fma3 && ! use cpu_flags_x86_bmi2; then
			ewarn
			ewarn "CPU_FLAGS_X86: bmi2 not enabled."
			ewarn "  Not building haswell runner."
			ewarn "  Not building skylakex runner."
			ewarn "  Not building icelake runner."
			ewarn "  Not building alderlake runner."
			ewarn
			if grep bmi2 /proc/cpuinfo > /dev/null; then
				ewarn "bmi2 found in /proc/cpuinfo"
				ewarn
			fi
		fi
	fi
}

pkg_setup() {
	if use rocm; then
		linux-info_pkg_setup
		if linux-info_get_any_version && linux_config_exists; then
			if ! linux_chkconfig_present HSA_AMD_SVM; then
				ewarn "To use ROCm/HIP, you need to have HSA_AMD_SVM option enabled in your kernel."
			fi
		fi
	fi
}

src_unpack() {
	if use rocm; then
		strip-unsupported-flags
		export CXXFLAGS="$(test-flags-HIPCXX "${CXXFLAGS}")"
	fi

	if [[ "${PV}" == *9999* ]]; then
		git-r3_src_unpack
		go-module_live_vendor
	else
		go-module_src_unpack
	fi
}

src_prepare() {
	cmake_src_prepare

	sed \
		-e "/set(GGML_CCACHE/s/ON/OFF/g" \
		-i CMakeLists.txt || die "Disable CCACHE sed failed"

	sed \
		-e "s/ -O3//g" \
		-i \
			ml/backend/ggml/ggml/src/ggml-cpu/cpu.go \
		|| die "-O3 sed failed"

	sed \
		-e "s/\"..\", \"lib\"/\"..\", \"$(get_libdir)\"/" \
		-e "s#\"lib/ollama\"#\"$(get_libdir)/ollama\"#" \
		-i \
			ml/backend/ggml/ggml/src/ggml.go \
			ml/path.go \
		|| die "libdir sed failed"

	if use amd64; then
		if ! use cpu_flags_x86_sse4_2; then
			sed -e "/ggml_add_cpu_backend_variant(sse42/s/^/# /g" -i ml/backend/ggml/ggml/src/CMakeLists.txt || die
		fi
		if ! use cpu_flags_x86_sse4_2 || ! use cpu_flags_x86_avx; then
			sed -e "/ggml_add_cpu_backend_variant(sandybridge/s/^/# /g" -i ml/backend/ggml/ggml/src/CMakeLists.txt || die
		fi
		if ! use cpu_flags_x86_sse4_2 || ! use cpu_flags_x86_avx || ! use cpu_flags_x86_f16c || ! use cpu_flags_x86_avx2 || ! use cpu_flags_x86_bmi2 || ! use cpu_flags_x86_fma3; then
			sed -e "/ggml_add_cpu_backend_variant(haswell/s/^/# /g" -i ml/backend/ggml/ggml/src/CMakeLists.txt || die
		fi
		if ! use cpu_flags_x86_sse4_2 || ! use cpu_flags_x86_avx || ! use cpu_flags_x86_f16c || ! use cpu_flags_x86_avx2 || ! use cpu_flags_x86_bmi2 || ! use cpu_flags_x86_fma3 || ! use cpu_flags_x86_avx512f; then
			sed -e "/ggml_add_cpu_backend_variant(skylakex/s/^/# /g" -i ml/backend/ggml/ggml/src/CMakeLists.txt || die
		fi
		if ! use cpu_flags_x86_sse4_2 || ! use cpu_flags_x86_avx || ! use cpu_flags_x86_f16c || ! use cpu_flags_x86_avx2 || ! use cpu_flags_x86_bmi2 || ! use cpu_flags_x86_fma3 || ! use cpu_flags_x86_avx512f || ! use cpu_flags_x86_avx512vbmi || ! use cpu_flags_x86_avx512_vnni; then
			sed -e "/ggml_add_cpu_backend_variant(icelake/s/^/# /g" -i ml/backend/ggml/ggml/src/CMakeLists.txt || die
		fi
		if ! use cpu_flags_x86_sse4_2 || ! use cpu_flags_x86_avx || ! use cpu_flags_x86_f16c || ! use cpu_flags_x86_avx2 || ! use cpu_flags_x86_bmi2 || ! use cpu_flags_x86_fma3 || ! use cpu_flags_x86_avx_vnni; then
			sed -e "/ggml_add_cpu_backend_variant(alderlake/s/^/# /g" -i ml/backend/ggml/ggml/src/CMakeLists.txt || die
		fi
	fi

	if use cuda; then
		cuda_src_prepare
	fi

	if use rocm; then
		find "${S}" -name ".go" -exec sed -i "s/ -Werror / /g" {} + || die
	fi
}

src_configure() {
	local mycmakeargs=(
		-DOLLAMA_INSTALL_RUNTIME_DEPS="no"
		-DGGML_CCACHE="no"
		-DGGML_BACKEND_DL="yes"
		-DGGML_BACKEND_DIR="${EPREFIX}/usr/$(get_libdir)/${PN}"
		-DGGML_BLAS="$(usex blas)"
		"$(cmake_use_find_package vulkan Vulkan)"
	)

	if tc-is-lto ; then
		mycmakeargs+=(
			-DGGML_LTO="yes"
		)
	fi

	if use blas; then
		if use flexiblas ; then
			mycmakeargs+=(
				-DGGML_BLAS_VENDOR="FlexiBLAS"
			)
		elif use blis ; then
			mycmakeargs+=(
				-DGGML_BLAS_VENDOR="FLAME"
			)
		elif use mkl ; then
			mycmakeargs+=(
				-DGGML_BLAS_VENDOR="Intel10_64lp"
			)
		elif use openblas ; then
			mycmakeargs+=(
				-DGGML_BLAS_VENDOR="OpenBLAS"
			)
		else
			mycmakeargs+=(
				-DGGML_BLAS_VENDOR="Generic"
			)
		fi
	fi

	if use cuda; then
		local -x CUDAHOSTCXX CUDAHOSTLD
		CUDAHOSTCXX="$(cuda_gccdir)"
		CUDAHOSTLD="$(tc-getCXX)"

		if [[ ! -v CUDAARCHS ]]; then
			local CUDAARCHS="all-major"
		fi

		mycmakeargs+=(
			-DCMAKE_CUDA_ARCHITECTURES="${CUDAARCHS}"
		)

		cuda_add_sandbox -w
		addpredict "/dev/char/"
	else
		mycmakeargs+=(
			-DCMAKE_CUDA_COMPILER="NOTFOUND"
		)
	fi

	if use rocm; then
		mycmakeargs+=(
			-DCMAKE_HIP_ARCHITECTURES="$(get_amdgpu_flags)"
			-DCMAKE_HIP_PLATFORM="amd"
			-DAMDGPU_TARGETS="$(get_amdgpu_flags)"
		)

		local -x HIP_PATH="${ESYSROOT}/usr"
	else
		mycmakeargs+=(
			-DCMAKE_HIP_COMPILER="NOTFOUND"
		)
	fi

	cmake_src_configure
}

src_compile() {
	local VERSION
	if [[ "${PV}" == *9999* ]]; then
		VERSION="$(
			git describe --tags --first-parent --abbrev=7 --long --dirty --always \
			| sed -e "s/^v//g"
		)"
	else
		VERSION="${PVR}"
	fi
	local EXTRA_GOFLAGS_LD=(
		"-X=github.com/ollama/ollama/version.Version=${VERSION}"
		"-X=github.com/ollama/ollama/server.mode=release"
	)
	GOFLAGS+=" '-ldflags=${EXTRA_GOFLAGS_LD[*]}'"

	ego build
	cmake_src_compile
}

src_install() {
	dobin ollama

	cmake_src_install

	newinitd "${FILESDIR}/ollama.init" "${PN}"
	newconfd "${FILESDIR}/ollama.confd" "${PN}"

	systemd_dounit "${FILESDIR}/ollama.service"
}

pkg_preinst() {
	keepdir /var/log/ollama
	fperms 750 /var/log/ollama
	fowners "${PN}:${PN}" /var/log/ollama
}

pkg_postinst() {
	if [[ -z ${REPLACING_VERSIONS} ]] ; then
		einfo "Quick guide:"
		einfo "\tollama serve"
		einfo "\tollama run llama3:70b"
		einfo
		einfo "See available models at https://ollama.com/library"
	fi

	if use cuda ; then
		einfo "When using cuda the user running ${PN} has to be in the video group or it won't detect devices."
		einfo "The ebuild ensures this for user ${PN} via acct-user/${PN}[cuda]"
	fi
}
```

```
