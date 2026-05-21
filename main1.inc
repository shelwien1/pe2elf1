
int __main(int argc, const char** argv) {
  sub_140002AD0();

  // Preserve SIMD state / floating-point control
  unsigned int csr = _mm_getcsr()|0x8040;
  _mm_setcsr(csr);

  int resetMethod = 1;
  int modelOrder = 12;
  int memoryMB = 256;

  if( argc != 4 ) {
    printf("Usage: coder c input archive | coder d archive input\n");
    return -1;
  }

  // Parse Mode (Encode / Decode)
  char mode = toupper(argv[1][0]);
  if( mode == 'C' ) {
    __f_ENC = 1;
  } else if( mode == 'D' ) {
    __f_ENC = 0;
  } else {
    printf("unknown command: %s\n", argv[1]);
    return -1;
  }

  int mb = atoi(&argv[1][1]);
  if( mb>0 ) memoryMB=mb;

  char (**streamBlock)[4096] = sub_140012804();
  FILE* internalStream = (FILE*)(streamBlock+6);

  int orderValPacked = 2*(((char)modelOrder+14)&0xF);
  int memValPacked = 32*(((short)memoryMB-1)&0xFFF);
  int resetValPacked = 32*(resetMethod&1);

  if( !__f_ENC ) {
    // --- DECOMPRESSION MODE ---
    const char* archiveName = argv[2];
    const char* outputName = argv[3];

    FILE* inFile = fopen(archiveName, "rb");
    if( !inFile ) {
      printf("Can`t open file %s", archiveName);
      exit(-1);
    }

    if( fread(__arch_hdr, 0x10, 1, inFile) ) {
      if( __arch_hdr[0]!=0x8CACAF8F||(__arch_hdr[2]&0x1F)!=9 ) {
        printf("Can`t open file %s", archiveName);
        exit(-1);
      }

      char decodedFileName[512];
      size_t nameLen = (unsigned short)(HIWORD(__arch_hdr[2])&0xFF80)>>7;
      fread(decodedFileName, nameLen, 1, inFile);
      decodedFileName[nameLen] = '\0';

      // Ignore the decodedFileName from archive, extract to requested output
      ::__Buffer = (void*)outputName;
      FILE* outFile = fopen(outputName, "wb");
      if( !outFile ) {
        printf("Can`t open file %s", outputName);
        exit(-1);
      }

      unsigned int param1 = ((__arch_hdr[2]&0x1FFE0u)>>5)+1;
      unsigned int param2 = ((unsigned char)(BYTE2(__arch_hdr[2])&0x1E)>>1)+2;
      int param3 = (unsigned char)(BYTE2(__arch_hdr[2])&0x20)>>5;

      sub_140003370();
      if( !__Alloc_PPMblock(param1, param2, param3) ) {
        printf("Out of memory!");
        exit(-1);
      }

      __arch_hdr[5] = clock();
      __arch_hdr[6] = ftell(inFile);

      if( (__arch_hdr[2]&0x400000)!=0 ) {
        // Stored without compression
        unsigned int rawBytesCount;
        fread(&rawBytesCount, 4, 1, inFile);
        for(; rawBytesCount; --rawBytesCount ) {
          putc(getc(inFile), outFile);
        }
        clock_t endTime = clock();
        __arch_hdr[7] = endTime;
        printf("%15s:%7ld >%7ld extracted.\n", (const char*)::__Buffer, ftell(outFile), ftell(inFile)-__arch_hdr[6]);
      } else {
        sub_14000C310(outFile, inFile, __PrintStats, 0);
      }

      fclose(outFile);
    }
    fclose(inFile);

  } else {
    // --- COMPRESSION MODE ---
    const char* inputName = argv[2];
    const char* archiveName = argv[3];

    FILE* inFile = fopen(inputName, "rb");
    if( !inFile ) {
      printf("Can`t open file %s", inputName);
      exit(-1);
    }

    FILE* outFile = fopen(archiveName, "wb");
    if( !outFile ) {
      printf("Can`t open file %s", archiveName);
      exit(-1);
    }

    const char* strippedName = strrchr(inputName, '/');
    if (!strippedName) strippedName = strrchr(inputName, '\\');
    strippedName = strippedName ? strippedName+1 : inputName;
    ::__Buffer = (void*)strippedName;

    fseek(inFile, 0, SEEK_END);
    __arch_hdr[1] = ftell(inFile);
    fseek(inFile, 0, SEEK_SET);

    __arch_hdr[0] = 0x8CACAF8F;

    // Header flag packing bitwise magic
    ((char*)&__arch_hdr[2])[0] = (((char*)&__arch_hdr[2])[0]&0xE0)|9;
    ((char*)&__arch_hdr[2])[2] = orderValPacked|(((char*)&__arch_hdr[2])[2]&0xE1);
    __arch_hdr[2] = memValPacked|(__arch_hdr[2]&0xFFFE001F);
    ((char*)&__arch_hdr[2])[2] = (resetValPacked&0xBF)|(((char*)&__arch_hdr[2])[2]&0x9F);

    size_t bufferLen = strlen((const char*)::__Buffer);
    HIWORD(__arch_hdr[2]) = (bufferLen<<7)|(HIWORD(__arch_hdr[2])&0x7F);

    fwrite(__arch_hdr, 0x10, 1, outFile);
    fwrite(::__Buffer, (unsigned short)(HIWORD(__arch_hdr[2])&0xFF80)>>7, 1, outFile);

    sub_140003370();
    if( !__Alloc_PPMblock(memoryMB, modelOrder, resetMethod) ) {
      printf("Out of memory!");
      exit(-1);
    }

    __arch_hdr[5] = clock();
    __arch_hdr[6] = ftell(outFile);

    sub_140003530(outFile, inFile, __PrintStats, 0);

    // Re-packing uncompressible data if size exploded
    long inPos = ftell(inFile);
    if( inPos != -1L ) {
      unsigned int outSize = (unsigned int)ftell(outFile);
      if( (unsigned int)(outSize-__arch_hdr[6])>(unsigned long long)(inPos+4LL) ) {
        unsigned int rawSize = (unsigned int)inPos;
        fseek(inFile, 0, SEEK_SET);

        int headerOffset = __arch_hdr[6]-((unsigned short)(HIWORD(__arch_hdr[2])&0xFF80)>>7)-16;
        fseek(outFile, headerOffset, SEEK_SET);
        _chsize(_fileno(outFile), headerOffset);

        BYTE2(__arch_hdr[2]) |= 0x40; // Set uncompressed flag

        fwrite(__arch_hdr, 0x10, 1, outFile);
        fwrite(::__Buffer, (unsigned short)(HIWORD(__arch_hdr[2])&0xFF80)>>7, 1, outFile);
        fwrite(&rawSize, 4, 1, outFile);

        for(; rawSize; --rawSize ) {
          putc(getc(inFile), outFile);
        }
      }
    }

    fclose(inFile);
    fclose(outFile);
  }

  sub_140003370();
  return 0;
}
