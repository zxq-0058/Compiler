#!/bin/bash
set -e
make parser 
cp ./parser ../Strict_Test/Lab2_Hard
echo -e "\033[1;32m#### make parser successfully ####\033[0m"

set +e
for i in {1..23}; do
    echo -e "\033[1;32m#### BEGIN TEST$i ####\033[0m"
    ./parser ../Test/$i.cmm
done



