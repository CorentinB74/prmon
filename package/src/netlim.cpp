#include "netlim.h"

#include <sstream>

netlim::netlim(int pid) {
  std::stringstream cgname;
  cgname << "group" << pid;
  std::string cgname_str = cgname.str();
  cg = Cgroup(cgname_str);

  int ret = cg.create_group("net_cls");
  if (ret) {
    cg.delete_group("net_cls");
    flag_init = false;
    return;
  }

  ret = cg.set_value_group("net_cls.classid", "0x10010");
  if (ret) {
    cg.delete_group("net_cls");
    flag_init = false;
    return;
  }
  flag_init = true;
}

int netlim::set_limits(const std::map<std::string, std::string> limits) {
  if (!is_init()) {
    return -1;
  }
  std::string upload, download, latency;
  for (const auto &limit : limits) {
    if (limit.first == "upload")
      upload = limit.second;
    else if (limit.first == "download")
      download = limit.second;
    else if (limit.first == "latency")
      latency = limit.second;
  }
  if (upload.empty() || download.empty() || latency.empty())
    return -1;
  int ret = system(("bash ~/Documents/prmon/package/scripts/CgNetLimit.sh -u " +
                    upload + " -d " + download + " -l " + latency)
                       .c_str());
  if (ret) {
    cg.delete_group("net_cls");
    return -1;
  }
  return 0;
}

int netlim::del_limits() {
  int ret = 0;
  if (is_init()) {
    ret = cg.delete_group("net_cls");
    ret = system("bash ~/Documents/prmon/package/scripts/CgNetLimit.sh -x");
  }
  return ret;
}

int netlim::assign(int pid) {
  int ret = cg.assign_proc_group("net_cls", pid);
  if (ret) {
    cg.delete_group("net_cls");
    return -1;
  }
  return 0;
}

bool netlim::is_init() { return flag_init; }

std::string netlim::get_type() { return name; }
