#include <vector>
#include <string>

#define CG_NV_MAX 100 // Max length of value in stats file
#define CG_CONTROLLER_MAX 100

#define NO_PERMS (-1)
#define NO_UID_GID (-1)

typedef enum{
    CG_SUCCESS = 0,

    // System
    CG_E_INVALID_PARAMS = 1000,
    CG_E_FILE_OPEN,
    CG_E_FILE_IO,
    CG_E_OOM,
    CG_E_STREAM_IO,
    CG_E_MKDIR,
    CG_E_DIR_OPEN,

    // Control group
    CG_E_NOT_INIT,
    CG_E_SUBSYS_NOT_MOUNT,
    CG_E_CONTROLLER_EXIT,
} CgError_t;

void print_error(CgError_t err, const std::string& reason);

class Cgroup{

public:
  Cgroup(std::string &_name)
  : name(_name){}

  Cgroup(const char *_name)
  : name(_name){}

private:
  struct cg_mount_table_entry{
      std::string name; // Subsystem name
      std::string path; // Mount point

      cg_mount_table_entry(std::string &_name, std::string &_path)
      : name(_name), path(_path){}

      cg_mount_table_entry(std::string &_name, const char *_path)
      : name(_name), path(_path){}
  };

  struct control_value{
      std::string name; // Parameter name
      std::string value; // Parameter value
      bool dirty;

      control_value(std::string& _name, std::string& _value, bool _dirty)
      : name(_name), value(_value), dirty(_dirty){}
  };

  struct cgroup_controller{
      std::string name; // Controller name
      std::vector<control_value> params; // Controller parameters

      cgroup_controller(std::string &_name)
      : name(_name){}

      cgroup_controller(const char *_name)
      : name(_name){}
  };

public:
  std::string name; // Control group name
  std::vector<cgroup_controller> controllers; // Controller included in the group
  std::vector<cg_mount_table_entry> cg_mount_table;
  std::string cgroup_mnt = "/sys/fs/cgroup/";

  bool cgroup_init_flag = false;

private:
  CgError_t init();
  CgError_t build_path(const std::string& grp_name, const std::string& subsys_name, std::string& path);
  CgError_t add_parameters(const std::string& controller);
  CgError_t init_group();

  bool is_subsys_mounted(const std::string& name);

public:
  void print_params_controller(const std::string& controller);
  void print_mount_table();

  CgError_t add_controller(const std::string& name);
  CgError_t set_control_value(const std::string& path, const std::string& val);
  CgError_t create_group(const std::string& controller);
  CgError_t set_value(const std::string& controller, const std::string& param, const std::string& val);
  CgError_t assign_proc_group(int pid, const std::string& controller);
};
