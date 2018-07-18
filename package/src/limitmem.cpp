#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <iostream>
#include <sstream>
#include <fstream>

#include "limitmem.h"
#include "cgapi.h"


int limitmem(int pid, const std::string& val){
  if (getuid()){
    std::cerr << "Permission denied (root needed)." << std::endl;
    return -1;
  }

  std::string cgname = "group" + std::to_string(pid);
  Cgroup cg(cgname);

  CgError_t ret = cg.create_group("memory");
  if(ret){
    print_error(ret);
  }

  ret = cg.set_value("memory", "memory.limit_in_bytes", val);
  if(ret){
    print_error(ret);
  }
  
  ret = cg.assign_proc_group(pid, "memory");
  if(ret){
    print_error(ret);
  }

  return 0;
}
