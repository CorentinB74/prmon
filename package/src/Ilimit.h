#ifndef PRMON_ILIM_H
#define PRMON_ILIM_H 1

#include <map>
#include <string>

class Ilimit {
public:
  virtual ~Ilimit(){};

  virtual int set_limits(const std::map<std::string, std::string> limits) = 0;
  virtual int del_limits() = 0;
  virtual bool is_init() = 0;
  virtual int assign(int pid) = 0;
  virtual std::string get_type() = 0;
};

#endif
