# Pointers as "0x...": ROSE recognizes the names it generates from addresses by that prefix,
# which "%p" lacks on Windows
s|snprintf(numberString, sizeof(numberString), "%p", x);|snprintf(numberString, sizeof(numberString), "0x%llx", (unsigned long long)(size_t)x); /* win32 */|
