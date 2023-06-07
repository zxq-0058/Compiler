set -e

make clean
make parser

cp ./parser ../Strict_Test/Lab3_Hard

echo  -e "\033[1;32m##########  常量传播测试(1)  #####\033[0m"
./parser ../Test/constprop1.cmm optimize.ir
echo -e "\033[1;32m#### 优化前中间代码为： ####\033[0m"
cat intercode.ir
echo -e "\033[1;32m#### 优化后中间代码为： ####\033[0m"
cat optimize.ir

echo  -e "\033[1;32m##########  常量传播测试(2)  #####\033[0m"
./parser ../Test/constprop2.cmm optimize.ir
echo -e "\033[1;32m#### 优化前中间代码为： ####\033[0m"
cat intercode.ir
echo -e "\033[1;32m#### 优化后中间代码为（返回 #32）： ####\033[0m"
cat optimize.ir