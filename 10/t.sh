#!/bin/bash
set -e

rm -f 1 2

./dummy c book1 1
./dummy d 1 2

md5sum 1 book1.pmm 2 book1
cmp 1 book1.pmm
cmp 2 book1
