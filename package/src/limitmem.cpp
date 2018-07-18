#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <iostream>
#include <sstream>
#include <fstream>

#include "limitmem.h"

int rmvcgroup(int pid){
  std::stringstream cgdir{"/sys/fs/cgroup/memory/group"};
  cgdir << pid;
  if(remove(cgdir.str().c_str())){
    std::perror("Error in deleting the cgroup ");
    return -1;
  }
  return 0;
}

int limitmem(int pid, double pourcentage){
  if (getuid()){
    std::cerr << "Permission denied (root needed)." << std::endl;
    return -1;
  }
  if(pourcentage > 100 || pourcentage < 0){
    std::cerr << "The memory limit (in %) should be between 0 and 100." << std::endl;
    return -1;
  }

  // Creating a new cgroup for this process
  std::stringstream groupDirectory;
  groupDirectory << "/sys/fs/cgroup/memory/group" << pid;

  int ret = mkdir(groupDirectory.str().c_str(), S_IRWXU | S_IRGRP | S_IROTH);

  if(ret){
    std::perror("Error in creating a new cgroup ");
    return -1;
  }

  // Query the amount of available memory
  long long int memSize, newMemSize;

  std::stringstream memFileSize;
  std::string param, size;
  memFileSize << "/proc/meminfo";

  std::ifstream proc_io{memFileSize.str()};
  while(proc_io) {
    proc_io >> param >> memSize >> size;
    if (!param.compare("MemAvailable:")){
      break;
    }
  }

  std::cout << "Amount of available memory : " << memSize << " kB." << std::endl;
  memSize *= 1000;
  newMemSize = memSize - memSize*pourcentage/100;
  std::cout << "Restraining memory by " << pourcentage << "% ..." << std::endl;

  // Setting the new memory size
  std::stringstream filePath;
  filePath << groupDirectory.str() << "/memory.limit_in_bytes";

  std::ofstream memLimitFile{filePath.str().c_str()};
  if(memLimitFile.is_open()){
    memLimitFile << newMemSize << std::endl;
  }
  else{
    std::cerr << "Error in restricting the memory." << std::endl;
    rmvcgroup(pid);
    return -1;
  }
  memLimitFile.close();

  std::cout << "New amount of available memory : " << newMemSize/1000 << " kB." <<  std::endl;

  // Adding this process in the new cgroup
  filePath.str("");
  filePath << groupDirectory.str() << "/cgroup.procs";

  std::ofstream procsFile{filePath.str().c_str()};
  if(procsFile.is_open()){
    procsFile << pid << std::endl;
  }
  else{
    std::cerr << "Cannot add this process in the group." << std::endl;
    rmvcgroup(pid);
    return -1;
  }

  return 0;
}
