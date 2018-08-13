
#include "memlim.h"

#include <sstream>

memlim::memlim(int pid) {

  std::stringstream cgname;
  cgname << "group" << pid;
  std::string cgname_str = cgname.str();
  cg = Cgroup(cgname_str);

  int ret = cg.create_group("memory");
  if (ret) {
    cg.delete_group("memory");
    flag_init = false;
  }
  flag_init = true;
}

int memlim::set_limits(const std::map<std::string, std::string> limits) {
  if (!is_init()) {
    return -1;
  }
  int ret;
  for (const auto &limit : limits) {
    ret = cg.set_value_group(limit.first, limit.second);
    if (ret) {
      cg.delete_group("memory");
      return -1;
    }
  }
  return 0;
}

int memlim::del_limits() {
  if (is_init())
    return cg.delete_group("memory");
  return 0;
}

int memlim::assign(int pid) {
  int ret = cg.assign_proc_group("memory", pid);
  if (ret) {
    cg.delete_group("memory");
    return -1;
  }
  return 0;
}

bool memlim::is_init() { return flag_init; }

std::string memlim::get_type() { return name; }
