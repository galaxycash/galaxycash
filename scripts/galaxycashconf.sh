#!/bin/bash -ev

mkdir -p ~/.galaxycash
echo "rpcuser=username" >>~/.galaxycash/galaxycash.conf
echo "rpcpassword=`head -c 32 /dev/urandom | base64`" >>~/.galaxycash/galaxycash.conf

