Package layout
==============

Place these files in your overlay:

  /var/db/repos/localrepo/sys-libs/intel-driver-compiler-npu/
    metadata.xml
    intel-driver-compiler-npu-1.30.0.ebuild

Commands
========

  cp /mnt/data/intel-driver-compiler-npu-1.30.0.ebuild \
    /var/db/repos/localrepo/sys-libs/intel-driver-compiler-npu/
  cp /mnt/data/intel-driver-compiler-metadata.xml \
    /var/db/repos/localrepo/sys-libs/intel-driver-compiler-npu/metadata.xml

  ebuild /var/db/repos/localrepo/sys-libs/intel-driver-compiler-npu/intel-driver-compiler-npu-1.30.0.ebuild manifest
  emerge -av --buildpkgonly =sys-libs/intel-driver-compiler-npu-1.30.0

Notes
=====

1. This ebuild builds from the same linux-npu-driver repository/tag as the
   intel-level-zero-npu package but keeps only libnpu_driver_compiler.so*.

2. Upstream states the compiler build requires OpenVINO runtime. If CMake
   cannot locate OpenVINO in your Gentoo setup, you may need to expose it via
   OpenVINO_DIR or CMAKE_PREFIX_PATH before running emerge.

3. This package depends on =sys-libs/intel-level-zero-npu-${PV} and dev-cpp/tbb.

4. USE=test is optional. Tests are disabled by default.
