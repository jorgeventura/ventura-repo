# Copyright 1999-2025 Gentoo Authors
# Distributed under the terms of the GNU General Public License v2

EAPI=8

#inherit go-module systemd git-r3
inherit go-module systemd

DESCRIPTION="Kubernetes Node Agent"
HOMEPAGE="https://kubernetes.io"
SRC_URI="https://github.com/kubernetes/kubernetes/archive/v${PV}.tar.gz -> kubernetes-${PV}.tar.gz"

# EGIT_COMMIT=4b8e819355d791d96b7e9d9efe4cbafae2311c88
# EGIT_REPO_URI="https://github.com/kubernetes/kubernetes"
# EGIT_BRANCH="release-1.29"

LICENSE="Apache-2.0"
SLOT="0"
KEYWORDS="amd64 ~arm64"
IUSE="hardened selinux"

BDEPEND=">=dev-lang/go-1.21.6"
RDEPEND="
  app-containers/cri-o
  selinux? ( sec-policy/selinux-kubernetes )
"

RESTRICT+=" test "
S="${WORKDIR}/kubernetes-${PV}"

src_compile() {
	CGO_LDFLAGS="$(usex hardened '-fno-PIC ' '')" \
	emake -j1 GOFLAGS="" GOLDFLAGS="" LDFLAGS="" FORCE_HOST_GO=yes \
	WHAT=cmd/${PN}
}

src_install() {
    dobin _output/bin/${PN}
    keepdir /etc/kubernetes/manifests /var/log/kubelet /var/lib/kubelet
    newinitd "${FILESDIR}"/${PN}.initd ${PN}
    newconfd "${FILESDIR}"/${PN}.confd ${PN}
    insinto /etc/logrotate.d
    newins "${FILESDIR}"/${PN}.logrotated ${PN}
    systemd_dounit "${FILESDIR}"/${PN}.service
    insinto /etc/kubernetes
    newins "${FILESDIR}"/${PN}.env ${PN}.env
}

pkg_postinst() {
    elog "This package installs kubelet from commit ${EGIT_COMMIT} on branch ${EGIT_BRANCH}."
    elog "To join a Kubernetes cluster, run 'kubeadm join' with the appropriate token and API server details."
    elog ""
    elog "This package installs the kubelet for a worker node using CRI-O."
    elog "To join a Kubernetes cluster, run 'kubeadm join' with the appropriate token and API server details."
    elog "The kubelet expects a kubeconfig at /etc/kubernetes/kubelet.conf, created by 'kubeadm join'."
    elog "Ensure crio.service is running before starting kubelet."
}
