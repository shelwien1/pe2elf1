
__int64 __PrintStats(FILE* outFile, FILE* inFile, __int64 isFinal) {
  clock_t currentTime = clock();

  // Throttle UI updates to every 500ms, unless this is the final flush
  if( (currentTime<__arch_hdr[7]+500) && !isFinal ) return 0x10000;

  int outSize = ftell(outFile);
  unsigned int outSizeSafe = (outSize==0) ? 1 : outSize;   // Prevent division by zero
  unsigned int inSize = ftell(inFile)-__arch_hdr[6];

  __arch_hdr[7] = currentTime;

  // Calculate compression ratio in Bits Per Byte (BPB) with rounding
  int bpbInt = (8*inSize)/outSizeSafe;
  int bpbFrac = (1000*(8*inSize-bpbInt*outSizeSafe)+(outSizeSafe>>1))/outSizeSafe;

  if( bpbFrac==1000 ) {
    ++bpbInt;
    bpbFrac = 0;
  }

  // Calculate processing speed in KB/sec
  // elapsedScaled effectively resolves to (elapsedMs * 1.024) / 1000,
  // scaling bytes/ms directly to KB/sec.
  int elapsedMs = currentTime-__arch_hdr[5];
  int elapsedScaled = (elapsedMs*1024)/1000;
  int timeDivisor = (elapsedScaled==0) ? 1 : elapsedScaled;
  unsigned int speedKBs = outSizeSafe/timeDivisor;

  // Calculate memory usage (from internal allocator)
  unsigned int memUsedBytes = __sub_1400032F0();
  unsigned int memUsedKB = memUsedBytes>>10;
  unsigned int memUsedMBInt = memUsedBytes>>20;
  unsigned int memUsedMBFrac = (10*(memUsedKB-(memUsedMBInt<<10))+512)>>10;           // Round to 0.1 MB

  if( memUsedMBFrac==10 ) {
    ++memUsedMBInt;
    memUsedMBFrac = 0;
  }

  // Determine display order (Original Size > Compressed Size)
  unsigned int displayLeft = outSizeSafe;
  unsigned int displayRight = inSize;
  if( !__f_ENC ) { // If decoding, swap the variables for the display
    displayLeft = inSize;
    displayRight = outSizeSafe;
  }

  char printBuffer[568];
  snprintf(printBuffer, sizeof(printBuffer), "%15s:%7u >%7u, %1d.%03d bpb, used:%3u.%1u MB, speed: %u KB/sec.", (const char*)::__Buffer, displayLeft, displayRight, bpbInt, bpbFrac, memUsedMBInt, memUsedMBFrac, speedKBs);

  // '\r' (13) continuously overwrites the line during progress.
  // '\n' (10) finalizes the line when the file finishes.
  char endChar = isFinal ? '\n' : '\r';
  printf("%-79.79s%c", printBuffer, endChar);

  // Log to ratios file if finalized and logging is enabled
  if( isFinal&&__f_LOG ) {
    FILE* logFile = fopen("!ratios.lst", "a+t");
    if( logFile ) {
      fprintf(logFile, "%s\n", printBuffer);
      fclose(logFile);
    }
  }

  return 0x10000;
}
