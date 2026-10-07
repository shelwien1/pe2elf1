# preventConstructionOnStack(): the check that an IR node is not within 10 KB of the current stack
# frame fails at times on Windows, where the heap can be next to the stack
s#^     assert (dist < -10000 || dist > 10000);$#     (void)dist; /* win32: heap and stack can be adjacent */#
