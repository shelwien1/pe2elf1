
// ===========================================================================
// main: what vbn's startup code did before it called vbn's main
// (sub_10002160), for plain `u` and `r` only.  The arguments go where the port
// put them, at 0x13E00000, with '/' in the archive and file names turned into
// '\'.  vbn's code runs on a stack inside its image, below them, as it did in
// the port: then every address it handles -- data, heap, stack -- is an image
// address, positive as a 32-bit integer.
// ===========================================================================
static const uint32_t ARGS = 0x13E00000, ARGS_END = 0x13FF0000;
static const uint32_t STACK_TOP = 0x13D00000, STACK_SIZE = 0x500000;   // down to the heap's end

static int vbn_argc;
static char **vbn_argv;
static int vbn_status;
static void vbn_main()
{
  vbn_status = __sub_10002160(vbn_argc, vbn_argv);
}

int main(int argc, char **argv)
{
  if (argc < 3 || (strcmp(argv[1], "u") && strcmp(argv[1], "r"))) {
    printf("vbn_ur: VBN 1.001's u and r commands\n"
           "Usage: vbn_ur u <[path/]archive> <[path/]file|mask> [...]   compress into archive.vbn (MAX mode)\n"
           "       vbn_ur r <[path/]archive>                            restore into the current directory\n");
    return 1;
  }
  image_map();
  if (!getcwd(dos_root, sizeof dos_root)) {
    perror("vbn_ur: getcwd");
    return 127;
  }
  dos_cwd[0] = 0;
  for (int i = 0; i < 256; i++)
    dos_fds[i] = -1;
  dos_fds[0] = 0;
  dos_fds[1] = 1;
  dos_fds[2] = 2;

  uint32_t *av = (uint32_t *)ARGS, cur = ARGS + 4 * ((uint32_t)argc + 1);
  for (int i = 0; i < argc; i++) {
    size_t n = strlen(argv[i]) + 1;
    if (cur + n >= ARGS_END) {
      fprintf(stderr, "vbn_ur: arguments too long\n");
      return 127;
    }
    av[i] = cur;
    char *text = (char *)(uintptr_t)cur;
    memcpy(text, argv[i], n);
    for (char *p = text; i >= 2 && *p; p++)
      if (*p == '/')
        *p = '\\';
    cur += n;
  }
  av[argc] = 0;
  vbn_argc = argc;
  vbn_argv = (char **)av;

  ucontext_t back, run;
  getcontext(&run);
  run.uc_stack.ss_sp = (void *)(uintptr_t)(STACK_TOP - STACK_SIZE);
  run.uc_stack.ss_size = STACK_SIZE;
  run.uc_link = &back;
  makecontext(&run, vbn_main, 0);
  if (swapcontext(&back, &run) < 0) {
    perror("vbn_ur: swapcontext");
    return 127;
  }
  fflush(stdout);
  return vbn_status & 255;                                // DOS 4Ch: AL
}
