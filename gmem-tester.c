/* gmem-tester application for gmem-driver kernel module
 * Author : Rohit Khanna
 *
 *
 */
#include <stdio.h>
#include <fcntl.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#define CB_SIZE 256									// define the circular buffer size, 256 in our case

int main(int argc, char **argv)
{
	int fd1, i;
	int res;
	char rd_buf[CB_SIZE];							// To read the buffer from driver
	int ret=0;


	if(argc < 2 || argc > 3){						// can only be 2 or 3
		ret = -1;
		goto usage;
	}


	/* open device */
	fd1 = open("/dev/gmem", O_RDWR);
	if (fd1<0 )
	{
		printf("Can not open device file.\n");
		return 0;
	}
	printf("%s: open: successful\n", argv[0]);

	if (strcmp(argv[1], "show") == 0){				// Read CB if show

		for(i=0; i<CB_SIZE; i++)					//Init the read buffer with blank characters
			rd_buf[i] = ' ';

		/* Issue a read */
		res = read(fd1, rd_buf, CB_SIZE);
		if ( res == -1 ) {
			printf("read failed");
			close(fd1);
			exit(-1);

		}
		printf("%s: read: returns with\n<<<%s>>>\n\n", argv[0], rd_buf);

	}
	else if( strcmp(argv[1], "write") == 0 ){		// Write to CB if write
			if(argc != 3){
				ret = -1;
				goto usage;
			}

			//printf("string to write = {{{%s}}}\n", argv[2]);

			res = write(fd1, argv[2], strlen(argv[2])+1);		/* Issue a write */
			if(res == strlen(argv[2])+1){
				printf("Can not write to the device file.\n");
				close(fd1);
				exit(-1);
			}
		printf("%s: write: returns with\n<<<%d>>>\n\n", argv[0], res);
		printf("\n\n");
	}
	else{
		ret = -1;
		goto usage;
	}


	usage:														// Label to handle erroneous run args
	if(ret == -1)
		printf("\nUSAGE : \n"
				"\t./gmem_tester show -- To print the content of the kernel buffer of the character device\n"
				"\t./gmem_tester write <your_string> -- To write <your_string> into the kernel buffer of the device\n\n");


	printf("closing file descriptor..\n");
	close(fd1);
	printf("file descriptor closed..\n");
	printf("PROGRAM ENDS...\n");

	return 0;
}
