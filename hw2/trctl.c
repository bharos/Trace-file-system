#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include "trfs_ioctl.h"

/* indicates the corresponding bit number from LSB in the bimap for the operation */
enum log_types {
	check_readdir, check_open, check_read, check_write, check_close, check_mkdir, check_unlink, check_link, check_rename, check_rmdir
};


void print_help()
{
	printf("\n\nUSAGE\n-----\n");
	printf("./trctl /mounted/path\n");
	printf("Retrieves the current value of the bitmap and display it in hex.\n\n");
	printf("./trctl all /mounted/path\n");
	printf("Sets all values in the bitmap (traces all operations).\n\n");
	printf("./trctl none /mounted/path\n");
	printf("Resets all values in the bitmap (Will not trace any operation).\n\n");
	printf("./trctl 0xNN /mounted/path\n");
	printf("Sets the bitmap with the hex value 0xNN.\n\n");
}
int validate_hex(const char* hex)
{

	if (strlen(hex) < 3)return 0;
	if (*hex == '0' && *(hex + 1) == 'x')
	{
		hex += 2;
	}
	else
	{
		return 0;
	}
	if (hex[strspn(hex, "0123456789abcdefABCDEF")] == 0)
	{
		return 1;
	}
	return 0;
}

int main(int argc, char **argv)
{
	char *mount_point = NULL;
	int result = 0;
	int fd;
	unsigned long long* x = NULL;

	if (argc == 1 || argc > 3)
	{
		print_help();
		return -1;
	}

	if (argc == 2)
	{

		mount_point = argv[1];
	}
	else
	{
		mount_point = argv[2];
	}

	//open mount and get fd
	fd = open(mount_point, O_RDONLY);
	if (fd < 0)
	{
		printf("Error: Could not open %s \n", mount_point);

		print_help();
		return -1;
	}

	if (argc == 2)
	{
		//passed only mount path. Get the bitmap from ioctl
		x = (unsigned long long *)malloc(sizeof(unsigned long long*));
		ioctl(fd, TRFS_IOCGETOPT, x);
		printf("\nBitmap : 0X%x\n", *x);
	}
	else if (strcasecmp(argv[1], "all") == 0)
	{
		//call ioctl to set all values in bitmap to 1 (no need to pass anything)
		 ioctl(fd, TRFS_IOCSETALL, 0);
		 printf("All bits set in bitmap\n");
	}
	else if (strcasecmp(argv[1], "none") == 0)
	{
		//call ioctl to set all values in bitmap to 0 (no need to pass anything)
		 ioctl(fd, TRFS_IOCSETOPT, 0);
		 printf("All bits reset in bitmap\n");
	}
	else if (validate_hex(argv[1]))
	{
		//call ioctl to set values in bitmap to 0xNN (given in argv)
		 ioctl(fd, TRFS_IOCSETOPT, strtoul(argv[1], NULL, 0));
		 printf("Bitmap set to : 0X%x\n",strtoul(argv[1], NULL, 0));
	}
	else
	{
		printf("\nInvalid usage");
		print_help();
		return -1;
	}

	printf("Bitmap operation done\n");

	close(fd);

	return 0;

}