# Copyright 1999-2025 Gentoo Authors
# Distributed under the terms of the GNU General Public License v2

EAPI=8

#inherit go-module systemd git-r3
inherit go-module systemd

DESCRIPTION="Kubernetes Proxy service"
HOMEPAGE="https://github.com/kubernetes/kubernetes https://kubernetes.io"
SRC_URI="https://github.com/kubernetes/kubernetes/archive/v${PV}.tar.gz -> kubernetes-${PV}.tar.gz"

# EGIT_COMMIT=4b8e819355d791d96b7e9d9efe4cbafae2311c88
# EGIT_REPO_URI="https://github.com/kubernetes/kubernetes"
# EGIT_BRANCH="release-1.29"

LICENSE="Apache-2.0"
SLOT="0"
KEYWORDS="amd64 ~arm64"
IUSE="hardened"

RDEPEND="
    app-containers/cri-o
    net-firewall/conntrack-tools
"
BDEPEND=">=dev-lang/go-1.21.6"

RESTRICT+=" test"
# Comment if you are going to use git
S="${WORKDIR}/kubernetes-${PV}"

src_compile() {
    CGO_LDFLAGS="$(usex hardened '-fno-PIC ' '')" \
        emake -j1 GOFLAGS="" GOLDFLAGS="" LDFLAGS="" FORCE_HOST_GO=yes \
        WHAT=cmd/${PN}
}

src_install() {
    dobin _output/bin/${PN}
    keepdir /var/log/${PN} /var/lib/${PN}
    insinto /etc/logrotate.d
    newins "${FILESDIR}"/${PN}.logrotated ${PN}
    systemd_dounit "${FILESDIR}"/${PN}.service
    insinto /etc/kubernetes
    newins "${FILESDIR}"/${PN}.env ${PN}.env
}

pkg_postinst() {
    elog "This package installs kube-proxy for a Kubernetes worker node using CRI-O."
    elog "Ensure 'kubeadm join' has been run to generate /etc/kubernetes/kube-proxy.yaml."
    elog "Start the service with 'systemctl start kube-proxy' after enabling crio.service."
}
