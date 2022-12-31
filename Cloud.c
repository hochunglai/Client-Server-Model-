#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#include "Device.h"


int main()
{

	char curr_state[256];
	int cloud_fifo_fd, read_res;
    
	//Create and open Cloud FIFO
	mkfifo(CLOUD_FIFO_NAME, 0777);
	cloud_fifo_fd = open(CLOUD_FIFO_NAME, O_RDONLY);
	if (cloud_fifo_fd == -1) {
		fprintf(stderr, "Cloud fifo failure\n");
		exit(EXIT_FAILURE);
	}
	
	//Wait and Read from Cloud FIFO 
	do{
		read_res = read(cloud_fifo_fd, &curr_state, sizeof(curr_state));
		if(read_res>0){
			//Print out the current state of the device provided by the Controller
			printf("Hello User, the current state of your device (Air Conditioner) is %s\n", curr_state);	
		}
	}while(read_res>0);

	close(cloud_fifo_fd);
	unlink(CLOUD_FIFO_NAME);
	
}
