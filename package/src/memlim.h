#ifndef PRMON_MEMLIM_H
#define PRMON_MEMLIM_H 1

#include <map>
#include <string>

#include "Ilimit.h"
#include "cgapi.h"

class memlim final : public Ilimit {
private:
  bool flag_init = false;
  Cgroup cg;

public:
  memlim(int pid);

  int set_limits(const std::map<std::string, std::string> limits);
  int del_limits();
  int assign(int pid);
  bool is_init();
  std::string get_type();

public:
  const std::string name = "memory";
};

#endif
