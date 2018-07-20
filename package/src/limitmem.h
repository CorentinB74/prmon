#include "cgapi.h"

class CgroupHandler{
public:
  int init(int pid);
  int limitmem(const std::string& val);
  int deletememcg();

private:
  Cgroup cg;
  int m_pid;
  bool cg_init = false;

};
