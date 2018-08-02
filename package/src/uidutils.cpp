#include <pwd.h>
#include <grp.h>
#include <iostream>

#include <sys/types.h>
#include <unistd.h>

#include "uidutils.h"

int drop_privileges(const std::string& username){
  struct passwd *pw = NULL;
  pw = getpwnam(username.c_str());
  if (pw) {
    if (initgroups(pw->pw_name, pw->pw_gid) != 0 ||
        setgid(pw->pw_gid) != 0 || setuid(pw->pw_uid) != 0) {
      std::cerr << "Could not drop privileges.\n";
      return 1;
    }
  } else {
    std::cerr << "Could not find user : " << username << std::endl;
    return 1;
  }
  return 0;
}
