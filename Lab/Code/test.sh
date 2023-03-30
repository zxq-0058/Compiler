#!/bin/bash
set -e
make parser 
echo -e "\033[1;32m#### make parser successfully ####\033[0m"

set +e
for i in {1..14}; do
    echo -e "\033[1;32m#### BEGIN TEST$i ####\033[0m"
    ./parser ../Test/$i.cmm
done



