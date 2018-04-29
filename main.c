#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <fcntl.h>
#include <inttypes.h>
#include <sys/stat.h>
#include <unistd.h>

pthread_t processes[4] = {-1,-1,-1,-1};

unsigned long cse320_malloc(unsigned long pt_1[64], unsigned long pt_2[64],char* myid){
	char *src_server = "emu_server";
	char *src_client = "emu_client";
	char *allo = "allo,";
	char *buf;
	strcat(allo,myid);
	if (access(src_server, F_OK) == -1 || access(src_client, F_OK) == -1 ){
		printf("Emulated Memory not found\n");
		exit(-1);
        } else {
		int emu_server = open(src_server,O_RDWR);
                if (emu_server < 0){
			printf("Error in opening file\n");
			exit(-1);
		}
		write(emu_server,allo,255*sizeof(char));
                int emu_client = open(src_client,O_RDWR);
		if (emu_client < 0){
			printf("Error in opening file\n");
			exit(-1);
                }
                buf = malloc(4*sizeof(char));
                read(emu_client, buf, 4*sizeof(char));
		if (strcmp(buf,"error,address out of range")==0 || strcmp(buf,"error,address is not aligned")==0){
			printf("%s\n",buf);
			return -1;
		}
		int physAddr = atoi(buf);
		int i = 0;
		for (i; i < 64 ;i++){
			if (pt_2[i] < 512){
				break;
			}
		}
		pt_2[i] = (unsigned long)(512 + physAddr);
		int j = 0;
		for (j; j < 64; j++){
			if (pt_1[j] < 512){
				break;
			}
		}
		pt_1[j] = (unsigned long)(512 + i);
		unsigned long pt1 = pt_1[j];
		pt1 = pt1 << 22;
		unsigned long pt2 = pt_2[i];
		pt2 = pt2 << 12;
		unsigned long va = pt1 | pt2;
                free(buf);
		return va;                        
	}
	return -1;
}

unsigned long cse320_virt_to_phys(unsigned long va){
	if (va > 0xFFFFF000) {
		printf("Invalid virtual address; address out of range");
	} else {
		unsigned long pa = ((0x003FF000 & va) >> 12);
		if (pa < 512){
			printf("Address not allocated\n");
		} else {
			return (pa - 512);
		}
	}
	return -1;
}

void *thread_func(void *vargp){
	pthread_t pid = pthread_self();
	int myid = -1;
	int pc;
	for (pc = 0; pc<4 ;pc++){
		if (processes[pc] == pid){
			myid = pc;
		}
	}
	char* sid;
	sprintf(sid, "%d",myid);
	char* id;
	sprintf(id, "%lu", pid);
	int virt_len = 0;
	unsigned long virt_addr[64];
	unsigned long pt_1[64];
	unsigned long pt_2[64];
	int p;
	for (p=0;p<64;p++){
		virt_addr[p] = 0;
		pt_1[p] = 0;
		pt_2[p] = 0;
	}
	int emu_server;
	int emu_client;
	int fifo_server;
	int fifo_client;
	char *src_emu_server = "emu_server";
	char *src_emu_client = "emu_client";
	char *src_server = "fifo_server_";
	strcat(src_server, id);
	char *src_client = "fifo_client_";
	strcat(src_client, id);
	char *buf;
	char *writ;
	char** args;	
	while (1) {
		fifo_server = open(src_server,O_RDWR);
		if (fifo_server<0) {
			printf("Error opening file\n");
			
		}
                buf = malloc(255*sizeof(char));
		read(fifo_server,buf,255*sizeof(char));
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
			printf("Process Killed\n");
			i -= 1;
                	while (i >= 0){
                        	free(args[i]);
                        	i -= 1;
			}
			if (access(src_emu_server, F_OK) == -1 || access(src_emu_client, F_OK) == -1 ){
				printf("Emulated Memory not found\n");
                                exit(-1);
                        } else {
				char *k = "kill,";
				strcat(k,sid);
                        	emu_server = open(src_emu_server,O_RDWR);
                                if (emu_server < 0){
                                	printf("Error in opening file\n");
                                        exit(-1);
                                }
                                write(emu_server,k,6*sizeof(char));
			}
			free(buf);
			write(fifo_client,writ,10*sizeof(char));
			close(fifo_server);
        		close(fifo_client);
        		close(emu_server);
        		close(emu_client);
			goto kill;
                } else if (strcmp(args[0],"memo")==0){
			writ = "memo_succ_";
			printf("\nAddresses within the process %s:\n", id);
			int m;
			for (m=0; m<virt_len ;m++){
				printf("%lu\n",virt_addr[m]);
			}
			write(fifo_client,writ,10*sizeof(char));
		} else if (strcmp(args[0],"allo")==0){
			unsigned long va = cse320_malloc(pt_1,pt_2,sid);
			if (va == 0xFFFFFFFF) {
				writ = "allo_fail_";
				write(fifo_client,writ,10*sizeof(char));		
			} else {
				writ = "allo_succ_";
				virt_addr[virt_len] = va;
				virt_len += 1;
				write(fifo_client,writ,10*sizeof(char));
			}
		} else if (strcmp(args[0],"read")==0){
			char *strVA;
			unsigned long va = strtoul(args[1], &strVA, 10);
			unsigned long pa = cse320_virt_to_phys(va);
			if (pa > 0xFFFFF000){
			} else {
				if (pa < myid * 256 || pa >= (myid + 1)* 256){
					printf("error,address out of range");
					writ = "NULL";
					write(fifo_client,writ,4*sizeof(char));
				} else {
					char *r = "read,";
					char *physADDR;
					sprintf(physADDR,"%lu",pa);
					strcat(r, physADDR);
					if (access(src_emu_server, F_OK) == -1 || access(src_emu_client, F_OK) == -1 ){
                                		printf("Emulated Memory not found\n");
                                		exit(-1);
                        		} else {
                                		emu_server = open(src_emu_server,O_RDWR);
                                		if (emu_server < 0){
                                        		printf("Error in opening file\n");
                                        		exit(-1);
                                		}
                                		write(emu_server,r,9*sizeof(char));
                                		emu_client = open(src_emu_client,O_RDWR);
                                		if (emu_client < 0){
                                        		printf("Error in opening file\n");
                                        		exit(-1);
                                		}
                                		char * bufff = malloc(28*sizeof(char));
                                		read(emu_client, bufff, 28*sizeof(char));
						write(fifo_client, bufff, 28*sizeof(char));
                        			free(bufff);
					}
				}
			}	
		} else if (strcmp(args[0],"write")==0){
			char *strVA;
                        unsigned long va = strtoul(args[1], &strVA, 10);
                        unsigned long pa = cse320_virt_to_phys(va);
                        if (pa > 0xFFFFF000){
                        } else {
                                if (pa < myid * 256 || pa >= (myid + 1) * 256){
                                        printf("error,address out of range");
                                        writ = "writ_fail_";
                                        write(fifo_client,writ,10*sizeof(char));
                                } else {
					char *w = "write,";
					char *del = ",";
					char *physADDR;
					sprintf(physADDR,"%lu",pa);
                                        strcat(w, physADDR);
					strcat(w, del);
					strcat(w, args[2]);
                                        if (access(src_emu_server, F_OK) == -1 || access(src_emu_client, F_OK) == -1 ){
                                                printf("Emulated Memory not found\n");
                                                exit(-1);
                                        } else {
                                                emu_server = open(src_emu_server,O_RDWR);
                                                if (emu_server < 0){
                                                        printf("Error in opening file\n");
                                                        exit(-1);
                                                }
                                                write(emu_server,w,255*sizeof(char));
                                                emu_client = open(src_emu_client,O_RDWR);
                                                if (emu_client < 0){
                                                        printf("Error in opening file\n");
                                                        exit(-1);
                                                }
                                                char *bufff = malloc(28*sizeof(char));
                                                read(emu_client, bufff, 28*sizeof(char));
                                                write(fifo_client, bufff, 28*sizeof(char));
                                        	free(bufff);
					}
				}
			}	
		} else {
			printf("Error, invalid command\n");
		}
		i -= 1;
                while (i >= 0){
                	free(args[i]);
                        i -= 1;
                }
		close(fifo_server);
        	close(fifo_client);
        	close(emu_server);
        	close(emu_client);
		free(buf);
	}	
	kill:
		pthread_exit(NULL);		
}

int main(int argc, char** argv){
	if (argc != 1){
		printf("Invalid number of arguments (expected 1)\n");
		return -1;
	} else {
		int status = 1;
		char str[255];
		int fifo_server = -1;
		int fifo_client = -1;
		char *src_emu_server = "emu_server";
		char *src_emu_client = "emu_client";
		char *tid;
		char *buf;
		int emu_server = mkfifo(src_emu_server,S_IWUSR | S_IRUSR | S_IRGRP | S_IROTH);
		if (emu_server < 0) {
			printf("Unable to create fifo to emulated mem, may already exists\n");
		}
		int emu_client = mkfifo(src_emu_client,S_IWUSR | S_IRUSR | S_IRGRP | S_IROTH);
		if (emu_client < 0) {
			printf("Unable to create fifo to emulated mem, may already exists\n");
		}
		do {
			char *src_server = "fifo_server_";
                	char *src_client = "fifo_client_";
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
					if (processes[i] == -1) {
						pthread_create(&processes[i],NULL,thread_func, (void*)i);	
						char *server = "fifo_server_";
						char *s;
						sprintf(s,"%lu",processes[i]);
						strcat(server, s);
						int thread_process = mkfifo(server, S_IWUSR | S_IRUSR | S_IRGRP | S_IROTH);
						if (thread_process < 0){
							printf("Unable to create a fifo, may have been created already\n");
						}
						char *client = "fifo_client_";
						char *c;
						sprintf(c,"%d",i);
						strcat(client, s);
						int main_process = mkfifo(client,S_IWUSR | S_IRUSR | S_IRGRP | S_IROTH);
						if (main_process < 0){
							printf("Unable to create a fifo, may have been created already\n");
						}
						break;
					}
				}
			} else if ( strcmp(str,"list") == 0){
				printf("\nCurrently Running Processes: \n");
				int j;
				for (j = 0; j < 4; j++){
					if (processes[j] > 0){
						printf("Process ID: %lu\n",processes[j]);
					}
				}
			} else if ( strcmp(str,"exit") == 0){
				emu_server = open(src_emu_server,O_RDWR);
				char * e = "exit";
                                write(emu_server,e,4*sizeof(char));
				status = 0;
			} else {
				char *bufff;
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
							fifo_client = open(src_client,O_RDWR);
							if (fifo_client < 0){
								printf("Error in opening file\n");
								exit(-1);
							}
							bufff = malloc(10*sizeof(char));
							read(fifo_client, bufff, 10*sizeof(char));
							if (strcmp(buf, "kill_succ_") == 0){
								free(bufff);
								unsigned long int rid = strtoul(tid, NULL, 10);
								int t = 0;
								for (t; t < 4;t++){
									if (((unsigned long int)processes[t]) == rid){
										processes[t] = -1;
									}	
								}
							} else {
								printf("Error in killing thread\n");
								free(bufff);
								exit(-1);
							}
						}
					}	
				} else if ( strcmp(args[0],"mem") == 0){
					if (i != 2){
                                                printf("Invalid number of arguments: Expected 2\n");
                                        } else {
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
							fifo_client = open(src_client,O_RDWR);
                                                        if (fifo_client < 0){
                                                                printf("Error in opening file\n");
                                                                exit(-1);
                                                        }
							bufff = malloc(10*sizeof(char));
                                                        read(fifo_client, bufff, 10*sizeof(char));
                                                        if (strcmp(bufff, "memo_succ_") == 0){
                                                        } else {
                                                                printf("Error in printing mem\n");
                                                        }
							free(bufff);
                                                }
                                        }
				} else if ( strcmp(args[0],"allocate") == 0){
					if (i != 2){
                                                printf("Invalid number of arguments: Expected 2\n");
					} else {
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
							fifo_client = open(src_client,O_RDWR);
                                                        if (fifo_client < 0){
                                                                printf("Error in opening file\n");
                                                                exit(-1);
                                                        }
							bufff = malloc(10*sizeof(char));
                                                        read(fifo_client, bufff, 10*sizeof(char));
                                                        if (strcmp(buf, "allo_succ_") == 0){
								printf("Allocated\n");
								free(bufff);
                                                        } else {
                                                                printf("Error in allocating\n");
                                                                free(bufff);
                                                                exit(-1);
                                                        }
                                                }
                                        }
				} else if ( strcmp(args[0],"read") == 0){
					if (i != 3){
                                                printf("Invalid number of arguments: Expected 3\n");
                                        } else {
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
							fifo_client = open(src_client,O_RDWR);
                                                        if (fifo_client < 0){
                                                                printf("Error in opening file\n");
                                                                exit(-1);
                                                        }
                                                        bufff = malloc(11*sizeof(char));
                                                        read(fifo_client, bufff, 11*sizeof(char));
                                                        printf("Value: %s\n",bufff);
                                                        free(bufff);
                                                }
                                        }
				} else if ( strcmp(args[0],"write") == 0){
					if (i != 4){
                                                printf("Invalid number of arguments: Expected 4\n");
                                        } else {
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
							fifo_client = open(src_client,O_RDWR);
                                                        if (fifo_client < 0){
                                                                printf("Error in opening file\n");
                                                                exit(-1);
                                                        }
                                                        bufff = malloc(10*sizeof(char));
                                                        read(fifo_client, bufff, 10*sizeof(char));
                                                        if (strcmp(bufff, "writ_succ_") == 0){
								free(bufff);
                                                        } else {
                                                                printf("Error in writing\n");
                                                                free(bufff);
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
	return 0;
}
