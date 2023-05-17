#!/bin/bash
set -e
make parser 
# cp ./parser ../Strict_Test/Lab2_Hard
cp ./parser ../Strict_Test/Lab3_Hard
echo -e "\033[1;32m#### make parser successfully ####\033[0m"

set +e
# ./parser ../Test/1.cmm ./1.s

./parser ../Test/task1.cmm ./task1.s
spim -file ./task1.s
