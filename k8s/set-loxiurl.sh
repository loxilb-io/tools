#!/bin/bash

usage() {
    echo "Usage: $0 -a <ip-addr> -z <zone> -t <type>"
}

if [[ $# -ne 6 ]];then
   usage
   exit
fi

addr=""
zone="llb"
utype="default"

while getopts a:z:t: opt 
do
    case "${opt}" in
        a) addr=${OPTARG};;
        z) zone=${OPTARG};;
        t) utype=${OPTARG};;
    esac
done

echo addr $addr
echo zone $zone
echo utype $utype

cat <<EOF | kubectl apply -f -
apiVersion: "loxiurl.loxilb.io/v1"
kind: LoxiURL
metadata:
  name: llb-${addr}
spec:
  loxiURL: http://${addr}:11111
  zone: llb
  type: ${utype}
EOF
