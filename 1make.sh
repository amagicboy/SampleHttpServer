#!/bin/bash
gcc -o main main.c func.c -lssl -lcrypto
echo "123456"|sudo -S ./main
