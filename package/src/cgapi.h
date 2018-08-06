#ifndef PRMON_CGAPI_H
#define PRMON_CGAPI_H 1

#include <vector>
#include <string>

class Cgroup{

public:
  Cgroup(std::string &_name)
  : name(_name){}

  Cgroup(const char *_name)
  : name(_name){}

private:
  std::string name;

public:

  int create_group(const std::string& controller);
  int delete_group(const std::string& controller);
  int assign_proc_group(const std::string& controller, int pid);
  int set_value_group(const std::string& val_name, const std::string& value);
};

#endif
