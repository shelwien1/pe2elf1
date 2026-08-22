#!/bin/sh
# Linux port of t.bat - compress/decompress every test image and check
# that the decoded output matches the original (compare the two md5 lines).

bmf=${BMF:-./bmf}

for a in testfiles/*.bmp; do

  rm -f 1 2

  echo "[$a]"
  "$bmf" c "$a" 1
  "$bmf" d 1 2

  md5sum "$a" 2
  for b in "$a" 1; do echo "$b - $(wc -c <"$b")"; done

  {
    md5sum "$a" 2
    for b in "$a" 1; do echo "$b - $(wc -c <"$b")"; done
  } >>log.txt

  rm -f 1 2

  echo
  echo

done
