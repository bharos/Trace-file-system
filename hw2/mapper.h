
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