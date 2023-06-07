set -e

make clean
make parser

cp ./parser ../Strict_Test/Lab3_Hard

echo  -e "\033[1;32m##########  全局传播测试(1)  #####\033[0m"
./parser ../Test/g_constprop1.cmm optimize.ir
echo -e "\033[1;32m#### 优化前中间代码为： ####\033[0m"
cat intercode.ir
echo -e "\033[1;32m#### 优化后中间代码为： ####\033[0m"
cat optimize.ir

echo  -e "\033[1;32m##########  全局传播测试(2)  #####\033[0m"
./parser ../Test/g_constprop2.cmm optimize.ir
echo -e "\033[1;32m#### 优化前中间代码为： ####\033[0m"
cat intercode.ir
echo -e "\033[1;32m#### 优化后中间代码为： ####\033[0m"
cat optimize.ir