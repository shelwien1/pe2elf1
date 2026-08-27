
#if defined(_WIN32) && !defined(FILE_API_STD)
#include "file_api_win.inc"
#else
#include "file_api_std.inc"
#endif
