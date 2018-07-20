#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <iostream>
#include <sstream>
#include <fstream>

#include "limitmem.h"

int CgroupHandler::init(int pid){
  if (getuid()){
    std::cerr << "Permission denied (root needed)." << std::endl;
    return -1;
  }

  std::string cgname = "group" + std::to_string(pid);
  cg = Cgroup(cgname);

  int ret = cg.create_group("memory");
  if(ret){
    cg.delete_group("memory");
    return -1;
  }

  m_pid = pid;
  cg_init = true;
  return 0;
}

int CgroupHandler::limitmem(const std::string& val){
  if(!cg_init){
    return -1;
  }

  int ret = cg.set_value("memory", "memory.limit_in_bytes", val);
  if(ret){
    cg.delete_group("memory");
    return -1;
  }

  ret = cg.assign_proc_group(m_pid, "memory");
  if(ret){
    cg.delete_group("memory");
    return -1;
  }

  return 0;
}

int CgroupHandler::deletememcg(){
  return cg.delete_group("memory");
}
