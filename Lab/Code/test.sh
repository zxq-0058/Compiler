#!/bin/bash
set -e
make parser 
# cp ./parser ../Strict_Test/Lab2_Hard
cp ./parser ../Strict_Test/Lab3_Hard
# cp ./parser ../Strict_Test/Lab4_Hard
echo -e "\033[1;32m#### make parser successfully ####\033[0m"

set -e

# echo  -e "\033[1;32m#######################\033[0m"
# ./parser ../Test/task1.cmm optimize.ir
# diff intercode.ir optimize.ir

# echo  -e "\033[1;32m#######################\033[0m"
# echo  -e "\033[1;32m原代码为：\033[0m"
# cat ../Test/task2.cmm
# ./parser ../Test/task2.cmm optimize.ir
# echo -e "\033[1;32m#### task2.cmm 文件输出为: ####\033[0m"
# cat optimize.ir
# echo -e "\033[1;32m#### task2.cmm 文件输出结束，进行文件比对 ####\033[0m"
# diff intercode.ir optimize.ir

# echo  -e "\033[1;32m#######################\033[0m"
# echo  -e "\033[1;32m原代码为：\033[0m"
# cat ../Test/task3.cmm
# ./parser ../Test/task3.cmm optimize.ir
# echo -e "\033[1;32m#### task3.cmm 文件输出为: ####\033[0m"
# cat optimize.ir
# echo -e "\033[1;32m#### task3.cmm 文件输出结束，进行文件比对 ####\033[0m"
# diff intercode.ir optimize.ir

# echo  -e "\033[1;32m#######################\033[0m"
# echo  -e "\033[1;32m原代码为：\033[0m"
# cat ../Test/task4.cmm
# ./parser ../Test/task4.cmm optimize.ir
# echo -e "\033[1;32m#### task4.cmm 文件输出为: ####\033[0m"
# cat optimize.ir
# echo -e "\033[1;32m#### task4.cmm 文件输出结束，进行文件比对 ####\033[0m"
# diff intercode.ir optimize.ir

# echo  -e "\033[1;32m#######################\033[0m"
# echo  -e "\033[1;32m原代码为：\033[0m"
# cat ../Test/task5.cmm
# ./parser ../Test/task5.cmm optimize.ir
# echo -e "\033[1;32m#### task5.cmm 文件输出为: ####\033[0m"
# cat optimize.ir
# echo -e "\033[1;32m#### task5.cmm 文件输出结束，进行文件比对 ####\033[0m"
# diff intercode.ir optimize.ir

for i in {7..7}; do
    echo  -e "\033[1;32m######## 开始测试 task"${i}" ##############\033[0m"
    echo  -e "\033[1;32m原代码为：\033[0m"
    cat ../Test/task${i}.cmm
    ./parser ../Test/task${i}.cmm optimize.ir
    echo -e "\033[1;32m#### 中间代码为： ####\033[0m"
    cat intercode.ir
    echo -e "\033[1;32m#### task${i}.cmm 文件输出为: ####\033[0m"
    cat optimize.ir
    # echo -e "\033[1;32m#### task${i}.cmm 文件输出结束，进行文件比对 ####\033[0m"
    # diff intercode.ir optimize.ir
done

