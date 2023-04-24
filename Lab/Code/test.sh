#!/bin/bash
set -e
make parser 
# cp ./parser ../Strict_Test/Lab2_Hard
cp ./parser ../Strict_Test/Lab3_Hard
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

# ./parser ../Test/9.cmm ../irsim/9.ir

./parser ../Test/10.cmm ../irsim/10.ir

./parser ../Test/struct.cmm ../irsim/struct.ir

./parser ../Test/struct_array.cmm ../irsim/struct_array.ir

./parser ../Test/logic.cmm ../irsim/logic.ir

./parser ../Test/logic1.cmm ../irsim/logic1.ir

./parser ../Test/logic2.cmm ../irsim/logic2.ir

./parser ../Test/easy_func1.cmm ../irsim/easy_func1.ir

./parser ../Test/easy_func2.cmm ../irsim/easy_func2.ir

./parser ../Test/students.cmm ../irsim/students.ir

./parser ../Test/high_dim.cmm ../irsim/high_dim.ir


./parser ../Test/tmp.cmm ../irsim/tmp.ir

./parser ../Test/tmp1.cmm ../irsim/tmp1.ir

./parser ../Test/floyd.cmm ../irsim/floyd.ir

