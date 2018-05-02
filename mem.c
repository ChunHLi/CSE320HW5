#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <fcntl.h>
#include <inttypes.h>
#include <sys/stat.h>
#include <unistd.h>

int emu_client;
int emu_server;
char *src_emu_client = "emu_client";
char *src_emu_server = "emu_server";
char *writ;
void* ram;
int thread_size[4] = {0,256,512,768};
int status = 1;

int main(int argc, char** argv){
	emu_server = mkfifo(src_emu_server,S_IWUSR | S_IRUSR | S_IRGRP | S_IROTH);
	if (emu_server < 0) {
        	printf("Unable to create fifo to emulated mem, may already exists\n");
        }
	emu_client = mkfifo(src_emu_client,S_IWUSR | S_IRUSR | S_IRGRP | S_IROTH);
	if (emu_client < 0) {
        	printf("Unable to create fifo to emulated mem, may already exists\n");
        }
	ram = calloc(1024, sizeof(char));
	char* buf = (char*)malloc(80*sizeof(char));
	while(status){
		emu_server = open(src_emu_server,O_RDONLY);
		read(emu_server,buf,80);
		close(emu_server);
		sleep(5);
		printf("passed sleep?\n");
		printf("%s\n",buf);
                int i = 0;
                char** args = (char**)calloc(4,sizeof(char*));
		int a;
		for (a = 0; a<4 ;a++){
			args[a]= (char *)calloc(32,sizeof(char));
		}
		char* token = strtok(buf,",");
                while (token != NULL){
                        args[i++] = strdup(token);
                        token = strtok(NULL, ",");
                }
		printf("args[0]: %s\n",args[0]);
		printf("args[1]: %s\n",args[1]);
		if (strcmp(args[0],"allo")==0){
			int id = atoi(args[1]);
			char physADDR[4];
			printf("Before sprintf\n");
			sprintf(physADDR,"%d",thread_size[id]);
			printf("Past sprintf\n");
			thread_size[id] = thread_size[id] + 4;
			writ = physADDR;
			printf("Got up to here somehow\n");
		} else if (strcmp(args[0],"read")==0 || strcmp(args[0],"write")==0){
			char* strADDR = args[1];
			int intADDR = atoi(strADDR);
			if (intADDR >= 1024){
				writ = "error,address out of range";
			} else if (intADDR % 4 != 0){
				writ = "error,address is not aligned";
			} else {
				if ((intADDR < 256 && thread_size[0] <= intADDR) || 
				((intADDR >= 256 && intADDR < 512) && thread_size[1] <= intADDR) ||
				((intADDR >= 512 && intADDR < 768) && thread_size[2] <= intADDR) ||
				((intADDR >= 768 && intADDR < 1024 && thread_size[3] <= intADDR))) {
					writ = "error,address out of range";
				}
				if (strcmp(args[0],"read")==0){
					int value = ((int*)ram)[intADDR/4];
                                	char strValue[16];
                                	sprintf(strValue,"%d",value);
                                	writ = strValue;
				} else {
					int val = atoi(args[2]);
                                	((int*)ram)[intADDR / 4] = val;
					printf("Value: %d\n",((int*)ram)[intADDR/4]);
                                	writ = "writ_succ_";
				}
			}
		} else if (strcmp(args[0],"kill")==0){
			int pid = atoi(args[1]);
			int offset;
			for (offset=0;offset<64;offset++){
				((int*)ram)[pid*64+offset] = 0;
			}
		} else if (strcmp(args[0],"exit")==0){
                        printf("Exited\n");
                        status = 0;
			goto free;	
		} else {
			printf("Error, invalid command\n");
		}
		emu_client = open(src_emu_client,O_WRONLY);
                if (emu_client < 0){
                	printf("Error in opening file\n");
                	status = 0;
		}
		write(emu_client, writ, 28*sizeof(char));
		close(emu_client);
		i = 0;
                while (i < 4){
                        free(args[i]);
                	i += 1;
                }
		free(args);
		memset(buf,0,80); 
	}
	free:
	free(buf);
	free(ram);
}
