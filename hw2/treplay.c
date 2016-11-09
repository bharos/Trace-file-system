#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
/* indicates the corresponding bit number from LSB in the bimap for the operation */
enum log_types {
	check_readdir, check_open, check_read, check_write, check_close, check_mkdir, check_unlink, check_link, check_rename, check_rmdir
};
struct record
{
	unsigned int id;
	char type;
	int return_value;
	unsigned long file_address;
	int open_flags;
	int perm_mode;
	unsigned short path_length;
	char* path;
	unsigned short new_path_length;
	char* new_path;
	unsigned long data_length;
	char* data;
	unsigned int record_size;
};
int fd;
char cwd[4096];
char open_path[4096];
char new_open_path[4096];

/* Mapping of fd to file address */
typedef struct {
	unsigned long file_addr; // key
	int file_d;				 // value

} FILE_MAP;

FILE_MAP **map;
int map_index = 0;

int map_expand_limit = 10;

FILE_MAP* create_entry(unsigned long file_address, int fd)
{
	FILE_MAP *entry  = (FILE_MAP *)malloc(sizeof(FILE_MAP));
	entry->file_addr = file_address;
	entry->file_d = fd;
	return entry;
}
void map_print()
{
	printf("******MAP******\n------------------\n");
	int i;
	for (i = 0; i < map_index; i++)
	{
		printf("%d    	%d \n", map[i]->file_addr, map[i]->file_d);
	}
	printf("expand limit : %d\n", map_expand_limit);
}
void map_add(unsigned long file_address, int fd)
{
	if (map_index == map_expand_limit)
	{
		/* add 10 more places in the map */
		map_expand_limit += 10;
		map = (FILE_MAP **)realloc(map, sizeof(FILE_MAP *)*map_expand_limit);

		
	}
	map[map_index++] = create_entry(file_address, fd);
}
void map_delete()
{
	int i;
	if (map)
	{
		for (i = 0; i < map_index; i++)
		{
			free(map[i]);
		}
		free(map);
	}
}
int map_find(unsigned long file_address)
{
	int i;
	for (i = 0; i < map_index; i++)
	{

		if (map[i]->file_addr == file_address)
		{
			return map[i]->file_d;
		}
	}
	return -1;
}
int replay_record(struct record *r)
{
	int ret = 0;
	char* buf = NULL;
	switch (r->type)
	{
	case check_readdir:

		break;
	case check_open:

		open_path[0] = '\0';
		strcat(open_path, cwd);
		strcat(open_path, r->path);
		printf("OPEN_PATH : %s\n", open_path);

		fd = open(open_path, r->open_flags, r->perm_mode);
		if (fd < -1)
		{
			ret = -1;
		}
		else
		{
			ret = 0;
			map_add(r->file_address, fd);
		}
		printf("open fd %d\n", fd);
		break;
	case check_read:
		buf = (char *)malloc(r->data_length+1);
		fd = map_find(r->file_address);
		if (fd == -1)
		{
			ret = -1;
		}
		printf("read fd %d\n", fd);
		ret = read(fd, buf, r->data_length);
		buf[r->data_length] = '\0';
		//printf("Replay Read data = %s\n",buf);
		//printf("Kernel Read data = %s\n",r->data);
		break;
	case check_write:
		fd = map_find(r->file_address);
		if (fd == -1)
		{
			ret = -1;
		}
		printf("write fd %d\n", fd);
		ret = write(fd, r->data, r->data_length);
		break;
	case check_close:
		fd = map_find(r->file_address);
		if (fd == -1)
		{
			ret = -1;
		}
		printf("close fd %d\n", fd);
		ret = close(fd);
		break;
	case check_mkdir:
		open_path[0] = '\0';
		strcat(open_path, cwd);
		strcat(open_path, r->path);
		printf("OPEN_PATH : %s\n", open_path);

		ret = mkdir(open_path, r->perm_mode);
		break;
	case check_unlink:
		open_path[0] = '\0';
		strcat(open_path, cwd);
		strcat(open_path, r->path);
		printf("OPEN_PATH : %s\n", open_path);
		ret = unlink(open_path);
		break;
	case check_link:
		open_path[0] = '\0';
		new_open_path[0] = '\0';
		strcat(open_path, cwd);
		strcat(open_path, r->path);
		strcat(new_open_path, cwd);
		strcat(new_open_path, r->new_path);
		printf("OPEN_PATH : %s\n", open_path);
		printf("NEW OPEN_PATH : %s\n", new_open_path);
		ret = link(open_path, new_open_path);
		break;
	case check_rename:
		open_path[0] = '\0';
		new_open_path[0] = '\0';
		strcat(open_path, cwd);
		strcat(open_path, r->path);
		strcat(new_open_path, cwd);
		strcat(new_open_path, r->new_path);
		printf("OPEN_PATH : %s\n", open_path);
		printf("NEW OPEN_PATH : %s\n", new_open_path);
		ret = rename(open_path, new_open_path);
		break;
	case check_rmdir:
		open_path[0] = '\0';
		strcat(open_path, cwd);
		strcat(open_path, r->path);
		printf("OPEN_PATH : %s\n", open_path);
		ret = rmdir(open_path);
		break;
	default:
		printf("\nUnknown option to replay\n");
		break;
	}

	if (buf)
	{
		free(buf);
	}
	return ret;
}

int main(int argc, char* argv[])
{

	struct stat info;
	const char *filename = NULL;
	int replay_return;
	char show_flag = 0, strict_flag = 0;
	char opt;

	while ((opt = getopt(argc, argv, "ns")) != -1) {
		switch (opt) {
		case 'n':

			show_flag = 1;
			break;
		case 's':

			strict_flag = 1;
			break;
		default:
			fprintf(stderr, "Usage:./treplay [-ns] TFILE\n");
			exit(EXIT_FAILURE);
		}
	}

	if (show_flag == 1 && strict_flag == 1)
	{
		fprintf(stderr, "Usage:./treplay [-ns] TFILE\n");
		exit(EXIT_FAILURE);
	}

	if (optind >= argc) {
		fprintf(stderr, "Usage:./treplay [-ns] TFILE\n");
		exit(EXIT_FAILURE);
	}
	else {

		if (argc - optind == 1)
			filename = argv[optind]; // first file name is the output file
		else
		{
			fprintf(stderr, "Usage: ./treplay [-ns] TFILE\n");
			exit(EXIT_FAILURE);
		}

	}
	printf("%s\n", filename);

	if (stat(filename, &info) != 0) {

		printf("Error.Stat failed.\n");
		return;
	}
	printf("FILE SIZE: %lu\n", (unsigned long)info.st_size);


	map = (FILE_MAP **)malloc(sizeof(FILE_MAP *)*map_expand_limit);

	char *content = malloc(info.st_size);
	if (content == NULL) {

		printf("Error. Failed to allocate memory\n");
		return;
	}
	FILE *fp = fopen(filename, "rb");
	if (fp == NULL) {

		printf("Error. Failed to open file\n");
		return;
	}
	/* Try to read a single block of info.st_size bytes */
	size_t blocks_read = fread(content, info.st_size, 1, fp);
	if (blocks_read != 1) {

		printf("Error. Failed to read from file");
		fclose(fp);
		return;
	}
	char * p = NULL;
	struct record *r = (struct record *)malloc(sizeof(struct record));
	int offset = 0;
	unsigned long file_length = (unsigned long)info.st_size;
	getcwd(cwd, 4096);
	printf("CWD : %s\n", cwd);

	while (offset < file_length)
	{
		printf("---------------------------------------------------------\n");
		r->path = NULL;
		r->data = NULL;

		//get the id
		memcpy(&r->id, &content[offset], sizeof(r->id));
		offset += sizeof(r->id);

		//get the type
		memcpy(&r->type, &content[offset], sizeof(r->type));
		offset += sizeof(r->type);


		//get the return value
		memcpy(&r->return_value, &content[offset], sizeof(r->return_value));
		offset += sizeof(r->return_value);

		//get the file address
		memcpy(&r->file_address, &content[offset], sizeof(r->file_address));
		offset += sizeof(r->file_address);


		printf("id : %d \n", r->id);
		printf("type : %d \n", r->type);
		printf("Return value: %d\n", r->return_value);
		printf("File address: %d\n", r->file_address);
		printf("Operation : ");
		switch (r->type)
		{

		case check_readdir:
			printf("readdir\n");
			//get the path length
			memcpy(&r->path_length, &content[offset], sizeof(r->path_length));
			offset += sizeof(r->path_length);

			r->path = (char *)malloc(r->path_length + 1);

			//get the path name
			memcpy(r->path, &content[offset], r->path_length);
			offset += r->path_length;

			r->path[r->path_length] = '\0';

			printf("path_length : %d \n", r->path_length);
			printf("path : %s \n", r->path);
			break;

		case check_open:
			printf("open\n");
			//get the path length
			memcpy(&r->path_length, &content[offset], sizeof(r->path_length));
			offset += sizeof(r->path_length);

			r->path = (char *)malloc(r->path_length + 1);

			//get the path name
			memcpy(r->path, &content[offset], r->path_length);
			offset += r->path_length;

			r->path[r->path_length] = '\0';


			//get open flags
			memcpy(&r->open_flags, &content[offset], sizeof(r->open_flags));
			offset += sizeof(r->open_flags);

			//get permission mode
			memcpy(&r->perm_mode, &content[offset], sizeof(r->perm_mode));
			offset += sizeof(r->perm_mode);

			printf("open_flags : %d \n", r->open_flags);
			printf("perm mode : %d \n", r->perm_mode);
			printf("path_length : %d \n", r->path_length);
			printf("path : %s \n", r->path);

			break;

		case check_read:
				printf("read\n");

			//length of data read from file
			memcpy(&r->data_length, &content[offset], sizeof(r->data_length));
			offset += sizeof(r->data_length);

			r->data = (char *)malloc(r->data_length + 1);

			//log data read from file
			memcpy(r->data, &content[offset], r->data_length);
			offset += r->data_length;

			r->data[r->data_length] = '\0';

			printf("data length : %d\n", r->data_length);
			printf("r->data = %s\n", r->data);
			break;
		case check_write:
			printf("write\n");

			//length of data read from file
			memcpy(&r->data_length, &content[offset], sizeof(r->data_length));
			offset += sizeof(r->data_length);

			r->data = (char *)malloc(r->data_length + 1);

			//log data read from file
			memcpy(r->data, &content[offset], r->data_length);
			offset += r->data_length;

			r->data[r->data_length] = '\0';

			printf("data length : %d\n", r->data_length);
			printf("r->data = %s\n", r->data);
			break;
		case check_close:
			printf("Close\n");
			break;
		case check_mkdir:
			printf("mkdir\n");
			//get the path length
			memcpy(&r->path_length, &content[offset], sizeof(r->path_length));
			offset += sizeof(r->path_length);

			r->path = (char *)malloc(r->path_length + 1);

			//get the path name
			memcpy(r->path, &content[offset], r->path_length);
			offset += r->path_length;

			r->path[r->path_length] = '\0';
			//get permission mode
			memcpy(&r->perm_mode, &content[offset], sizeof(r->perm_mode));
			offset += sizeof(r->perm_mode);
			printf("perm mode : %d \n", r->perm_mode);
			printf("path_length : %d \n", r->path_length);
			printf("path : %s \n", r->path);
			break;
		case check_unlink:
			printf("unlink\n");
			//get the path length
			memcpy(&r->path_length, &content[offset], sizeof(r->path_length));
			offset += sizeof(r->path_length);

			r->path = (char *)malloc(r->path_length + 1);

			//get the path name
			memcpy(r->path, &content[offset], r->path_length);
			offset += r->path_length;

			r->path[r->path_length] = '\0';


			printf("path_length : %d \n", r->path_length);
			printf("path : %s \n", r->path);
			break;
		case check_link:
		printf("Link\n");
			//get the path length
			memcpy(&r->path_length, &content[offset], sizeof(r->path_length));
			offset += sizeof(r->path_length);

			r->path = (char *)malloc(r->path_length + 1);

			//get the path name
			memcpy(r->path, &content[offset], r->path_length);
			offset += r->path_length;

			r->path[r->path_length] = '\0';

			//get the new path length
			memcpy(&r->new_path_length, &content[offset], sizeof(r->new_path_length));
			offset += sizeof(r->new_path_length);

			r->new_path = (char *)malloc(r->new_path_length + 1);

			//get the new path name
			memcpy(r->new_path, &content[offset], r->new_path_length);
			offset += r->new_path_length;

			r->new_path[r->new_path_length] = '\0';


			printf("path_length : %d \n", r->new_path_length);
			printf("path : %s \n", r->new_path);
			break;
		case check_rename:
			printf("Rename\n");
			//get the path length
			memcpy(&r->path_length, &content[offset], sizeof(r->path_length));
			offset += sizeof(r->path_length);

			r->path = (char *)malloc(r->path_length + 1);

			//get the path name
			memcpy(r->path, &content[offset], r->path_length);
			offset += r->path_length;

			r->path[r->path_length] = '\0';

			//get the new path length
			memcpy(&r->new_path_length, &content[offset], sizeof(r->new_path_length));
			offset += sizeof(r->new_path_length);

			r->new_path = (char *)malloc(r->new_path_length + 1);

			//get the new path name
			memcpy(r->new_path, &content[offset], r->new_path_length);
			offset += r->new_path_length;

			r->new_path[r->new_path_length] = '\0';


			printf("path_length : %d \n", r->new_path_length);
			printf("path : %s \n", r->new_path);
			break;
		case check_rmdir:
			printf("rmdir\n");
			//get the path length
			memcpy(&r->path_length, &content[offset], sizeof(r->path_length));
			offset += sizeof(r->path_length);

			r->path = (char *)malloc(r->path_length + 1);

			//get the path name
			memcpy(r->path, &content[offset], r->path_length);
			offset += r->path_length;

			r->path[r->path_length] = '\0';
			printf("Path : %s\n",r->path);
			break;
		default :
			printf("\n Unknown record type in log %d\n", r->type);
		}

		if (show_flag == 0)
		{
			replay_return = replay_record(r);

			printf("replay return : %d\n", replay_return);
			printf("kernel return : %d\n", r->return_value);
			if (replay_return == r->return_value)
			{
				printf("\nSame result\n");
			}
			else
			{
				printf("\nDeviation in result\n");
				if (strict_flag == 1)
				{

					if (r->path)
					{
						free(r->path);
					}
					if (r->data)
					{
						free(r->data);
					}
					free(r);
					map_delete();
					fclose(fp);
					printf("Strict Mode : Aborting...\n");
					return;
				}
			}
		}
		if (r->path)
		{
			free(r->path);
		}
		if (r->data)
		{
			free(r->data);
		}

	}
	printf("---------------------------------------------------------\n");
	free(r);
	map_delete();
	fclose(fp);
	if (strict_flag == 1)
		printf("\nAll logs replayed\n");
}