# librose is a static library on Windows: no DLL import/export attributes
s|^#ifdef BOOST_WINDOWS$|#if 0 /* win32: librose is a static library */|
