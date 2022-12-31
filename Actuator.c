#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdbool.h>

#include "Device.h"


int main(int argc, char *argv[])
{
	int controller_fifo_fd, actuator_fifo_fd, read_res;
	int over_count =0;
	int ack;
	char mess[256], actuator_fifo[256];
	struct data_to_pass_st my_data;

	//Initialize basic information
	strcpy(my_data.name, argv[1]);
	my_data.sensor_pid = getpid();
	my_data.threshold = 0;

	//Open Controller FIFO
	controller_fifo_fd = open(CONTROLLER_FIFO_NAME, O_WRONLY);
	if (controller_fifo_fd == -1) {
		fprintf(stderr, "Controller fifo failure\n");
		exit(EXIT_FAILURE);
	}

	printf("Sending basic information to Controller\n");
	//Writing basic information to Controller
	write(controller_fifo_fd, &my_data, sizeof(my_data));
	close(controller_fifo_fd);
	printf("Finished Sending basic information to Controller\n");
	
	//Set up and create Actuator FIFO
	sprintf(actuator_fifo, DEVICE_FIFO_NAME, my_data.sensor_pid);
	mkfifo(actuator_fifo, 0777);

	printf("Waiting for Controller's ACK message\n");
	//Open Actuator FIFO and read ACK message 
	actuator_fifo_fd = open(actuator_fifo, O_RDONLY);
	if (actuator_fifo_fd == -1) {
		fprintf(stderr, "Actuator fifo failure\n");
		exit(EXIT_FAILURE);
	}
	
	read(actuator_fifo_fd, &ack, sizeof(ack));
	close(actuator_fifo_fd);
	printf("Obtained Controller's ACK message\n");

	printf("Starting normal operation\n");
	//Start normal operation when ACK message is received
	actuator_fifo_fd = open(actuator_fifo, O_RDONLY);
	if (actuator_fifo_fd == -1) {
		fprintf(stderr, "Actuator fifo failure\n");
		exit(EXIT_FAILURE);
	}

	printf("Reading alarm from Controller\n");
	while(ack && over_count!=2){
		read_res = read(actuator_fifo_fd, &mess, sizeof(mess));
    	if(strcmp(mess, "alarm")==0){
			over_count += 1;
		}
	}

	//Only print when both sensors are over threshold
	printf("\n%s is ON\n", my_data.name);
	printf("Done");
	close(actuator_fifo_fd);
	unlink(actuator_fifo);
    	
}
