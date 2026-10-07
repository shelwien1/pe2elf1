# The name of the executable (Sawyer::thisExecutableName), which Sawyer's Windows code lacks
s|^# if 0 // \[Robb Matzke 2014-06-13\] temporarily disable.*|    retval = _pgmptr ? _pgmptr : ""; /* win32: the executable */\n&|
