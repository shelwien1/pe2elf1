Transcoded coder variants, for testing the entropy decoders.

Produced from testfiles/ with jpegtran 2.1.5 (libjpeg-turbo), which transcodes
losslessly in the DCT domain -- every variant of an image holds exactly the same
coefficients, so all six must report identical MCU and block counts:

  <name>.base.jpg           sequential,  Huffman     -copy none -optimize
  <name>.prog.jpg           progressive, Huffman     -copy none -progressive
  <name>.arith.jpg          sequential,  arithmetic  -copy none -arithmetic
  <name>.arithprog.jpg      progressive, arithmetic  -copy none -arithmetic -progressive
  <name>.arithrst.jpg       the same, with one restart interval per MCU row
  <name>.arithprogrst.jpg     (-restart 1B added to the two above)

The restart variants are here for jpgcoder rather than for pjpg.  An entropy
coder resets its predictors at every RSTn, and the arithmetic one has to run its
whole termination procedure and start a fresh coding interval as well -- a path
no file without restart markers can reach.

10-2-t and laplata4_1 are 3-component YCbCr; jcaron is 4-component CMYK, which
exercises a component count above 3 and which djpeg cannot write as BMP (an
output limitation, not a decode one).

run-tests.sh 'coders' checks that Huffman and arithmetic agree on block counts
for every image, and that no scan needs a resync or ends up INCOMPLETE.
