
#ifndef READ_CONFIG_H__
#define READ_CONFIG_H__

#include <stdlib.h>
#include <string>


class CReadConfig {
 public:
  CReadConfig();
  CReadConfig (const char* pConfigFileName);
  CReadConfig (const std::string& pConfigFileName);
  virtual ~CReadConfig();

  void Openf (const char* strFile);
  long ReadLine (std::string* strVal, const int iValSize = 4);
  const bool EndOfFile();
  const int GetLines();
  const bool ExistFile();
  const std::string& GetFileName();

 private:
  FILE*             m_pCfgFile;
  std::string       m_strCfgFileName;
  unsigned int      m_iLines;
};

#endif // READ_CONFIG_H__

