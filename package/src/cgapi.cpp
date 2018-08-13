#include "cgapi.h"

int Cgroup::create_group(const std::string &controller) {
  return system(("cgcreate -g " + controller + ":" + name).c_str());
}

int Cgroup::delete_group(const std::string &controller) {
  return system(("cgdelete -g " + controller + ":" + name).c_str());
}

int Cgroup::assign_proc_group(const std::string &controller, int pid) {
  return system(
      ("cgclassify -g " + controller + ":" + name + " " + std::to_string(pid))
          .c_str());
}

int Cgroup::set_value_group(const std::string &val_name,
                            const std::string &value) {
  return system(("cgset -r " + val_name + "=" + value + " " + name).c_str());
}
