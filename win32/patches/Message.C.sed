# Diagnostics: no ANSI color sequences (which the Windows console shows as text), messages
# written to their file descriptor (stderr) rather than to std::cout, and Windows paths in the
# program name
s|^    overridePropertiesNS().useColor = true;$|    overridePropertiesNS().useColor = false; /* win32 */|
s|^    std::cout <<render(mesg, props);$|    { std::string s = render(mesg, props); ::write(fd_, s.c_str(), (unsigned)s.size()); } /* win32 */|
s|size_t slashIdx = name.rfind('/');|size_t slashIdx = name.find_last_of("/\\\\"); /* win32 */|
