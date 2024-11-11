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
