set -e
cd ./Code
make parser
cp ./parser ../parser
make clean
cd ..
rm 张旭钦_201240058.zip
zip -r 张旭钦_201240058.zip . -x "./Strict_Test/*"