#!/bin/bash
set -e
make parser 
echo -e "\033[1;32m#### make parser successfully ####\033[0m"

set +e
for i in {1..7}; do
    echo "#### BEGIN TEST$i ####"
    ./parser ../Test/test$i.cmm > ../Test/myanswer$i.txt
    if diff ../Test/myanswer$i.txt ../Test/answer$i.txt >/dev/null; then
        echo -e "\033[1;32mPassed!\033[0m"
    else
        echo -e "\033[1;31mFailed!\033[0m"
        echo "Differences for test $i:"
        diff ../Test/myanswer$i.txt ../Test/answer$i.txt
    fi
done



