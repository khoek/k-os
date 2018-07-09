#ifndef _SYS_UTSNAME_H
#define _SYS_UTSNAME_H

struct utsname
  {
    char sysname[256];
    char nodename[256];
    char release[256];
    char version[256];
    char machine[256];
    char domainname[256];
  };

int uname(struct utsname *name);

#endif
