#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <limits.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdbool.h>
#include <signal.h>

#include "Device.h"


int ack;

void stop_send(){
	ack =0;
}

int main(int argc, char *argv[])
{
	(void) signal(SIGUSR1, stop_send);

	int controller_fifo_fd, sensor_fifo_fd;
	struct data_to_pass_st my_data;
	struct sensed_data_to_pass sensed_data;
	char sensor_fifo[256];

	//Initialize basic information
	strcpy(my_data.name, argv[1]);
	my_data.threshold = atoi(argv[2]);
	my_data.sensor_pid = getpid();

	printf("Sending basic information to Controller\n");
	//Writing basic information to Controller
	controller_fifo_fd = open(CONTROLLER_FIFO_NAME, O_WRONLY);
	if (controller_fifo_fd == -1) {
		fprintf(stderr, "Controller fifo failure\n");
		exit(EXIT_FAILURE);
	}
	write(controller_fifo_fd, &my_data, sizeof(my_data));
	close(controller_fifo_fd);
	printf("Finished Sending basic information to Controller\n");


	//Set and create sensor fifo name
	sprintf(sensor_fifo, DEVICE_FIFO_NAME, my_data.sensor_pid);
	mkfifo(sensor_fifo, 0777);

	printf("Waiting for Controller's ACK message\n");
	//Wait for Controller's ACK message
	sensor_fifo_fd = open(sensor_fifo, O_RDONLY);
	if (sensor_fifo_fd == -1) {
		fprintf(stderr, "Sensor fifo failure\n");
		exit(EXIT_FAILURE);
	}	
	read(sensor_fifo_fd, &ack, sizeof(ack));
	close(sensor_fifo_fd);
	printf("Obtained Controller's ACK message\n");

	printf("Starting normal operation\n");
	//Reopen Controller FIFO
	controller_fifo_fd = open(CONTROLLER_FIFO_NAME, O_WRONLY);
	if (controller_fifo_fd == -1) {
		fprintf(stderr, "Controller fifo failure\n");
		exit(EXIT_FAILURE);
	}

	//Initialize PID
	sensed_data.sensor_pid = getpid();
	
	printf("Sending sensed result to Controller\n");
	//Send sensed result to Controller
	while(ack){
		sensed_data.random = rand() % 50;
		write(controller_fifo_fd, &sensed_data, sizeof(sensed_data));
		sleep(1);
	}

	printf("Done\n");

}
