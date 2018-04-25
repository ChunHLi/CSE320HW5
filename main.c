#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <fcntl.h>
#include <inttypes.h>
#include <sys/stat.h>
#include <unistd.h>

void *thread_func(void *vargp){
	int *myid = (int *)vargp;
	char* id;
	sprintf(id, "%d", *myid);
	int virt_len = 0;
	int* virt_addr;
	int* pt_1;
	int* pt_2;
	int fifo_server;
	int fifo_client;
	char *src_server;
	char *src_client;
	char *buf;
	char *writ;
	char** args;	
	while (1) {
		src_server = "fifo_server_";
		strcat(src_server, id);
		fifo_server = open(src_server,O_RDWR);
		if (fifo_server<0) {
			printf("Error opening file\n");
		}
		struct stat buff;
                fstat(fifo_client, &buff);
                int size = buff.st_size;
                buf = malloc(size*sizeof(char));
		read(fifo_server,buf,size*sizeof(char));
		src_client = "fifo_client_";
		strcat(src_client, id);
		fifo_client = open(src_client, O_RDWR);
		if (fifo_client<0) {
			printf("Error opening file\n");
		}
		char* token;
                int i = 0;
               	for (token=strtok(buf," "); token != NULL; token=strtok(NULL, " ")){
                	args[i] = strdup(token);
                       	i += 1;
                }
                args[i] = NULL;
		if (strcmp(args[0],"kill")==0){
			writ = "kill_succ_";
			write(fifo_client,writ,10*sizeof(char));
			printf("Process Killed\n");
			i -= 1;
                	while (i >= 0){
                        	free(args[i]);
                        	i -= 1;
			}
			free(buf);
			goto kill;
                } else if (strcmp(args[0],"memo")==0){
			writ = "memo_succ_";
			write(fifo_client,writ,10*sizeof(char));
			printf("\nAddresses within the process %s:\n", id);
			int m;
			for (m=0; m<virt_len ;m++){
				printf("%#08x\n",virt_addr[m]);
			}
		} else if (strcmp(args[0],"allo")==0){
			virt_len += 1;
		} else if (strcmp(args[0],"read")==0){
		} else if (strcmp(args[0],"write")==0){
		} else {
			printf("Error, invalid command\n");
		}
		i -= 1;
                while (i >= 0){
                	free(args[i]);
                        i -= 1;
                }
		free(buf);	
	}
	kill:
		pthread_exit(NULL);
	
		
}

int main(int argc, char** argv){
	if (argc != 1){
		printf("Invalid number of arguments (expected 1)\n");
	} else {
		int status = 1;
		char str[255];
		pthread_t processes[4];
		int fifo_server;
		int fifo_client;
		char *src_server; 
		char *src_client;
		char *tid;
		char *buf;
		do {
			
			char** args;
			printf("> ");
			fgets(str,255,stdin);
			char* p = strchr(str,'\n');
                        if (p != NULL){
                                str[p-str]='\0';
                        }
			if ( strcmp(str,"") == 0 ){
			} else if ( strcmp(str,"create") == 0){
				int i;
				for (i = 0; i < 4; i++){
					if (processes[i] != -1) {	
						char *server = "fifo_server_";
						char *s;
						sprintf(s,"%d",i);
						strcat(server, s);
						int thread_process = mkfifo(server, S_IWUSR | S_IRUSR | S_IRGRP | S_IROTH);
						if (thread_process < 0){
							printf("Unable to create a fifo\n");
							exit(-1);
						}
						char *client = "fifo_client_";
						char *c;
						sprintf(c,"%d",i);
						strcat(client, s);
						int main_process = mkfifo(client,S_IWUSR | S_IRUSR | S_IRGRP | S_IROTH);
						if (main_process < 0){
							printf("Unable to create a fifo\n");
							exit(-1);
						}
						pthread_create(&processes[i],NULL,thread_func, (void*)i);
						break;
					}
				}
			} else if ( strcmp(str,"list") == 0){
				printf("\nCurrently Running Processes: \n");
				int j;
				for (j = 0; j < 4; j++){
					if (processes[j]){
						printf("Processa ID: %d\n",j);
					}
				}
			} else if ( strcmp(str,"exit") == 0){
				status = 0;
			} else {
				
				char* token;
                                int i = 0;
                                for (token=strtok(str," "); token != NULL; token=strtok(NULL, " ")){
                                        args[i] = strdup(token);
                                        i += 1;
                                }
                                args[i] = NULL;
				if ( strcmp(args[0],"kill") == 0){
					if (i != 2){
						printf("Invalid number of arguments: Expected 2\n");
					} else {
						src_server = "fifo_server_";
						src_client = "fifo_client_";
						tid = args[1];
						strcat(src_server,tid);
						strcat(src_client,tid);
						if (access(src_server, F_OK) == -1 || access(src_client, F_OK) == -1 ){
							printf("Thread does not exist\n");
						} else {
							fifo_server = open(src_server,O_RDWR);
							if (fifo_server < 0){
								printf("Error in opening file\n");
								exit(-1);
							}
							write(fifo_server, "kill",4*sizeof(char));
							sleep(10);
							fifo_client = open(src_client,O_RDWR);
							if (fifo_client < 0){
								printf("Error in opening file\n");
								exit(-1);
							}
							buf = malloc(10*sizeof(char));
							read(fifo_client, buf, 10*sizeof(char));
							if (strcmp(buf, "kill_succ_") == 0){
								int tidi = atoi(tid);
								free(buf);
								processes[tidi] = -1;
							} else {
								printf("Error in killing thread\n");
								free(buf);
								exit(-1);
							}
							
						}
					}	
				} else if ( strcmp(args[0],"mem") == 0){
					if (i != 2){
                                                printf("Invalid number of arguments: Expected 2\n");
                                        } else {
                                                src_server = "fifo_server_";
                                                src_client = "fifo_client_";
                                                tid = args[1];
                                                strcat(src_server,tid);
                                                strcat(src_client,tid);
                                                if (access(src_server, F_OK) == -1 || access(src_client, F_OK) == -1 ){
                                                        printf("Thread does not exist\n");
                                                } else {
                                                        fifo_server = open(src_server,O_RDWR);
                                                        if (fifo_server < 0){
                                                                printf("Error in opening file\n");
                                                                exit(-1);
                                                        }
                                                        write(fifo_server, "memo",4*sizeof(char));
							sleep(10);
                                                        fifo_client = open(src_client,O_RDWR);
                                                        if (fifo_client < 0){
                                                                printf("Error in opening file\n");
                                                                exit(-1);
                                                        }
							buf = malloc(10*sizeof(char));
                                                        read(fifo_client, buf, 10*sizeof(char));
                                                        if (strcmp(buf, "memo_succ_") == 0){
                                                                int tidi = atoi(tid);
                                                                free(buf);
                                                                processes[tidi] = -1;
                                                        } else {
                                                                printf("Error in printing mem\n");
                                                                free(buf);
                                                                exit(-1);
                                                        }
                                                        
                                                }
                                        }
				} else if ( strcmp(args[0],"allocate") == 0){
					if (i != 2){
                                                printf("Invalid number of arguments: Expected 2\n");
                                        } else {
                                                src_server = "fifo_server_";
                                                src_client = "fifo_client_";
                                                tid = args[1];
                                                strcat(src_server,tid);
                                                strcat(src_client,tid);
                                                if (access(src_server, F_OK) == -1 || access(src_client, F_OK) == -1 ){
                                                        printf("Thread does not exist\n");
                                                } else {
                                                        fifo_server = open(src_server,O_RDWR);
                                                        if (fifo_server < 0){
                                                                printf("Error in opening file\n");
                                                                exit(-1);
                                                        }
                                                        write(fifo_server, "allo",4*sizeof(char));
							sleep(10);
                                                        fifo_client = open(src_client,O_RDWR);
                                                        if (fifo_client < 0){
                                                                printf("Error in opening file\n");
                                                                exit(-1);
                                                        }
							buf = malloc(10*sizeof(char));
                                                        read(fifo_client, buf, 10*sizeof(char));
                                                        if (strcmp(buf, "allo_succ_") == 0){
								free(buf);
                                                        } else {
                                                                printf("Error in allocating\n");
                                                                free(buf);
                                                                exit(-1);
                                                        }
                                                        
                                                        
                                                }
                                        }
				} else if ( strcmp(args[0],"read") == 0){
					if (i != 3){
                                                printf("Invalid number of arguments: Expected 3\n");
                                        } else {
                                                src_server = "fifo_server_";
                                                src_client = "fifo_client_";
                                                tid = args[1];
                                                strcat(src_server,tid);
                                                strcat(src_client,tid);
                                                if (access(src_server, F_OK) == -1 || access(src_client, F_OK) == -1 ){
                                                        printf("Thread does not exist\n");
                                                } else {
                                                        fifo_server = open(src_server,O_RDWR);
                                                        if (fifo_server < 0){
                                                                printf("Error in opening file\n");
                                                                exit(-1);
                                                        }
							
							char* cmdr = "read ";
							char* virt_addrr = args[2];
							strcat(cmdr, virt_addrr);
                                                        write(fifo_server, cmdr,strlen(cmdr)*sizeof(char));
							sleep(10);
                                                        fifo_client = open(src_client,O_RDWR);
                                                        if (fifo_client < 0){
                                                                printf("Error in opening file\n");
                                                                exit(-1);
                                                        }
                                                        struct stat buffr;
                                                        fstat(fifo_client, &buffr);
                                                        int size = buffr.st_size;
                                                        buf = malloc(size*sizeof(char));
                                                        read(fifo_client, buf, size*sizeof(char));
                                                        printf("Value: %s\n",buf);
                                                        free(buf);
                                                        
                                                        
                                                }
                                        }
				} else if ( strcmp(args[0],"write") == 0){
					if (i != 4){
                                                printf("Invalid number of arguments: Expected 4\n");
                                        } else {
                                                src_server = "fifo_server_";
                                                src_client = "fifo_client_";
                                                tid = args[1];
                                                strcat(src_server,tid);
                                                strcat(src_client,tid);
                                                if (access(src_server, F_OK) == -1 || access(src_client, F_OK) == -1 ){
                                                        printf("Thread does not exist\n");
                                                } else {
                                                        fifo_server = open(src_server,O_RDWR);
                                                        if (fifo_server < 0){
                                                                printf("Error in opening file\n");
                                                                exit(-1);
                                                        }
                                                
                                                        char* cmdw = "write ";
                                                        char* virt_addrw = args[2];
							char* value = args[3];
							char* tmp = " ";
                                                        strcat(cmdw, virt_addrw);
							strcat(cmdw, tmp);
							strcat(cmdw, value);
                                                        write(fifo_server, cmdw,strlen(cmdw)*sizeof(char));
							sleep(10);
                                                        fifo_client = open(src_client,O_RDWR);
                                                        if (fifo_client < 0){
                                                                printf("Error in opening file\n");
                                                                exit(-1);
                                                        }
                                                        buf = malloc(10*sizeof(char));
                                                        read(fifo_client, buf, 10*sizeof(char));
                                                        if (strcmp(buf, "writ_succ_") == 0){
                                                                free(buf);
                                                        } else {
                                                                printf("Error in writing\n");
                                                                free(buf);
                                                                exit(-1);
                                                        }
                                                 
                                                 
                                                }
                                        }
				} else {
					printf("Invalid command: %s not recognized\n", args[0]);
				}
				i -= 1;
                                while (i >= 0){
                                        free(args[i]);
                                        i -= 1;
                                } 
			}
			
		} while (status);
	}
}
