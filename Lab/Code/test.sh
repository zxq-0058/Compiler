#!/bin/bash
set -e
make parser 
# cp ./parser ../Strict_Test/Lab2_Hard
cp ./parser ../Strict_Test/Lab4_Hard
echo -e "\033[1;32m#### make parser successfully ####\033[0m"

set -e

# echo  -e "\033[1;32m#######################\033[0m"
# echo ">0: 1 否则 输出0 "
# ./parser ../Test/1.cmm ./1.s
# spim -file ./1.s

# echo  -e "\033[1;32m#######################\033[0m"
# echo "阶乘"
# ./parser ../Test/2.cmm ./2.s
# spim -file ./2.s

# echo  -e "\033[1;32m#######################\033[0m"
# echo "奇数1 偶数-1"
# ./parser ../Test/3.cmm ./3.s
# spim -file ./3.s

# echo  -e "\033[1;32m#######################\033[0m"
# echo "输出1，2"
# ./parser ../Test/4.cmm ./4.s
# spim -file ./4.s

# ./parser ../Test/task1.cmm ./task1.s
# spim -file ./task1.s

# ./parser ../Test/task2.cmm ./task2.s
# spim -file ./task2.s

# ./parser ../Test/task3.cmm ./task3.s
# spim -file ./task3.s

# ./parser ../Test/task4.cmm ./task4.s
# spim -file ./task4.s

# 简单数组测试\
# echo  -e "\033[1;32m#######################\033[0m"
# echo "task5: 数组测试"
# ./parser ../Test/task5.cmm ./task5.mips
# spim -file ./task5.mips

# ./parser ../Test/task6.cmm ./task6.mips
# spim -file ./task6.mips

# ./parser ../Test/task7.cmm ./task7.mips
# spim -file ./task7.mips

# 函数测试（fact）
# echo  -e "\033[1;32m#######################\033[0m"
# echo "task8: 函数测试（fact）"
# ./parser ../Test/task8.cmm ./task8.mips
# spim -file ./task8.mips

# ./parser ../Test/task9.cmm ./task9.mips
# spim -file ./task9.mips

# echo  -e "\033[1;32m#######################\033[0m"
# echo "函数测试（honoi）"
# echo "[1000003,1000002,3000002,1000003,2000001,2000003,1000003]"
# ./parser ../Test/hanoi.cmm ./hanoi.mips
# spim -file ./hanoi.mips


# echo  -e "\033[1;32m#######################\033[0m"
# echo "函数测试（fib）"
# # rm *.mips *.ir 

# echo -e "\033[1;32m#### make parser successfully ####\033[0m"
# ./parser ../Test/zzw-3.cmm ./zzw-3.mips
# spim -file ./zzw-3.mips

# echo -e "\033[1;32m#### make parser successfully ####\033[0m"
# ./parser ../Test/yzy18.cmm ./yzy18.mips
# spim -file ./yzy18.mips
# echo "[0, 1, 2, 3, 4, 5, 0]"

echo -e "\033[1;32m#### make parser successfully ####\033[0m"
./parser ../Test/impossible.cmm ./impossible.mips
spim -file ./impossible.mips