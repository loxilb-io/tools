#!/bin/bash

if [[ $# -lt 3 ]] ; then
    echo "Need to specify version,release and arch" 
    exit 1
fi

# exit when any command fails
set -e

ver=$(echo $1)
gver=v$ver
if [[ $ver == "latest" ]]; then
  gver="main"
  ver="0.99rc"
fi
rel=$(echo $2)
arch=$(echo $3)

rm -fr loxilb_$ver-$rel_$arch
rm -fr loxilb*.deb
mkdir loxilb_$ver-$rel_$arch
mkdir -p loxilb_$ver-$rel_$arch/usr/local/sbin/
mkdir -p loxilb_$ver-$rel_$arch/usr/local/sbin/
mkdir -p loxilb_$ver-$rel_$arch/etc/systemd/system/
mkdir -p loxilb_$ver-$rel_$arch/DEBIAN/
mkdir -p loxilb_$ver-$rel_$arch/opt/loxilb/
mkdir -p loxilb_$ver-$rel_$arch/etc/loxilb/
mkdir -p loxilb_$ver-$rel_$arch/opt/loxilb/cert/

cp -f systemd/loxilb.service loxilb_$ver-$rel_$arch/etc/systemd/system/
cp -f debian/postinst loxilb_$ver-$rel_$arch/DEBIAN/
cp -f debian/preinst loxilb_$ver-$rel_$arch/DEBIAN/

echo "Package: loxilb
Version: $ver-$rel
Section: base
Priority: optional
Architecture: $arch
Depends:
Maintainer: Trekkie <trekkie@netlox.io>
Description: LoxiLB" >> loxilb_$ver-$rel_$arch/DEBIAN/control

wget https://github.com/openssl/openssl/releases/download/openssl-3.3.1/openssl-3.3.1.tar.gz && tar -xvzf openssl-3.3.1.tar.gz && \
cd openssl-3.3.1 && ./Configure enable-ktls '-Wl,-rpath,$(LIBRPATH)' --prefix=/usr/local/build && \
make -j$(nproc) && sudo make install_dev install_modules && cd - && \
sudo rm /usr/include/openssl/ && \
sudo cp -a /usr/local/build/include/openssl /usr/include/ && \
if [ -d /usr/local/build/lib64  ] ; then sudo mv /usr/local/build/lib64  /usr/local/build/lib; fi && \
sudo cp -fr /usr/local/build/lib/* /usr/lib/ && sudo ldconfig && \
rm -fr openssl-3.3.1*

LD_LIBRARY_PATH="${LD_LIBRARY_PATH}:/usr/lib/"

echo "openssl local build"
ls /usr/local/build/include/openssl

echo "openssl local build"
ls /usr/include/openssl

git clone --recurse-submodules https://github.com/loxilb-io/loxilb
cd loxilb/loxilb-ebpf
git checkout $gver
cd -
cd loxilb
git checkout $gver
make
cd -
cp loxilb/loxilb loxilb_$ver-$rel_$arch/usr/local/sbin/
cp loxilb/loxilb-ebpf/utils/mkllb_bpffs.sh loxilb_$ver-$rel_$arch/usr/local/sbin/mkllb_bpffs
sudo cp /opt/loxilb/*.o loxilb_$ver-$rel_$arch/opt/loxilb/
rm -fr loxilb

git clone https://github.com/loxilb-io/loxicmd.git && cd loxicmd && \
git fetch --all --tags && git checkout $gver && go get . && make && cd -
cp ./loxicmd/loxicmd loxilb_$ver-$rel_$arch/usr/local/sbin/loxicmd
rm -fr loxicmd

dpkg-deb --build loxilb_$ver-$rel_$arch
