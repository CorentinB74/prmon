#include <cstring>
#include <iostream>
#include <fstream>
#include <sstream>

#include <sys/types.h>
#include <sys/stat.h>
#include <mntent.h>
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>

#include "cgapi.h"

int Cgroup::init(){
    int ret = 0;
    std::string subsys_name;
    int hier, num_cgroups, enabled;
    std::vector<std::string> controllers;
    FILE *proc_mounts = NULL;
    char *mntopt = NULL;
    char mntent_buf[4 * FILENAME_MAX];
    struct mntent *ent = NULL, *tmp_ent = NULL;

    // Clean control group mount table
    cg_mount_table.clear();

    // Read '/proc/cgroups' to get what subsystems are enabled
    std::ifstream proc_cgroups{"/proc/cgroups"};
    if (!proc_cgroups.is_open()){
        print_error(CG_E_FILE_OPEN, "/proc/cgroups");
        ret = -1;
        goto unlock_exit;
    }

    while (!proc_cgroups.eof()){
        std::string line;

        std::getline(proc_cgroups, line);
        if (proc_cgroups.fail()){
            if (!proc_cgroups.eof()){
                print_error(CG_E_FILE_IO, "/proc/cgroups");
                ret = -1;
                goto unlock_exit;
            }
        }
        else{
            if (line.empty())
                // Skip empty line
                continue;
            else if (line.find_first_of('#') == 0)
                // Skip comment line
                continue;
            else{
                std::istringstream iss(line);
                iss >> std::dec >> subsys_name >> hier >> num_cgroups >> enabled;
                if (iss.fail() && !iss.eof()){
                    ret = CG_E_STREAM_IO;
                    goto unlock_exit;
                }
                controllers.push_back(subsys_name);
            }
        }
    }

    // Read '/proc/mounts' to get mount pointer of subsystems
    proc_mounts = fopen("/proc/mounts", "re");
    if (!proc_mounts){
        print_error(CG_E_FILE_OPEN, "/proc/mounts");
        ret = -1;
        goto unlock_exit;
    }

    tmp_ent = new struct mntent;
    if (!tmp_ent){
        print_error(CG_E_OOM, "mntent struct");
        ret = -1;
        goto unlock_exit;
    }
    while ((ent = getmntent_r(proc_mounts, tmp_ent, mntent_buf, sizeof(mntent_buf))) != NULL){
        // Skip noncgroup mount entry
        if (std::string(ent->mnt_type).compare("cgroup"))
            continue;

        for (auto& c : controllers){
            mntopt = hasmntopt(ent, c.c_str());
            if (!mntopt)
                continue;

            // Check duplicates
            for (const auto& it : cg_mount_table){
                if (c.compare(it.name) == 0){
                    // Skip duplicates
                    continue;
                }
			      }

            cg_mount_table_entry new_entry(c, ent->mnt_dir);
            cg_mount_table.push_back(new_entry);
        }
    }

    cgroup_init_flag = true;

unlock_exit:
    if (tmp_ent)
        delete tmp_ent;

    if (proc_cgroups.is_open())
        proc_cgroups.close();

    if (proc_mounts)
        fclose(proc_mounts);

    return ret;
}

void Cgroup::print_mount_table(){
    std::cout << "control group mount table" << std::endl;
    for (const auto& it : cg_mount_table){
        std::cout << "subsystem \'" << it.name << "\' is mounted at \'" << it.path << "\'" << std::endl;
    }
}

void Cgroup::print_params_controller(const std::string& controller){
    for(auto& cit : controllers){
      if(!cit.name.compare(controller)){
        for(auto& pit : cit.params){
          std::cout << "file \'" << pit.name << "\' has value " << pit.value << std::endl;
        }
      }
    }
}

int Cgroup::set_control_value(const std::string& path, const std::string& val){
    std::ofstream control_file{path.c_str()};

    if (!control_file.is_open()){
      print_error(CG_E_FILE_OPEN, path.c_str());
      return -1;
    }

    control_file << val;
    if (control_file.fail()){
      print_error(CG_E_STREAM_IO, path.c_str());
      return -1;
    }

    control_file.close();
    return 0;
}

bool Cgroup::is_subsys_mounted(const std::string& name){
	for (const auto& it : cg_mount_table)
        if (name.compare(it.name) == 0)
            return true;
	return false;
}

int Cgroup::build_path(const std::string& grp_name, const std::string& subsys_name, std::string& path){
	for (auto& entry : cg_mount_table){
	    if (entry.name.compare(subsys_name) == 0){
		    std::ostringstream oss;
		    oss << entry.path << "/" << grp_name;
		    if (oss.fail()){
          print_error(CG_E_STREAM_IO, oss.str());
          return -1;
        }

			path = oss.str();
			return 0;
		}
	}
  print_error(CG_E_SUBSYS_NOT_MOUNT, "");
	return -1;
}

int Cgroup::add_controller(const std::string& name){
  for (const auto& controller : controllers){
      if (controller.name.compare(name) == 0){
        print_error(CG_E_CONTROLLER_EXIT, controller.name);
        return -1;
      }
    }

  cgroup_controller ctlr(name.c_str());
	controllers.push_back(ctlr);
	return 0;
}

int Cgroup::add_parameters(const std::string& controller) {
  int ret = 0;

  std::string path;
  ret = build_path(name, controller, path);
  if(ret != 0)
    return ret;

  DIR* d;
  struct dirent* dir;
  d = opendir(path.c_str());
  if (d) {
    std::stringstream ss{};
    std::ifstream ifs{};
    std::string dirname, val_str;

    for(auto& cit : controllers){
      if(!cit.name.compare(controller)){
        while ((dir = readdir(d)) != NULL) {
          if (!(!std::strcmp(dir->d_name, ".") || !std::strcmp(dir->d_name, "..") ||
              !std::strcmp(dir->d_name, "memory.stat") || !std::strcmp(dir->d_name, "memory.oom_control") ||
              !std::strcmp(dir->d_name, "memory.numa_stat"))){
            ss << path << "/" << dir->d_name;
            ifs.open(ss.str().c_str());
            if(ifs.is_open()){
              getline(ifs,val_str); // Saves the line in STRING.
              dirname = dir->d_name;
              cit.params.push_back(control_value(dirname, val_str, false));
              ifs.close();
            }
            else {
              print_error(CG_E_FILE_OPEN, ss.str());
              ret = -1;
            }
            ss.str("");
          }
        }
      }
    }
    closedir(d);
  } else {
    return CG_E_DIR_OPEN;
  }
  return ret;
}

int Cgroup::init_group(){
  int ret = 0;

	if (!cgroup_init_flag){
    print_error(CG_E_NOT_INIT, "");
    return -1;
  }

  // Check if all subsystems are mounted
	for (auto& controller : controllers)
		if (!is_subsys_mounted(controller.name)){
      print_error(CG_E_SUBSYS_NOT_MOUNT, controller.name);
      return -1;
    }

  for (auto& controller : controllers){
	  std::string path;
	  std::string base;

		if (build_path(name, controller.name, path) != CG_SUCCESS)
			continue;

    if (mkdir(path.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH)){
      print_error(CG_E_MKDIR, path);
      return -1;
    }

    base = path;
		for (auto& param : controller.params){
	    std::ostringstream oss;
      oss << base << param.name;
	    if (oss.fail())
	        return -1;

		  ret = set_control_value(path, param.value);
		}
	}

	return ret;
}

int Cgroup::create_group(const std::string& controller){
  int ret = 0;

  ret = init();
  if (ret != 0)
      return ret;

  ret = add_controller(controller);
  if (ret != 0)
      return ret;

  ret = init_group();
  if (ret != 0)
      return ret;

  ret = add_parameters(controller);
  if (ret != 0)
    return ret;

  return ret;
}

int Cgroup::set_value(const std::string& controller, const std::string& param, const std::string& val){
  int ret = 0;

  for (auto& ctrlit : controllers){
	  std::string path;
	  std::string base;

    if(ctrlit.name.compare(controller))
      continue;

    ret = build_path(name, ctrlit.name, path);
		if (ret != 0)
			return ret;

    base = path;
		for (auto& paramit : ctrlit.params){
      if(paramit.name.compare(param))
        continue;

      std::stringstream ss;
      ss << path << "/" << param;
	    if (ss.fail())
	        return -1;

		  ret = set_control_value(ss.str().c_str(), val);
      if(ret == CG_SUCCESS){
        std::ifstream ifs{ss.str().c_str()};
        if(ifs.is_open()){
          std::string val_str;
          getline(ifs, val_str);
          paramit.value = val_str;
          ifs.close();
        }
        return ret;
      }
      else
        return ret;
		}
	}
  print_error(CG_E_INVALID_PARAMS, param);
  return -1;
}

int Cgroup::assign_proc_group(int pid, const std::string& controller){
  return set_value(controller, "tasks", std::to_string(pid));
}

int Cgroup::delete_group(const std::string& controller){
  std::string path;
  int ret = build_path("", controller, path);
  if(ret)
    return -1;

  return remove(path.c_str());
}

void print_error(CgError_t err, const std::string& reason){
  switch (err) {
    case CG_E_INVALID_PARAMS:
      std::cerr << "cgroup error : invalid parameter" << " (" << reason << ")." << std::endl;
      break;
    case CG_E_FILE_OPEN:
      std::cerr << "cgroup error : cannot open file" << " (" << reason << ")." << std::endl;
      break;
    case CG_E_FILE_IO:
      std::cerr << "cgroup error : IO file" << " (" << reason << ")." << std::endl;
      break;
    case CG_E_OOM:
      std::cerr << "cgroup error : memory allocation" << " (" << reason << ")." << std::endl;
      break;
    case CG_E_STREAM_IO:
      std::cerr << "cgroup error : IO stream" << " (" << reason << ")." << std::endl;
      break;
    case CG_E_MKDIR:
      std::cerr << "cgroup error : cannot make directory" << " (" << reason << ")." << std::endl;
      break;
    case CG_E_DIR_OPEN:
      std::cerr << "cgroup error : cannot open directory" << " (" << reason << ")." << std::endl;
      break;
    case CG_E_NOT_INIT:
      std::cerr << "cgroup error : cannot initiate cgroup service" << " (" << reason << ")." << std::endl;
      break;
    case CG_E_SUBSYS_NOT_MOUNT:
      std::cerr << "cgroup error : cgroup not mounted" << " (" << reason << ")." << std::endl;
      break;
    case CG_E_CONTROLLER_EXIT:
      std::cerr << "cgroup error : controller does not exist" << " (" << reason << ")." << std::endl;
      break;
    default:
      break;
  }
}
