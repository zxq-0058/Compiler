set -e
make clean
make parser

echo  -e "\033[1;32m##########  样例测试（1）局部公共子表达式  #####\033[0m"
./parser ../Test/sample1.ir out.ir
echo -e "\033[1;32m#### 优化后中间代码为： ####\033[0m"
cat out.ir
echo -e "\033[1;32m#### 优化前中间代码为： ####\033[0m"
cat intercode.ir
echo "------------------"

echo  -e "\033[1;32m##########  样例测试（2）局部无用代码消除  #####\033[0m"
./parser ../Test/sample2.ir out.ir
echo -e "\033[1;32m#### 优化后中间代码为： ####\033[0m"
cat out.ir
echo -e "\033[1;32m#### 优化前中间代码为： ####\033[0m"
cat intercode.ir
echo "------------------"

echo  -e "\033[1;32m##########  样例测试（3）  #####\033[0m"
./parser ../Test/sample3.ir out.ir
echo -e "\033[1;32m#### 优化后中间代码为： ####\033[0m"
cat out.ir
echo -e "\033[1;32m#### 优化前中间代码为： ####\033[0m"
cat intercode.ir
echo "------------------"

