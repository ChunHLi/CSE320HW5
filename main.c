#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <fcntl.h>
#include <inttypes.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

pthread_t processes[4] = {-1,-1,-1,-1};

unsigned long cse320_malloc(unsigned long pt_1[64], unsigned long pt_2[64],char* myid){
	char* src_server = "emu_server";
	char* src_client = "emu_client";
	char allo[10] = "allo,";
	strcat(allo,myid);
	int emu_server = open(src_server,O_WRONLY);
        if (emu_server < 0){
		printf("Error in opening file\n");
		exit(-1);
	}
	write(emu_server,allo,10*sizeof(char));
	close(emu_server);
        int emu_client = open(src_client,O_RDONLY);
	if (emu_client < 0){
		printf("Error in opening file\n");
		exit(-1);
        }
        char* buf = calloc(28,sizeof(char));
        read(emu_client, buf, 4*sizeof(char));
	close(emu_client);
	printf("Physical Addr: %s\n", buf);
	if (strcmp(buf,"error,address out of range")==0 || strcmp(buf,"error,address is not aligned")==0){
		printf("%s\n",buf);
		return -1;
	}
	//int physAddr = atoi(buf);
	int i = 0;
	for (i; i < 64 ;i++){
		if (pt_2[i] < 512){
			break;
		}
	}
	pt_2[i] = (unsigned long)(512 + i);
	int j = 0;
	for (j; j < 64; j++){
		if (pt_1[j] < 512){
			break;
		}
	}
	pt_1[j] = (unsigned long)(512 + j);
	unsigned long pt1 = pt_1[j];
	pt1 = pt1 << 22;
	unsigned long pt2 = pt_2[i];
	pt2 = pt2 << 12;
	unsigned long va = pt1 | pt2;
        free(buf);
	return va;                        
}

unsigned long cse320_virt_to_phys(unsigned long va, int myid){
	if (va > 0xFFFFF000) {
		printf("Invalid virtual address; address out of range");
	} else {
		unsigned long pa = ((0x003FF000 & va) >> 12);
		if (pa < 512){
			printf("Address not allocated\n");
		} else {
			pa -= 512;
			pa = pa + 256*myid;
			return pa;
		}
	}
	//printf("Physical Address: %lu\n",pa);
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
	char sid[1];
	sprintf(sid, "%d",myid);
	char id[32];
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
	char* src_emu_server = "emu_server";
	char* src_emu_client = "emu_client";
	char src_server[80] = "fifo_server_";
	strcat(src_server, id);
	char src_client[80] = "fifo_client_";
	strcat(src_client, id);	
	while (1) {
		char* writ;
		fifo_server = open(src_server,O_RDONLY);
		if (fifo_server<0) {
			printf("Error opening file\n");
		}
                char* buf = (char*)calloc(80,sizeof(char));
		read(fifo_server,buf,80*sizeof(char));
		close(fifo_server);
		if (strcmp(buf,"kill")==0){
			printf("Process Index: %s\n",sid);
			writ = "kill_succ_";
			printf("Process Killed");
			char k[6] = "kill,";
			strcat(k,sid);
                        emu_server = open(src_emu_server,O_WRONLY);
                        if (emu_server < 0){
                                printf("Error in opening file\n");
                                exit(-1);
                        }
                        write(emu_server,k,6*sizeof(char));
			close(emu_server);
			fifo_client = open(src_client, O_WRONLY);
                	if (fifo_client<0) {
                        	printf("Error opening file\n");
                	}
			write(fifo_client,writ,10*sizeof(char));
        		close(fifo_client);
			goto kill;
                } else if (strcmp(buf,"memo")==0){
			writ = "memo_succ_";
			printf("Addresses within the process %s:\n", id);
			int m;
			for (m=0; m<virt_len ;m++){
				printf("%lu\n",virt_addr[m]);
			}
			fifo_client = open(src_client, O_WRONLY);
                        if (fifo_client<0) {
                                printf("Error opening file\n");
                        }
			write(fifo_client,writ,10*sizeof(char));
			close(fifo_client);
		} else if (strcmp(buf,"allo")==0){
			unsigned long va = cse320_malloc(pt_1,pt_2,sid);
			if (va == 0xFFFFFFFF) {
				printf("allo_fail_: %lu\n", va);
				writ = "allo_fail_";
				fifo_client = open(src_client, O_WRONLY);
                        	if (fifo_client<0) {
                                	printf("Error opening file\n");
                        	}
				write(fifo_client,writ,10*sizeof(char));
				close(fifo_client);		
			} else {
				printf("allo_succ_: %lu\n", va);
				writ = "allo_succ_";
				virt_addr[virt_len] = va;
				virt_len += 1;
				fifo_client = open(src_client, O_WRONLY);
                        	if (fifo_client<0) {
                                	printf("Error opening file\n");
                        	}
				write(fifo_client,writ,10*sizeof(char));
				close(fifo_client);
			}
		} else {
			printf("Most recent addition: %s\n",buf);
			int i = 0;
			char** args = (char**)calloc(4,sizeof(char*));
                        int a;
                        for (a=0;a<4;a++){
                        	args[a] = (char *)calloc(32,sizeof(char));
                        }
                        char* token = strtok(buf," ");
                        while (token != NULL){
                                args[i++] = strdup(token);
                        	token = strtok(NULL, " ");
                        }
			printf("args[0]: %s\n",args[0]);
			printf("args[1]: %s\n",args[1]);
			if (strcmp(args[0],"read")==0){
				printf("what?\n");
				unsigned long va = strtoul(args[1], NULL, 10);
				unsigned long pa = cse320_virt_to_phys(va,myid);
				if (pa > 0xFFFFF000){
				} else {
					if (pa < myid * 256 || pa >= (myid + 1)* 256){
						printf("error,address out of range");
						writ = "NULL";
						fifo_client = open(src_client, O_WRONLY);
                        			if (fifo_client<0) {
                                			printf("Error opening file\n");
                        			}
						write(fifo_client,writ,4*sizeof(char));
						close(fifo_client);
					} else {
						char r[10] = "read,";
						char physADDR[4];
						sprintf(physADDR,"%lu",pa);
						strcat(r, physADDR);
						printf("Sending to mem: %s\n", r);
                                		emu_server = open(src_emu_server,O_WRONLY);
                                		if (emu_server < 0){
                                        		printf("Error in opening file\n");
                                        		exit(-1);
                                		}
                                		write(emu_server,r,10*sizeof(char));
						close(emu_server);
						printf("Wrote to mem\n");
                                		char* bufff = (char*)calloc(28,sizeof(char));
						emu_client = open(src_emu_client,O_RDONLY);
                                                if (emu_client < 0){
                                                        printf("Error in opening file\n");
                                                        exit(-1);
                                                }
                                		read(emu_client, bufff, 28*sizeof(char));
						close(emu_client);
						fifo_client = open(src_client, O_WRONLY);
                        			if (fifo_client<0) {
                                			printf("Error opening file\n");
                        			}
						write(fifo_client, bufff, 28*sizeof(char));
						close(fifo_client);
                        			free(bufff);
					}
				}
			} else if (strcmp(args[0],"write")==0){
                        	unsigned long va = strtoul(args[1], NULL, 10);
                        	unsigned long pa = cse320_virt_to_phys(va,myid);
                        	if (pa > 0xFFFFF000 != pa == 0xFFFFFFFFF){
                        	} else {
                                	if (pa < myid * 256 || pa >= (myid + 1) * 256){
                                        	printf("error,address out of range");
                                        	writ = "writ_fail_";
                                        	write(fifo_client,writ,10*sizeof(char));
						close(fifo_client);
                                	} else {
						char w[32] = "write,";
						char* del = ",";
						char physADDR[4];
						sprintf(physADDR,"%lu",pa);
                                        	strcat(w, physADDR);
						strcat(w, del);
						strcat(w, args[2]);
						printf("Sending: %s\n",w);
                                                emu_server = open(src_emu_server,O_WRONLY);
                                                if (emu_server < 0){
                                                       	printf("Error in opening file\n");
                                                       	exit(-1);
                                                }
                                                write(emu_server,w,32*sizeof(char));
						close(emu_server);
						char* bufff =  (char*)calloc(28,sizeof(char));
                                                emu_client = open(src_emu_client,O_RDONLY);
                                                if (emu_client < 0){
                                                        printf("Error in opening file\n");
                                                        exit(-1);
                                                }
                                                read(emu_client, bufff, 28*sizeof(char));
						close(emu_client);
						fifo_client = open(src_client, O_WRONLY);
						if (fifo_client < 0){
							printf("Error in opening file\n");
							exit(-1);
						}
                                                write(fifo_client, bufff, 28*sizeof(char));
						close(fifo_client);
						free(bufff);		
					}
				}
			} else {
                        	printf("Error, invalid command\n");
                	}
			i = 0;
                        while (i < 4){
                        	free(args[i]);
                                i += 1;
                        }
			free(args);
			free(buf);
		}
	}	
	kill:
		pthread_exit(NULL);		
}

int main(int argc, char** argv){
	if (argc != 1){
		printf("Invalid number of arguments (expected 1)\n");
		return -1;
	} else {
		int i = 0;
		int status = 1;
		char str[64];
		int fifo_server = -1;
		int fifo_client = -1;
		char* src_emu_server = "emu_server";
		char* src_emu_client = "emu_client";
		char* buf;
		int emu_server = mkfifo(src_emu_server,S_IWUSR | S_IRUSR | S_IRGRP | S_IROTH);
		if (emu_server < 0) {
			printf("Unable to create fifo to emulated mem, may already exists\n");
		}
		int emu_client = mkfifo(src_emu_client,S_IWUSR | S_IRUSR | S_IRGRP | S_IROTH);
		if (emu_client < 0) {
			printf("Unable to create fifo to emulated mem, may already exists\n");
		}
		do {
			char src_server[80] = "fifo_server_";
                	char src_client[80] = "fifo_client_";
			printf("> ");
			char** args;
			fgets(str,64,stdin);
			char* p = strchr(str,'\n');
                        if (p != NULL){
                                str[p-str]='\0';
			}
			if ( strcmp(str,"") == 0 ){
			} else if ( strcmp(str,"create") == 0){
				int d;
				for (d = 0; d < 4; d++){
					if (processes[d] == -1) {
						pthread_create(&processes[d],NULL,thread_func, NULL);	
						char s[80];
						sprintf(s,"%lu",processes[d]);
						strcat(src_server, s);
						int thread_process = mkfifo(src_server, S_IWUSR | S_IRUSR | S_IRGRP | S_IROTH);
						if (thread_process < 0){
							printf("Unable to create a fifo, may have been created already\n");
						}
						char c[80];
						sprintf(c,"%lu",processes[d]);
						strcat(src_client, c);
						int main_process = mkfifo(src_client,S_IWUSR | S_IRUSR | S_IRGRP | S_IROTH);
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
					if (processes[j] != -1){
						printf("Process ID: %lu\n",processes[j]);
					}
				}
			} else if ( strcmp(str,"exit") == 0){
				char* e = "exit";
				emu_server = open(src_emu_server,O_WRONLY);
                                write(emu_server,e,4*sizeof(char));
				close(emu_server);
				status = 0;
			} else {
				char** args = (char**)calloc(5,sizeof(char*));
				int a;
				for (a=0;a<5;a++){
					args[a] = (char*)calloc(32,sizeof(char));
				}
				char* token = strtok(str," ");
                                i = 0;
				while (token != NULL){
					args[i++] = strdup(token);
					token = strtok(NULL, " ");
				}
				if ( strcmp(args[0],"kill") == 0){
					if (i != 2){
						printf("Invalid number of arguments: Expected 2\n");
					} else {
						char* tid = args[1];
						strcat(src_server,tid);
						strcat(src_client,tid);
						fifo_server = open(src_server,O_WRONLY);
						if (fifo_server < 0){
							printf("Error in opening file\n");
							exit(-1);
						}
						write(fifo_server, "kill",4*sizeof(char));
						close(fifo_server);
						char* bufff = malloc(10*sizeof(char));
						fifo_client = open(src_client,O_RDONLY);
						if (fifo_client < 0){
							printf("Error in opening file\n");
							exit(-1);
						}
						read(fifo_client, bufff, 10*sizeof(char));
						close(fifo_client);
						if (strcmp(bufff, "kill_succ_") == 0){
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
				} else if ( strcmp(args[0],"mem") == 0){
					printf("Debug Message\n");
					if (i != 2){
                                                printf("Invalid number of arguments: Expected 2\n");
                                        } else {
                                                char* tid = args[1];
                                                strcat(src_server,tid);
                                                strcat(src_client,tid);
                                                fifo_server = open(src_server,O_WRONLY);
                                                if (fifo_server < 0){
                                                        printf("Error in opening file\n");
                                                        exit(-1);
                                                }
                                                write(fifo_server, "memo",4*sizeof(char));
						close(fifo_server);
						char* bufff = malloc(10*sizeof(char));
						fifo_client = open(src_client,O_RDONLY);
                                                if (fifo_client < 0){
                                                        printf("Error in opening file\n");
                                                        exit(-1);
                                                }
						read(fifo_client, bufff, 10*sizeof(char));
						close(fifo_client);
                                                if (strcmp(bufff, "memo_succ_") == 0){
							printf("Success\n");
                                                } else {
                                                        printf("Error in printing mem\n");
                                                }
						free(bufff);
                                        }
				} else if ( strcmp(args[0],"allocate") == 0){
					if (i != 2){
                                                printf("Invalid number of arguments: Expected 2\n");
					} else {
                                                char* tid = args[1];
						printf("tid : %s\n", tid);
                                                strcat(src_server,tid);
                                                strcat(src_client,tid);
						printf("server filename: %s\n", src_server);
						printf("client filename: %s\n", src_client);
                                                fifo_server = open(src_server,O_WRONLY);
                                                if (fifo_server < 0){
                                                        printf("Error in opening file\n");
                                                        exit(-1);
                                                }
                                                write(fifo_server, "allo",4*sizeof(char));
						close(fifo_server);
						fifo_client = open(src_client,O_RDONLY);
                                                if (fifo_client < 0){
                                                	printf("Error in opening file\n");
                                                        exit(-1);
                                                }
						char* bufff = malloc(10*sizeof(char));
                                                read(fifo_client, bufff, 10*sizeof(char));
						close(fifo_client);
                                                if (strcmp(bufff, "allo_succ_") == 0){
							printf("Allocated\n");
							free(bufff);
                                                } else {
                                                	printf("Error in allocating\n");
                                                        free(bufff);
                                                        exit(-1);
                                                }
                                        }
				} else if ( strcmp(args[0],"read") == 0){
					if (i != 3){
                                                printf("Invalid number of arguments: Expected 3\n");
                                        } else {
                                                char* tid = args[1];
                                                strcat(src_server,tid);
                                                strcat(src_client,tid);
                                                fifo_server = open(src_server,O_WRONLY);
                                                if (fifo_server < 0){
                                                        printf("Error in opening file\n");
                                                        exit(-1);
                                                }	
						char cmdr[64] = "read ";
						char* virt_addrr = args[2];
						strcat(cmdr, virt_addrr);
						printf("What am I reading in?: %s\n",cmdr);
                                                write(fifo_server, cmdr,strlen(cmdr)*sizeof(char));
						close(fifo_server);
						printf("Did it get up to here? YES\n");
						fifo_client = open(src_client,O_RDONLY);
                                                if (fifo_client < 0){
                                                	printf("Error in opening file\n");
                                                        exit(-1);
                                                }
                                                char* bufff = malloc(11*sizeof(char));
                                                read(fifo_client, bufff, 11*sizeof(char));
						close(fifo_client);
                                                printf("Value: %s\n",bufff);
                                                free(bufff);
                                        }
				} else if ( strcmp(args[0],"write") == 0){
					if (i != 4){
                                                printf("Invalid number of arguments: Expected 4\n");
                                        } else {
                                                char* tid = args[1];
                                                strcat(src_server,tid);
                                                strcat(src_client,tid);
						char cmdw[64] = "write ";
                                                char* virt_addrw = args[2];
                                                char* value = args[3];
                                                char* tmp = " ";
                                                strcat(cmdw, virt_addrw);
                                                strcat(cmdw, tmp);
                                                strcat(cmdw, value);
                                                fifo_server = open(src_server,O_WRONLY);
                                                if (fifo_server < 0){
                                                        printf("Error in opening file\n");
                                                        exit(-1);
                                               	}
                                               	write(fifo_server, cmdw,strlen(cmdw)*sizeof(char));
						close(fifo_server);
						char* bufff = malloc(10*sizeof(char));
						fifo_client = open(src_client,O_RDONLY);
                                                if (fifo_client < 0){
                                                	printf("Error in opening file\n");
                                                        exit(-1);
                                                }
                                                read(fifo_client, bufff, 10*sizeof(char));
						close(fifo_client);
						printf("%s\n",bufff);
                                                if (strcmp(bufff, "writ_succ_") == 0){
							free(bufff);
                                                } else {
                                                        printf("Error in writing\n");
                                                        free(bufff);
                                                       	exit(-1);
                                                }
                                        }
				} else {
					printf("Invalid command: %s not recognized\n", args[0]);
				}
				i = 0;
                                while (i < 4){
                                        free(args[i]);
                                        i += 1;
                                }
				free(args); 
			}
			
		} while (status);
	}
	return 0;
}
