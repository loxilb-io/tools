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

cat > loxilb_$ver-$rel_$arch/DEBIAN/control <<EOF
Package: loxilb
Version: $ver-$rel
Section: base
Priority: optional
Architecture: $arch
Depends: libc6, libssl3, libelf1, zlib1g, libzstd1
Maintainer: Trekkie <trekkie@netlox.io>
Description: LoxiLB
EOF

wget https://github.com/openssl/openssl/releases/download/openssl-3.3.1/openssl-3.3.1.tar.gz && tar -xvzf openssl-3.3.1.tar.gz && \
cd openssl-3.3.1 && ./Configure enable-ktls '-Wl,-rpath,$(LIBRPATH)' --prefix=/usr/local/build && \
make -j$(nproc) && sudo make install_dev install_modules && cd - && \
sudo rm -fr /usr/include/openssl/ && \
sudo cp -a /usr/local/build/include/openssl /usr/include/ && \
if [ -d /usr/local/build/lib64  ] ; then sudo mv /usr/local/build/lib64  /usr/local/build/lib; fi && \
sudo cp -fr /usr/local/build/lib/* /usr/lib/ && sudo ldconfig && \
rm -fr openssl-3.3.1*

LD_LIBRARY_PATH="${LD_LIBRARY_PATH}:/usr/lib/"

git clone --recurse-submodules https://github.com/loxilb-io/loxilb
cd loxilb/loxilb-ebpf
git checkout $gver
cd -
cd loxilb
git checkout $gver
# Pass the version explicitly rather than letting the loxilb Makefile derive it
# from the checked out tag: that derivation fails silently (empty VERSION, no
# stamp, in-source dev placeholder) if the clone ever loses its tags, and $ver
# is the value this build was asked for anyway.
make VERSION=$ver
# Confirm the stamp actually landed. Without this a build that skipped the
# stamp still produces a deb, and the mismatch only surfaces once someone runs
# `loxilb --version` on an installed release.
if ! version_out=$(./loxilb --version 2>&1); then
    echo "Could not run the freshly built loxilb binary:" >&2
    echo "$version_out" >&2
    exit 1
fi
built_ver=$(printf '%s\n' "$version_out" | awk '/^loxilb version:/ {print $3; exit}')
if [[ "$built_ver" != "$ver" && "$built_ver" != "$ver"-* ]]; then
    echo "loxilb version mismatch: requested '$ver', binary reports '${built_ver:-<none>}'" >&2
    exit 1
fi
echo "loxilb binary reports version '$built_ver'"
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
