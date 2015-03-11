# Maintainer: Federico Di Pierro <nierro92@gmail.com>

pkgname=ncursesfm-git
_gitname=ncursesFM
pkgver=12
pkgrel=1
pkgdesc="A FileManager written in c and ncurses library."
arch=('i686' 'x86_64')
url="https://github.com/FedeDP/${_gitname}"
license=('GPL')
depends=('ncurses' 'libconfig')
optdepends=('fuseiso: iso images mount support')
makedepends=('git')
source=("git://github.com/FedeDP/${_gitname}.git")
md5sums=("SKIP")

pkgver() {
	cd ${_gitname}
    git rev-list --count HEAD
}

build()
{
    cd $srcdir/$_gitname
    make
}

package() {
    cd $srcdir/$_gitname
    mkdir -p $pkgdir/usr/bin
    install -Dm755 ncursesfm $pkgdir/usr/bin
    mkdir -p $pkgdir/etc/default
    install -Dm644 ncursesFM.conf $pkgdir/etc/default
}
