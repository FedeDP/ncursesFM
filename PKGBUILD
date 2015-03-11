# Maintainer: Federico Di Pierro <nierro92@gmail.com>

pkgname=ncursesfm-git
_gitname=ncursesFM
pkgver=11
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
    cp ncursesfm $pkgdir/usr/bin
    cp ncursesFM.conf $pkgdir/etc/default
}
