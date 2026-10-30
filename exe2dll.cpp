// exe2dll - static PE surgery that turns a console EXE into a DLL.
//
// Goal: let a wrapper/launcher invoke a tool as   rundll tool.dll,run <args>
// instead of letting people double-click / run the EXE directly, WITHOUT
// rewriting the tool as a real DLL and WITHOUT any runtime code patching,
// manual mapping, or debugger tricks (the sort of things AVs dislike).
//
// How it works - three small, static edits to a normal PE:
//   1. Set the IMAGE_FILE_DLL characteristic so the OS loader treats the file
//      as a DLL (LoadLibrary) instead of refusing it.
//   2. Replace the PE entry point with a tiny DllMain stub ( mov eax,1; ret )
//      so that LoadLibrary does NOT run the program at load time - it just
//      returns TRUE for DLL_PROCESS_ATTACH.
//   3. Append an export directory that exports one name ("run" by default)
//      pointing at the EXE's ORIGINAL entry point (its C runtime startup).
//
// Calling the "run" export therefore runs the C runtime startup exactly as
// the EXE would: global ctors, then main(), then the CRT calls exit() /
// ExitProcess - i.e. the process ends when the tool is done, just like the
// real EXE. The companion "rundll" loader sets up the command line first so
// the CRT parses the intended argv (see rundll.cpp).
//
// Relocations, imports, TLS and load-config are left untouched, so the normal
// Windows loader maps the image, applies base relocations (the input must be
// relocatable, i.e. keep its .reloc / DYNAMIC_BASE - console EXEs built by
// modern linkers are) and binds imports the usual way. Nothing is patched at
// run time; the output is an ordinary, valid DLL on disk.
//
// This is append-only surgery: it writes one extra section-header into the
// spare padding of the PE header and appends one section of raw data at the
// end of the file. Existing sections keep their file offsets and RVAs, so the
// code, relocation targets and data directories stay valid as-is.
//
// Build (portable, no Windows headers needed):
//     g++ -O2 -o exe2dll exe2dll.cpp
// Usage:
//     exe2dll <in.exe> <out.dll> [exportname]      (exportname defaults to "run")

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <vector>
#include <string>

typedef uint8_t  byte;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

// ---- little-endian field access (PE is always little-endian) ----
static u16 g16( const byte* p ) { u16 v; memcpy(&v,p,2); return v; }
static u32 g32( const byte* p ) { u32 v; memcpy(&v,p,4); return v; }
static void p16( byte* p, u16 v ) { memcpy(p,&v,2); }
static void p32( byte* p, u32 v ) { memcpy(p,&v,4); }

static u64 AlignUp( u64 x, u64 a ) { return a ? ((x + a-1) - (x + a-1) % a) : x; }

static std::vector<byte> readfile( const char* fn ) {
  std::vector<byte> d;
  FILE* f = fopen( fn, "rb" );
  if( !f ) return d;
  fseek(f,0,SEEK_END); long n=ftell(f); fseek(f,0,SEEK_SET);
  if( n>0 ) { d.resize(n); if( fread(d.data(),1,n,f)!=(size_t)n ) d.clear(); }
  fclose(f);
  return d;
}

// Offsets inside IMAGE_FILE_HEADER (COFF), relative to the "PE\0\0" signature.
enum { COFF = 4,
       COFF_NumSections   = COFF + 2,
       COFF_SizeOptHeader = COFF + 16,
       COFF_Characteristics = COFF + 18,
       OPT  = COFF + 20 };
// Offsets of IMAGE_OPTIONAL_HEADER64 fields, given as absolute offsets from
// the "PE\0\0" signature (i.e. already include the OPT base).
enum { OPT_Magic              = OPT + 0,
       OPT_AddressOfEntry     = OPT + 16,
       OPT_ImageBase          = OPT + 24,
       OPT_SectionAlignment   = OPT + 32,
       OPT_FileAlignment      = OPT + 36,
       OPT_SizeOfImage        = OPT + 56,
       OPT_SizeOfHeaders      = OPT + 60,
       OPT_CheckSum           = OPT + 64,
       OPT_DllCharacteristics = OPT + 70,
       OPT_NumberOfRvaAndSizes= OPT + 108,
       OPT_DataDirectory      = OPT + 112 };  // 16 x { u32 VA; u32 Size; }

enum { SEC_SIZE = 40,
       SEC_VirtualSize    = 8,
       SEC_VirtualAddress = 12,
       SEC_SizeOfRawData  = 16,
       SEC_PointerToRaw   = 20,
       SEC_Characteristics= 36 };

#define IMAGE_FILE_DLL              0x2000
#define IMAGE_SCN_CNT_CODE          0x00000020
#define IMAGE_SCN_MEM_EXECUTE       0x20000000
#define IMAGE_SCN_MEM_READ          0x40000000

int main( int argc, char** argv ) {
  if( argc < 3 ) {
    fprintf( stderr,
      "exe2dll - turn a console EXE into a DLL that exports an entry function.\n"
      "usage: %s <in.exe> <out.dll> [exportname]   (exportname defaults to \"run\")\n",
      argv[0] );
    return 1;
  }
  const char* infn  = argv[1];
  const char* outfn = argv[2];
  const char* expnm = (argc>3) ? argv[3] : "run";

  std::vector<byte> d = readfile( infn );
  if( d.empty() ) { fprintf(stderr,"error: cannot read '%s'\n",infn); return 1; }
  byte* b = d.data();
  size_t fsize = d.size();

  if( fsize < 0x40 || g16(b) != 0x5A4D ) { fprintf(stderr,"error: not an MZ file\n"); return 1; }
  u32 e = g32(b+0x3C);                                  // e_lfanew -> PE header
  if( e+OPT+4 > fsize || g32(b+e) != 0x00004550 ) { fprintf(stderr,"error: no PE signature\n"); return 1; }

  u32 pe = e;                                           // base for COFF_* offsets
  if( g16(b+pe+OPT_Magic) != 0x20B ) {
    fprintf(stderr,"error: not PE32+ (x64). This tool handles 64-bit images only.\n");
    return 1;
  }

  u16 nsec   = g16(b+pe+COFF_NumSections);
  u16 szopt  = g16(b+pe+COFF_SizeOptHeader);
  u16 chars  = g16(b+pe+COFF_Characteristics);
  u32 secTbl = pe + OPT + szopt;                        // start of section table
  u32 salign = g32(b+pe+OPT_SectionAlignment);
  u32 falign = g32(b+pe+OPT_FileAlignment);
  u32 szhdrs = g32(b+pe+OPT_SizeOfHeaders);
  u32 oep    = g32(b+pe+OPT_AddressOfEntry);
  u32 nrva   = g32(b+pe+OPT_NumberOfRvaAndSizes);
  u16 dllch  = g16(b+pe+OPT_DllCharacteristics);

  if( (u64)secTbl + (u64)nsec*SEC_SIZE > fsize ) { fprintf(stderr,"error: section table out of range\n"); return 1; }
  if( nrva < 1 || szopt < 120 ) { fprintf(stderr,"error: optional header has no data directories to hold an export entry\n"); return 1; }

  // We append one section header into the spare header padding. Require room.
  if( (u64)secTbl + (u64)(nsec+1)*SEC_SIZE > szhdrs ) {
    fprintf(stderr,
      "error: no room in PE headers for another section header\n"
      "       (section table ends at 0x%X, SizeOfHeaders=0x%X; need 0x%X)\n",
      secTbl+nsec*SEC_SIZE, szhdrs, secTbl+(nsec+1)*SEC_SIZE );
    return 1;
  }

  // Warn (don't fail) if the image looks non-relocatable: a DLL may be loaded
  // at a base other than ImageBase, and without relocations it would crash.
  bool hasReloc = (nrva>5) && g32(b+pe+OPT_DataDirectory+5*8) != 0;
  if( !hasReloc )
    fprintf(stderr,"warning: no base relocation directory - the DLL may only work if loaded at its preferred base.\n");

  // Compute where the new section lands in memory (after the last one).
  u64 maxEnd = 0;
  for( u16 i=0; i<nsec; i++ ) {
    const byte* s = b + secTbl + (size_t)i*SEC_SIZE;
    u32 va = g32(s+SEC_VirtualAddress);
    u32 vs = g32(s+SEC_VirtualSize);
    u32 rs = g32(s+SEC_SizeOfRawData);
    u64 end = (u64)va + (vs>rs?vs:rs);
    if( end>maxEnd ) maxEnd = end;
  }
  u32 newVA = (u32)AlignUp( maxEnd, salign );
  u32 newPR = (u32)AlignUp( fsize, falign );            // raw data appended at EOF

  // ---- derive the DLL "Name" string from the output filename ----
  std::string dllname = outfn;
  { size_t s = dllname.find_last_of("/\\"); if( s!=std::string::npos ) dllname = dllname.substr(s+1); }
  if( dllname.empty() ) dllname = "out.dll";

  // ---- build the new section's content ----
  //   [0]  DllMain stub: mov eax,1 ; ret   (6 bytes), padded to 8
  //   [8]  IMAGE_EXPORT_DIRECTORY (40 bytes)
  //   [..] EAT(4) ENT(4) EOT(2) "<exportname>\0" "<dllname>\0"
  const u32 STUB = 0;
  const u32 EXP  = 8;
  const u32 EAT  = EXP + 40;        // 48
  const u32 ENT  = EAT + 4;         // 52
  const u32 EOT  = ENT + 4;         // 56
  const u32 NRUN = EOT + 2;         // 58
  u32 rl = (u32)strlen(expnm) + 1;
  u32 NDLL = NRUN + rl;
  u32 dl = (u32)dllname.size() + 1;
  u32 contentVS = NDLL + dl;

  std::vector<byte> C( contentVS, 0 );
  byte* c = C.data();

  // DllMain stub
  c[STUB+0]=0xB8; c[STUB+1]=0x01; c[STUB+2]=0x00; c[STUB+3]=0x00; c[STUB+4]=0x00; c[STUB+5]=0xC3;

  // IMAGE_EXPORT_DIRECTORY
  p32(c+EXP+0,  0);                 // Characteristics
  p32(c+EXP+4,  0);                 // TimeDateStamp
  p16(c+EXP+8,  0);                 // MajorVersion
  p16(c+EXP+10, 0);                 // MinorVersion
  p32(c+EXP+12, newVA+NDLL);        // Name (RVA of dll name)
  p32(c+EXP+16, 1);                 // Base (ordinal base)
  p32(c+EXP+20, 1);                 // NumberOfFunctions
  p32(c+EXP+24, 1);                 // NumberOfNames
  p32(c+EXP+28, newVA+EAT);         // AddressOfFunctions
  p32(c+EXP+32, newVA+ENT);         // AddressOfNames
  p32(c+EXP+36, newVA+EOT);         // AddressOfNameOrdinals
  // tables
  p32(c+EAT, oep);                  // EAT[0] -> original entry point RVA
  p32(c+ENT, newVA+NRUN);           // ENT[0] -> name RVA
  p16(c+EOT, 0);                    // EOT[0] -> function index 0
  // strings
  memcpy( c+NRUN, expnm, rl );
  memcpy( c+NDLL, dllname.c_str(), dl );

  u32 contentRS = (u32)AlignUp( contentVS, falign );

  // ---- assemble output image ----
  std::vector<byte> out = d;                            // start from the original
  out.resize( (size_t)newPR + contentRS, 0 );           // pad to new-section raw end
  byte* o = out.data();

  // new section header, written into the header padding after the last entry
  byte* sh = o + secTbl + (size_t)nsec*SEC_SIZE;
  memset( sh, 0, SEC_SIZE );
  memcpy( sh, ".exe2dll", 8 );                          // section name (cosmetic)
  p32( sh+SEC_VirtualSize,     contentVS );
  p32( sh+SEC_VirtualAddress,  newVA );
  p32( sh+SEC_SizeOfRawData,   contentRS );
  p32( sh+SEC_PointerToRaw,    newPR );
  p32( sh+SEC_Characteristics, IMAGE_SCN_CNT_CODE|IMAGE_SCN_MEM_EXECUTE|IMAGE_SCN_MEM_READ );

  // patch COFF / optional header
  p16( o+pe+COFF_NumSections,    nsec+1 );
  p16( o+pe+COFF_Characteristics, chars | IMAGE_FILE_DLL );
  p32( o+pe+OPT_AddressOfEntry,  newVA+STUB );          // entry -> DllMain stub
  p32( o+pe+OPT_SizeOfImage,     (u32)AlignUp( (u64)newVA+contentVS, salign ) );
  p32( o+pe+OPT_CheckSum,        0 );                   // not needed for a user DLL
  // export data directory [0]
  p32( o+pe+OPT_DataDirectory+0, newVA+EXP );
  p32( o+pe+OPT_DataDirectory+4, contentVS-EXP );

  // copy new section raw data
  memcpy( o+newPR, C.data(), contentVS );

  // ---- write it out ----
  FILE* g = fopen( outfn, "wb" );
  if( !g ) { fprintf(stderr,"error: cannot create '%s'\n",outfn); return 1; }
  if( fwrite( out.data(), 1, out.size(), g ) != out.size() ) { fprintf(stderr,"error: write failed\n"); fclose(g); return 1; }
  fclose(g);

  printf( "exe2dll: %s -> %s\n", infn, outfn );
  printf( "  original entry (RVA)   : %08X  (exported as \"%s\")\n", oep, expnm );
  printf( "  new DllMain stub (RVA) : %08X  (set as DLL entry point)\n", newVA+STUB );
  printf( "  export directory (RVA) : %08X  size %08X\n", newVA+EXP, contentVS-EXP );
  printf( "  appended section       : VA %08X  rawoff %08X  vsize %X rawsize %X\n", newVA, newPR, contentVS, contentRS );
  printf( "  Characteristics        : %04X -> %04X (IMAGE_FILE_DLL set)\n", chars, chars|IMAGE_FILE_DLL );
  printf( "  SizeOfImage            : %08X\n", (u32)AlignUp((u64)newVA+contentVS,salign) );
  printf( "  DllCharacteristics     : %04X%s\n", dllch, hasReloc?"":"  (NO RELOCS - see warning)" );
  printf( "done. load with: rundll %s,%s <args>\n", dllname.c_str(), expnm );
  return 0;
}
