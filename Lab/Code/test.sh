#!/bin/bash
set -e
make parser 
cp ./parser ../Strict_Test/Lab2_Hard
echo -e "\033[1;32m#### make parser successfully ####\033[0m"

set +e
./parser ../Test/1.cmm ../irsim/1.ir

./parser ../Test/2.cmm ../irsim/2.ir

./parser ../Test/3.cmm ../irsim/3.ir

./parser ../Test/4.cmm ../irsim/4.ir

./parser ../Test/5.cmm ../irsim/5.ir

./parser ../Test/6.cmm ../irsim/6.ir

./parser ../Test/7.cmm ../irsim/7.ir

./parser ../Test/8.cmm ../irsim/8.ir

./parser ../Test/9.cmm ../irsim/9.ir

./parser ../Test/10.cmm ../irsim/10.ir