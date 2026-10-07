# Rose::utility_stripPathFromFileName and Rose::getPathFromFileName: Windows directory separators
s#size_t pos = fileNameWithPath.rfind('/');#size_t pos = fileNameWithPath.find_last_of("/\\\\"); /* win32 */#
s#size_t pos = fileName.rfind('/');#size_t pos = fileName.find_last_of("/\\\\"); /* win32 */#
