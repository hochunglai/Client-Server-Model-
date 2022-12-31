#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <ctype.h>
#include <signal.h>

#include "Device.h"


char curr_state[256];

//Signal SIGUSR1 Handler
void on_state(){
	strcpy(curr_state,"on");
}

//Signal SIGUSR2 Handler
void off_state(){
	strcpy(curr_state,"on");
}


int main()
{
	
	//Initialization
	struct data_to_pass_st my_data;
	struct data_to_pass_st arr[3];
	struct sensed_data_to_pass sensed_data;
	int controller_fifo_fd, sensor_fifo_fd, actuator_fifo_fd, cloud_fifo_fd;
	int read_res;
	int over_count =0;
	int ack=1;
	int count =0;
	char sensor_fifo[256], mess[256], actuator_fifo[256];
	pid_t pid;
	
	
	printf("Fork program starting\n");

	//Starting fork
	pid = fork();

	switch(pid)
	{
	//Fork failed if pid = -1 
	case -1:
		perror("Fork failed\n");
		exit(1);

	case 0:
	//Child process if pid = 0 
		printf("Child Starting\n");

		//Create and open Controller FIFO
		mkfifo(CONTROLLER_FIFO_NAME, 0777);
		controller_fifo_fd = open(CONTROLLER_FIFO_NAME, O_RDONLY);
		if (controller_fifo_fd == -1) {
			fprintf(stderr, "Controller fifo failure\n");
			exit(EXIT_FAILURE);
		}

		printf("Waiting for devices\n");
		//Wait for Devices 
		sleep(15);

		//Obtain basic information from sensors and register them
		do{
			//Read from Controller FIFO
			read_res = read(controller_fifo_fd, &arr[count], sizeof(my_data));
			if(read_res>0){
				//Print information of sensor
				printf("\nSensor %d\n", arr[count].sensor_pid);
				printf("Name %s\n", arr[count].name);
				printf("Threshold %d\n", arr[count].threshold);

				//Remember and set up FIFO path for Actuator
				if(arr[count].threshold ==0){
					sprintf(actuator_fifo, DEVICE_FIFO_NAME, arr[count].sensor_pid);
				}

				count += 1;
			}
		}while(read_res>0);
		close(controller_fifo_fd);

		//Send ACK message to devices to signal them to start normal operations
		int arr_len = sizeof(arr)/sizeof(arr[0]);

		for(int i=0; i<(arr_len); i++){
			sprintf(sensor_fifo, DEVICE_FIFO_NAME, arr[i].sensor_pid);
			sensor_fifo_fd = open(sensor_fifo, O_WRONLY);
			if (sensor_fifo_fd == -1) {
				fprintf(stderr, "Sensor fifo failure\n");
				exit(EXIT_FAILURE);
			}
			write(sensor_fifo_fd, &ack, sizeof(ack));
			close(sensor_fifo_fd);
			printf("Sent ACK to %s sensor PID: %d\n", arr[i].name, arr[i].sensor_pid);
		}


		//Reopen Controller FIFO to read sensed data from the sensors
		controller_fifo_fd = open(CONTROLLER_FIFO_NAME, O_RDONLY);
		if (controller_fifo_fd == -1) {
			fprintf(stderr, "Controller fifo failure\n");
			exit(EXIT_FAILURE);
		}

		do{
			read_res = read(controller_fifo_fd, &sensed_data, sizeof(sensed_data));
			if(read_res>0){
				printf("\nSensed Data = %d\n", sensed_data.random);
				printf("Sensor PID %d\n", sensed_data.sensor_pid);

				//Compare the sensed data to the threshold previously stored based on the PID of sensor
				for(int i=0; i<(arr_len); i++){
					if(arr[i].sensor_pid == sensed_data.sensor_pid){
						//Take action if sensed data is greater than threshold
						if(sensed_data.random > arr[i].threshold){
							//Send a signal to stop sensor
							printf("Sensed Result %d > Threshold %d\n", sensed_data.random, arr[i].threshold);
							printf("Send signal to stop %s sensor, PID: %d\n", arr[i].name, arr[i].sensor_pid);
							kill(sensed_data.sensor_pid, SIGUSR1);
							printf("Done, Sent signal to stop %s sensor, PID: %d\n", arr[i].name, arr[i].sensor_pid);
							over_count +=1;
							//Send an alarm to actuator 
							actuator_fifo_fd = open(actuator_fifo, O_WRONLY | O_NONBLOCK);
							if (actuator_fifo_fd == -1) {
								fprintf(stderr, "Actuator fifo failure\n");
								exit(EXIT_FAILURE);
							}
							if(actuator_fifo_fd != -1){
								strcpy(mess, "alarm");
								printf("Sending alarm to Actuator\n");
								write(actuator_fifo_fd, &mess, sizeof(mess));
								printf("Done, sent alarm to Actuator\n");
							}

							//Only raise a signal when both sensors are over the threshold
							if(over_count == 2){
								printf("Sending signal to Parent\n");
								kill(getppid(), SIGUSR1);
								printf("Done, sent signal to Parent\n");
							}
						}
					}
				}
			}
		}while(read_res>0);
		close(controller_fifo_fd);
		unlink(CONTROLLER_FIFO_NAME);
		printf("Child done\n");
	default:
	//Parent process
		printf("Parent Starting\n");
		(void) signal(SIGUSR1, on_state);
		(void) signal(SIGUSR2, off_state);

		//Wait for signal
		pause();

		//Write current state to Cloud
		cloud_fifo_fd = open(CLOUD_FIFO_NAME, O_WRONLY);
		if (cloud_fifo_fd == -1) {
			fprintf(stderr, "Cloud fifo failure\n");
			exit(EXIT_FAILURE);
		}
		if(cloud_fifo_fd != -1){
			printf("Sending current state to Cloud\n");
			write(cloud_fifo_fd, &curr_state, sizeof(curr_state));
			printf("Done, Sent current state to Cloud\n");
		}
		printf("Parent done\n");
		close(cloud_fifo_fd);
		unlink(CLOUD_FIFO_NAME);
		break;
	}
	
	
}
