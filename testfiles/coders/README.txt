Transcoded coder variants, for testing the entropy decoders.

Produced from testfiles/ with jpegtran 2.1.5 (libjpeg-turbo), which transcodes
losslessly in the DCT domain -- every variant of an image holds exactly the same
coefficients, so all four must report identical MCU and block counts:

  <name>.base.jpg       sequential,  Huffman     jpegtran -copy none -optimize
  <name>.prog.jpg       progressive, Huffman     jpegtran -copy none -progressive
  <name>.arith.jpg      sequential,  arithmetic  jpegtran -copy none -arithmetic
  <name>.arithprog.jpg  progressive, arithmetic  jpegtran -copy none -arithmetic -progressive

10-2-t and laplata4_1 are 3-component YCbCr; jcaron is 4-component CMYK, which
exercises a component count above 3 and which djpeg cannot write as BMP (an
output limitation, not a decode one).

run-tests.sh 'coders' checks that Huffman and arithmetic agree on block counts
for every image, and that no scan needs a resync or ends up INCOMPLETE.
