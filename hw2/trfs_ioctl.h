#include <sys/ioctl.h>

#define TRFS_MAGIC 's'             
#define TRFS_IOCSETALL  _IOW(TRFS_MAGIC, 1 , char *) //set  data   
#define TRFS_IOCSETOPT  _IOW(TRFS_MAGIC, 2 , char *) //set  data     
#define TRFS_IOCGETOPT  _IOR(TRFS_MAGIC, 3 , char *) //get  data


