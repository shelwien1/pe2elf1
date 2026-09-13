/*  oggcomp as a library: the model behind `oggcomp c` and `oggcomp d`, one
    instance at a time, driven by whoever holds the files.

    The same functions come two ways -- linked into a program, and exported
    by a loadable library, oggcompN.dll on Windows and oggcompN.so elsewhere,
    each built from an IDX model of its own -- and a program that takes them
    as a table of pointers (oc_load.inc) runs either without knowing which.

      p = oggcomp_Alloc();                          one instance per library
      oggcomp_Init(p, OC_ENCODE, 0, "in.ogg");      or OC_DECODE; flags below
      oggcomp_addout(p, obuf, sizeof obuf);         somewhere for output to go
      for(;;) {
        r = oggcomp_Loop(p);
        if(r == OC_NEED_INPUT)                      got == 0 is the end of the input
          oggcomp_addinp(p, ibuf, got = read(ibuf));
        else if(r == OC_NEED_OUTPUT || r == OC_DONE) {
          write(obuf, oggcomp_getoutlen(p));
          if(r == OC_DONE) break;
          oggcomp_addout(p, obuf, sizeof obuf);
        } else { puts(oggcomp_Error(p)); break; }   4 and up: an error
      }
      oggcomp_Free(p);

    What goes through it is the coded stream without its file header: the
    seven bytes `oggcomp c` writes first -- "oggc\x1a", the stream version,
    the model's number -- are the caller's to write and to check, and
    oggcomp_StreamVersion() is what goes in the version byte.  Init again
    on the same instance for the next stream; with OC_F_SOLID the model is
    kept from the one before, which is what `oggdet -S` does, and without
    it the model starts over, so that every stream codes as `oggcomp c`
    would code it alone.  */
#ifndef OC_API_H
#define OC_API_H
#ifdef _WIN32
#define OC_CALL __stdcall
#else
#define OC_CALL
#endif
#ifdef OC_BUILD_DLL
#ifdef _WIN32
#define OC_API __declspec(dllexport)
#else
#define OC_API __attribute__((visibility("default")))
#endif
#else
#define OC_API
#endif
#ifdef __cplusplus
extern "C" {
#endif
#define OC_ENCODE 0
#define OC_DECODE 1
/*  Init flags  */
#define OC_F_SOLID 1   /*  keep the model from the run before on this instance  */
#define OC_F_VERBOSE 2 /*  count where the bits go, for oggcomp_Report()  */
#define OC_F_HUGE 4    /*  ask for huge pages for the tables (first Init only)  */
/*  Loop results  */
#define OC_DONE 0
#define OC_NEED_INPUT 1
#define OC_NEED_OUTPUT 2
/*  4 and above: an error, 4 + the exit status `oggcomp` would have given
    (5 a stream it refuses, 7 something the system said, 8 internal), and
    oggcomp_Error() says what.  */
OC_API void *OC_CALL oggcomp_Alloc(void);          /*  0 when the library's instance is out already  */
OC_API void OC_CALL oggcomp_Free(void *p);
OC_API int OC_CALL oggcomp_Init(void *p, int dir, int flags, const char *name); /*  0, or an error as Loop gives them; name is for messages  */
OC_API int OC_CALL oggcomp_Loop(void *p);
OC_API int OC_CALL oggcomp_getoutlen(void *p);    /*  bytes in the output buffer after NEED_OUTPUT or DONE  */
OC_API void OC_CALL oggcomp_addinp(void *p, void *buf, int size); /*  size 0: the input is over  */
OC_API void OC_CALL oggcomp_addout(void *p, void *buf, int size);
OC_API const char *OC_CALL oggcomp_Error(void *p); /*  the last error's message, "" when none  */
OC_API int OC_CALL oggcomp_StreamVersion(void);   /*  what the stream's version byte holds  */
OC_API unsigned long long OC_CALL oggcomp_TableBytes(void); /*  the model tables, once mapped  */
OC_API const char *OC_CALL oggcomp_Describe(void); /*  one line about the model  */
OC_API void OC_CALL oggcomp_Report(void *p);      /*  where the bits went, on stderr, after an encode with OC_F_VERBOSE  */
#ifdef __cplusplus
}
#endif
#endif
