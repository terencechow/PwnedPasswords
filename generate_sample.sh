#!/usr/bin/env bash

for i in {1..100}
do
    echo -n "test$i" | openssl sha1 | awk '{print toupper($0)}' >> data/pwned-passwords-sample.txt
done