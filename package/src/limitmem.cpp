#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <iostream>
#include <sstream>
#include <fstream>

#include "limitmem.h"

bool CgroupHandler::is_init(const std::string& controller){
  for(const auto& it : init_map){
    if(it.first == controller)
      return it.second;
  }
  return false;
}

void CgroupHandler::set_init(const std::string& controller, bool flag){
  for(auto& it : init_map){
    if(it.first == controller){
      it.second = flag;
      return;
    }
  }
}

int CgroupHandler::init(int pid, const std::string& controller){
  if(is_init(controller))
    return 0;

  std::stringstream cgname;
  cgname << "group" << pid;
  std::string cgname_str = cgname.str();
  cg = Cgroup(cgname_str);

  int ret = cg.create_group(controller.c_str());
  if(ret){
    cg.delete_group(controller.c_str());
    return -1;
  }

  set_init(controller, true);
  return 0;
}

int CgroupHandler::limitmem(int pid, const std::string& val){
  if(!is_init("memory")){
    return -1;
  }

  int ret = cg.set_value_group("memory.limit_in_bytes", val);
  if(ret){
    cg.delete_group("memory");
    return -1;
  }

  ret = cg.assign_proc_group("memory", pid);
  if(ret){
    cg.delete_group("memory");
    return -1;
  }

  return 0;
}

int CgroupHandler::limitnet(int pid, const std::string& upload, const std::string& download, const std::string& latency){
  if(!is_init("net_cls")){
    return -1;
  }

  int ret = cg.set_value_group("net_cls.classid", "0x10010");
  if(ret){
    cg.delete_group("net_cls");
    return -1;
  }

  ret = cg.assign_proc_group("net_cls", pid);
  if(ret){
    cg.delete_group("net_cls");
    return -1;
  }

  ret = system(("bash ../scripts/CgNetLimit.sh -u " + upload + " -d " + download + " -l " + latency).c_str());
  if(ret){
    cg.delete_group("net_cls");
    return -1;
  }
  return 0;
}

int CgroupHandler::del_controller(const std::string& controller){
  if(is_init(controller))
    return cg.delete_group(controller);
  return 0;
}
