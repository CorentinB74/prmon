#include <map>

#include "cgapi.h"

class CgroupHandler{
public:
  int del_controller(const std::string& controller);
  int limitmem(int pid, const std::string& val);
  int limitnet(int pid, const std::string& upload, const std::string& download, const std::string& latency);
  int init(int pid, const std::string& controller);
  void set_init(const std::string& controller, bool flag);
  bool is_init(const std::string& controller);


private:
  Cgroup cg;
  // Keep track of modules initialized
  std::map<std::string, bool> init_map{{"memory", false}, {"net_cls", false}};

};
