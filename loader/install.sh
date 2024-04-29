#!/bin/bash
# usage : curl -sfL https://github.com/loxilb-io/loxilb-tools/raw/main/loader/install.sh | sh -
git clone --recurse-submodules https://github.com/loxilb-io/iproute2.git && cd iproute2 && \
 cd libbpf/src/ && mkdir build && DESTDIR=build OBJDIR=build make install && cd - && \
 export PKG_CONFIG_PATH=$PKG_CONFIG_PATH:`pwd`/libbpf/src/ && \
 LIBBPF_FORCE=on LIBBPF_DIR=`pwd`/libbpf/src/build ./configure && make && \
 sudo cp -f tc/tc /usr/local/sbin/ntc && cd ../ && rm -fr iproute2

