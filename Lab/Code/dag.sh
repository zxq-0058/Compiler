set -e
make clean
make parser
./parser ../Test/dag1.cmm out.ir
cat intercode.ir
echo "------------------"
cat out.ir

echo  -e "\033[1;32m##########  dag测试(2)  #####\033[0m"
./parser ../Test/dag2.cmm out.ir
echo -e "\033[1;32m#### 优化后中间代码为： ####\033[0m"
cat out.ir
echo -e "\033[1;32m#### 优化前中间代码为： ####\033[0m"
cat intercode.ir
echo "------------------"
diff out.ir intercode.ir

# echo  -e "\033[1;32m##########  dag测试(3)  #####\033[0m"
# ./parser ../Test/dag3.cmm out.ir
# echo -e "\033[1;32m#### 优化后中间代码为： ####\033[0m"
# cat out.ir
# echo -e "\033[1;32m#### 优化前中间代码为： ####\033[0m"
# cat intercode.ir
# echo "------------------"
