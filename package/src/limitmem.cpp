#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <iostream>
#include <sstream>
#include <fstream>

#include "limitmem.h"

static Cgroup cg("");

int limitmem(int pid, const std::string& val){
  if (getuid()){
    std::cerr << "Permission denied (root needed)." << std::endl;
    return -1;
  }

  std::string cgname = "group" + std::to_string(pid);
  cg.name = cgname;

  int ret = cg.create_group("memory");
  if(ret){
    cg.delete_group("memory");
    return -1;
  }

  ret = cg.set_value("memory", "memory.limit_in_bytes", val);
  if(ret){
    cg.delete_group("memory");
    return -1;
  }

  ret = cg.assign_proc_group(pid, "memory");
  if(ret){
    cg.delete_group("memory");
    return -1;
  }

  return 0;
}

int rmvcgroup(int pid){
  return cg.delete_group("memory");
}
