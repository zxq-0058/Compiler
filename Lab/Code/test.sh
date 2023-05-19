#!/bin/bash
set -e
make parser 
# cp ./parser ../Strict_Test/Lab2_Hard
cp ./parser ../Strict_Test/Lab3_Hard
echo -e "\033[1;32m#### make parser successfully ####\033[0m"

set +e
# ./parser ../Test/1.cmm ./1.s

# ./parser ../Test/task1.cmm ./task1.s
# spim -file ./task1.s

set -e
# ./parser ../Test/task2.cmm ./task2.s
# spim -file ./task2.s

# ./parser ../Test/task3.cmm ./task3.s
# spim -file ./task3.s

# ./parser ../Test/task4.cmm ./task4.s
# spim -file ./task4.s

./parser ../Test/task5.cmm ./task5.s
spim -file ./task5.s