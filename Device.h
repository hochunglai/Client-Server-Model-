#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>


#define CONTROLLER_FIFO_NAME "/tmp/serv_fifo"
#define CLOUD_FIFO_NAME "/tmp/clo_fifo"
#define DEVICE_FIFO_NAME "/tmp/cli_%d_fifo"
#define BUFFER_SIZE 20

struct data_to_pass_st {
	pid_t sensor_pid;
	char name[BUFFER_SIZE - 1];
	int threshold;

};

struct sensed_data_to_pass {
	pid_t sensor_pid;
	int random;

};



