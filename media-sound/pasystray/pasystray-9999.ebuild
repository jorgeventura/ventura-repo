# Copyright 1999-2024 Gentoo Authors
# Distributed under the terms of the GNU General Public License v2

EAPI=8

inherit git-r3 autotools xdg

DESCRIPTION="PulseAudio system tray"
HOMEPAGE="https://github.com/christophgysin/pasystray"
EGIT_REPO_URI="https://github.com/christophgysin/pasystray.git"
# If the default branch is not master, uncomment:
# EGIT_BRANCH="main"

LICENSE="LGPL-2.1+"
SLOT="0"

# Live ebuilds should not be keyworded
KEYWORDS=""
IUSE="libnotify zeroconf"

# Typical live-ebuild metadata
PROPERTIES="live"
RESTRICT="mirror"

RDEPEND="
	dev-libs/glib
	|| (
		media-libs/libpulse[glib]
		media-sound/pulseaudio-daemon[glib,zeroconf?]
	)
	x11-libs/gtk+:3
	x11-libs/libX11
	zeroconf? ( net-dns/avahi )
	libnotify? ( x11-libs/libnotify )
"
DEPEND="${RDEPEND}"
BDEPEND="
	virtual/pkgconfig
"

src_prepare() {
	default
	eautoreconf
}

src_configure() {
	econf \
		$(use_enable libnotify notify) \
		$(use_enable zeroconf avahi)
}

