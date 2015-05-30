# Maintainer: Federico Di Pierro <nierro92@gmail.com>

pkgname=ncursesfm-git
_gitname=ncursesFM
pkgver=r163.6a97e6b
pkgrel=1
pkgdesc="A FileManager written in c and ncurses library."
arch=('i686' 'x86_64')
url="https://github.com/FedeDP/${_gitname}"
license=('GPL')
depends=('ncurses' 'libconfig')
optdepends=('libcups: for printing support'
            'fuseiso: for fuse archive/iso mounting support'
            'libarchive: for files/folders (de)compression support'
            'openssl: for shasum viewing support')
makedepends=('git' 'libarchive' 'libcups' 'openssl')
source=("git://github.com/FedeDP/${_gitname}.git")
backup=('etc/default/ncursesFM.conf')
install=ncursesFM.install
sha256sums=("SKIP")

pkgver() {
  cd $_gitname
  printf "0.r%s.%s" "$(git rev-list --count HEAD)" "$(git rev-parse --short HEAD)"
}

build()
{
    cd $srcdir/$_gitname
    make ncursesFM
    make clean
}

package() {
    cd $srcdir/$_gitname
    mkdir -p $pkgdir/usr/bin
    install -Dm755 ncursesFM $pkgdir/usr/bin
    mkdir -p $pkgdir/etc/default
    install -Dm644 ncursesFM.conf $pkgdir/etc/default
}
