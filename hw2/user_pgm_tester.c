#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdint.h>

pthread_mutex_t lock;
void execute_operations(void * arg)
{
	FILE *fptr;
	char error = 0;
	char opened_in_read  = 1;
	char buf[12000];
	char buf2[500];
	int fd;
	ssize_t ret_read, ret_write;
	char * filename1 = "/mnt/trfs/bigDirectoryName/abracadabra.txt";
	char * filename2 = "/mnt/trfs/bigDirectoryName/abracadabra2.txt";

		mkdir("/mnt/trfs/bharathkrishna",0644);
		link("/mnt/trfs/abc.txt","/mnt/trfs/efg.txt");
		rename("/mnt/trfs/abc.txt","/mnt/trfs/xyz.txt");

pthread_mutex_lock(&lock);
int fd2 = open("content.txt", O_RDONLY, 0644);
	read(fd2, &buf, 12000);
	close(fd2);

strcpy(buf2, "abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyz");


	fd = open(filename1 , O_RDWR | O_CREAT | O_TRUNC, 0644);
	if (fd == -1) {
		perror ("open1");
		return;
	}
	
	
printf("READ BUF =  %s", buf);
 	ret_write = write(fd, &buf, strlen(buf));


	// int i = 0;

	// int end = (intptr_t)arg;
	// printf("end = %d", end);
	// while (i < end )
	// {
	// 	usleep(1);
	// 	printf("threadFunc says: %d\n", end);
	// 	++i;
	// }

	close(fd);
	fd = open(filename1, O_RDONLY | O_CREAT , 0644);
	if (fd == -1) {
		perror ("open");
		return;
	}
	ret_read = read(fd, &buf, ret_write);
	//printf("read : %s \n", buf);

	close(fd);

	fd = open(filename2, O_RDWR  | O_CREAT , 0644);
	ret_write = write(fd, &buf2, strlen(buf2));
	close(fd);
	printf("ret_write= %d",ret_write);
	fd = open(filename2, O_RDONLY, 0644);
	if (fd == -1) {
		perror ("open");
		return;
	}
	ret_read = read(fd, &buf2, ret_write);
	printf("read : %s \n", buf2);

	close(fd);

	pthread_mutex_unlock(&lock);

//	fd = open("/mnt/trfs/Blah/blah/blah/blah/blah.txt", O_RDWR, 0644);
// if (fd == -1) {
// 		perror ("open2");
// 		return;
// 	}
// 	close(fd);

}
void main()
{
	pthread_t pth1;	//thread identifier
	pthread_t pth2;	//thread identifier
	pthread_t pth3;	//thread identifier
	int i = 0;
//	pthread_create(&pth1, NULL, execute_operations, (void *) (intptr_t) 30);

//	pthread_create(&pth2, NULL, execute_operations, (void *) (intptr_t) 20);
//	pthread_create(&pth3, NULL, execute_operations, (void *) (intptr_t) 10);
	while (i < 30 )
	{
		usleep(1);
		printf("main() is running...\n");
		++i;
	}
	execute_operations(NULL);
	/* wait for our thread to finish before continuing */

}