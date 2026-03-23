Suggested local overlay layout:

  /var/db/repos/localrepo/sys-libs/intel-level-zero-npu/
    metadata.xml
    intel-level-zero-npu-1.30.0.ebuild

Then run:

  ebuild /var/db/repos/localrepo/sys-libs/intel-level-zero-npu/intel-level-zero-npu-1.30.0.ebuild manifest
  emaint sync -r localrepo
  emerge -av =sys-libs/intel-level-zero-npu-1.30.0

Notes:
- This ebuild uses git-r3 with EGIT_COMMIT="v1.30.0" so it can fetch a fixed tag
  and all submodules.
- It intentionally disables ENABLE_NPU_COMPILER_BUILD. That compiler piece should
  be packaged separately as intel-driver-compiler-npu.
- It removes libze_loader and optional Level Zero layers from the image to avoid
  colliding with sys-libs/level-zero.
