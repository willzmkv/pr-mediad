# PKGBUILD
pkgname=pr-mediad
pkgver=0.1.0
pkgrel=1
pkgdesc="Project-R media detection daemon"
arch=('x86_64')
url="https://github.com/willzmkv/pr-mediad"
license=('GPL3')
depends=('libudev.so' 'libnotify')
makedepends=('cmake' 'pkgconf')
source=("$pkgname-$pkgver.tar.gz")
sha256sums=('SKIP')

build() {
    cmake -B build -S "$pkgname-$pkgver" \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX=/usr
    cmake --build build
}

check() {
    ctest --test-dir build --output-on-failure
}

package() {
    DESTDIR="$pkgdir" cmake --install build
    install -Dm644 "$pkgname-$pkgver/pr-mediad.service" \
        "$pkgdir/usr/lib/systemd/system/pr-mediad.service"
    install -Dm644 "$pkgname-$pkgver/pr-mediad-tmpfiles.conf" \
        "$pkgdir/usr/lib/tmpfiles.d/pr-mediad.conf"
}
