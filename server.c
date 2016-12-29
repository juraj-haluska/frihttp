/**
 * Implementation of web server with threads.
 * This server supports GET method and execution of BASH
 * scripts on POSIX compatible machines.
 *
 * © Copyright 20.12.2015 Juraj Haluška
 *
 * @file
 */

#include "server.h"
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/sendfile.h>

/**
 * Function fills buffer with users request.
 *
 * @param[in] cliFd 	Clients file descriptor.
 * @param[in,out] buff	Buffer for request.
 * @param[in] buffSize	Size of buffer.
 */
void getRequest(int cliFd, char * buff,int buffSize){
	int c = read(cliFd,buff,buffSize);
	if (c < 0){
		fprintf(stderr,"Error while reading socket\n");
	}
}

/**
 * Function parses request in buffer and
 * writes values to request structure.
 *
 * @param[in] buff	Buffer with request.
 * @param[out] request	Structure of request.
 */
int parseRequest(char * buff, struct request_t * request){
	char line[LINE_LENGTH];
	bzero(line,LINE_LENGTH);

	size_t len = strcspn(buff, CRLF);

	if(len != 0 && len <= LINE_LENGTH){
		strncpy(line,buff,len);
	} else return 0;

	if(strncmp(line,"GET",3) == 0){
		request->method = GET;
	} else {
		request->method = UNKNOWN;
		return 0;
	}

	char * urip = strstr(line,SPC);
	if (urip != NULL){
		//skip space
		urip += strlen(SPC);
		char * verp = strstr(urip,SPC);
		if (verp != NULL){
			strncpy(request->uri,urip,verp-urip);
			verp += strlen(SPC);
			strcpy(request->version,verp);
			return 1;
		}
	}
	return 0;
}

/**
 * This function parses uri in structure
 * request and saves data to it.
 *
 * @param[in,out] request Structure request.
 */
int parseUri(struct request_t * request){
	char path[URI_LENGTH];
	bzero(path,URI_LENGTH);

	size_t len = strcspn(request->uri, DELIM_URI); //returns number of characters where is not DELIM_URI

	if (len != 0){
		strncpy(path,request->uri,len);

		char * chptr = request->uri + len;
		if (*chptr!='\0'){
			chptr++; //skip space
			request->optc = 0;
			int znak = 0;
			while(*chptr!='\0'){
				if(*chptr == DELIM_ARGS){
					if(request->optc >= OPT_COUNT - 1) return 0;
					else {
						request->optc++;
						chptr++;
						znak = 0;
					}
				} else {
					if (znak >= OPT_LENGTH - 1) return 0;
					else {
						request->opts[request->optc][znak++]=*chptr;
						request->optv[request->optc+1]=request->opts[request->optc];
						chptr++;
					}
				}
			}
			request->optc++;
		}
		bzero(request->uri,URI_LENGTH);
		strcpy(request->uri,path);
	}
	return 1;
}

/**
 * Function checks if file exists
 * and if exists, it will return its size.
 *
 * @param[in] filename	File name.
 * @retval Size of file, if does not exists 0.
 */
int fileExist(char *filename) {
    struct stat st;
    stat(filename, &st);
    if (S_ISREG(st.st_mode)) return st.st_size;
    else return 0;
}

/**
 * Function creates and checks path to file.
 *
 * @param[in,out] request	Structure of request.
 * @param[in] data		Structure Data, which contains path to rootdir.
 * @retval 			If success will return 1.
 */
int makePath(struct request_t * request,struct thData * Data){
	char temp[URI_LENGTH];
	bzero(temp,URI_LENGTH);

	strcpy(temp,Data->rootDir);

	if(strstr(request->uri,"../") == NULL){
		if(strlen(request->uri) == 1 && *request->uri == '/'){
			strcat(temp,"index.html");
		} else if(request->uri[strlen(request->uri) - 1] =='/'){
			char * urip = request->uri + 1;
			strcat(temp,urip);
			strcat(temp,"index.html");
		} else {
			char * urip = request->uri + 1;
			strcat(temp,urip);
		}
		if((request->filesize = fileExist(temp)) > 0){
			bzero(request->uri,URI_LENGTH);
			strcpy(request->uri,temp);

			char * bodka = strrchr(temp, '.');
			if(bodka != NULL){
				if(strcmp(bodka,".html") == 0) request->type = HTML;
				else if(strcmp(bodka,".jpg") == 0) request->type = JPG;
				else if(strcmp(bodka,".jpeg") == 0) request->type = JPEG;
				else if(strcmp(bodka,".bmp") == 0) request->type = BMP;
				else if(strcmp(bodka,".gif") == 0) request->type = GIF;
				else if(strcmp(bodka,".png") == 0) request->type = PNG;
				else if(strcmp(bodka,".bash") == 0) request->type = BASH;
				else request->type = UNK;
			} else {
				request->type = UNK;
			}
			return 1;
		}
		else return 0;
	} else {
		return 0;
	}
}

/**
 * Function writes line on filedescriptor.
 * If outout string is NULL function
 * returns CRLF on filedescriptor.
 *
 * @param[in] line	Pointer on string.
 * @param[in] cliFd	Clients filedescriptor.
 * @retval		If successful 1 else 0.
 */
int writeln(char * line,int cliFd){
	char lbuf[LINE_LENGTH];
	bzero(lbuf,LINE_LENGTH);
	if(line != NULL){
		strcpy(lbuf,line);
		strcat(lbuf,CRLF);
		if(write(cliFd,lbuf,strlen(lbuf)) < 0){
			perror("Writing on socket was not successful.\n");
			return 0;
		} else return 1;
	}else {
		if(write(cliFd,CRLF,strlen(CRLF)) > 0) return 1;
		else return 0;
	}
}

/**
 * Function sends message no. 404.
 *
 * @param[in] cliFd Clients filedescriptor.
 * @param[in] uri	Pointer on string with filename.
 */
void error404(int cliFd,char * uri){
	/*writeln(S_404,cliFd);
	writeln(SERVER,cliFd);
	writeln(CLOSED,cliFd);
	writeln(NULL,cliFd);//CRLF
	writeln(S_404TXTA,cliFd);
	writeln(uri,cliFd);
	writeln(S_404TXTB,cliFd);*/
	char buff[1024];
	bzero(buff,1024);
	sprintf(buff,"%s\r\n%s\r\n%s\r\n\r\n%s%s%s",S_404,SERVER,CLOSED,S_404TXTA,uri,S_404TXTB);
	write(cliFd,buff,strlen(buff));
}

/**
 * Returns messsage no. 400.
 *
 * @param[in] cliFd Clients filedescriptor.
 */
void error400(int cliFd){
	/*writeln(S_400,cliFd);
	writeln(SERVER,cliFd);
	writeln(CLOSED,cliFd);
	writeln(NULL,cliFd);//CRLF
	writeln(S_400TXTA,cliFd);
	writeln(S_400TXTB,cliFd);*/
	char buff[1024];
	bzero(buff,1024);
	sprintf(buff,"%s\r\n%s\r\n%s\r\n\r\n%s%s",S_400,SERVER,CLOSED,S_400TXTA,S_400TXTB);
	write(cliFd,buff,strlen(buff));
}

/**
 * Send messsage no. 200.
 *
 * @param[in] cliFd Clients filedescriptor.
 */
void ok200(int cliFd){
	/*writeln(S_200,cliFd);
	writeln(SERVER,cliFd);
	writeln(CLOSED,cliFd);
	writeln(NULL,cliFd);//CRLF*/
	char buff[512];
	bzero(buff,512);
	sprintf(buff,"%s\r\n%s\r\n%s\r\n\r\n",S_200,SERVER,CLOSED);
	write(cliFd,buff,strlen(buff));
}

/**
 * Function executes bash script with
 * parameters from request struct.
 *
 * @param[in] request	Structure request.
 * @param[in] Data	Data structure of thread containing clients filedescriptor.
 */
void obsluzBash(struct request_t * request,struct thData * Data){
	int bashPipe[2];
	if(pipe(bashPipe) < 0 ){
		fprintf(stderr,"Coudlnt create pipe\n");
	}

	pid_t pid = fork();
	if (pid < 0){
		fprintf(stderr,"Error while calling fork\n");
	} else if(pid > 0)
	{
		close(bashPipe[1]);
			char buf;
			while (read(bashPipe[0], &buf, 1) > 0)
				write(Data->cliFd, &buf, 1);
		close(bashPipe[0]);
		wait(NULL);
	} else
	{
		close(bashPipe[0]);
		dup2(bashPipe[1],1);
		request->optv[0] = request->uri;
		execv(request->uri,request->optv);
		fprintf(stderr, "Error while executing script\n");
	}
}

/**
 * Function sends file on clients filedescriptor
 * Uses unix function sendfile.
 *
 * @param[in] request	Structure request.
 * @param[in] cliFd	Clients filedescriptor.
 */
void sendFile(struct request_t * request, int cliFd){
    int file;
    if((file=open(request->uri,O_RDONLY)) > 0){
		int c = sendfile(cliFd, file, 0, request->filesize);
		if (c == -1) {
			fprintf(stderr, "Error while sending file: %s\n", strerror(errno));
		}
		if (c != request->filesize) {
			fprintf(stderr, "Sending of file was not successful, bytes sended: %d from %d \n", c, (int)request->filesize);
		}
	}
	close(file);
}


/**
 * Main function of thread.
 *
 * @param[in] params	Dynamically allocated structure thData.
 */
void * Obsluz(void * params){
	struct thData * Data = (struct thData *)params;

	struct request_t request;
	bzero((char *)&request,sizeof(request));

	char buffer[REQUEST_BUFFER];
	bzero(buffer,REQUEST_BUFFER);

	getRequest(Data->cliFd,buffer,REQUEST_BUFFER);

	if(parseRequest(buffer,&request))
	{
		if(!parseUri(&request)){
			error400(Data->cliFd);
		} else {
			int stat = makePath(&request,Data);
			if((stat > 0 ) && request.type == BASH){
				ok200(Data->cliFd);
				obsluzBash(&request,Data);
			} else if(stat > 0){
				ok200(Data->cliFd);
				sendFile(&request,Data->cliFd);
			} else {
				error404(Data->cliFd,request.uri);
			}
		}
	} else error400(Data->cliFd);

	if(close(Data->cliFd) == 0){
		free(Data);
		return NULL;
	} else {
		fprintf(stderr,"Error while closing socket %d.\n",Data->cliFd);
		free(Data);
		return NULL;
	}
}

int main(int argc,char *argv[]){

	int c;
	opterr = 0; //disabling error output from getopt on stderr

	char * ipAddr = DEF_IP;
	char * ipPort = DEF_PORT;
	char * rootDir = DEF_ROOT;

	while((c = getopt(argc,argv,"a:p:r:")) != -1){
		switch(c){
			case 'a':
				ipAddr = optarg;
			break;

			case 'p':
				ipPort = optarg;
			break;

			case 'r':
				rootDir = optarg;
			break;

			case '?':
				if (optopt == 'a')
					fprintf (stderr, "After parameter -%c give IP address.\n", optopt);
				else if (optopt == 'p')
					fprintf (stderr, "After parameter -%c give port.\n", optopt);
				else if (optopt == 'r')
					fprintf (stderr, "After parameter -%c give path to the directory\n", optopt);
				else
					fprintf (stderr, "Unknown parameter `-%c'.\n", optopt);
			return 1;

			default:
				return 1;
		}
	}

	int index;
	for (index = optind; index < argc; index++){
		printf ("Not existing parameter %s\n", argv[index]);
	}

	printf("Address of server: %s \n",ipAddr);
	printf("Port of server: %s \n",ipPort);
	printf("Root directory: %s \n",rootDir);

	int servFd, cliFd;
	struct sockaddr_in servAddr;

	bzero((char *)&servAddr,sizeof(servAddr));
	servAddr.sin_family = AF_INET;
	servAddr.sin_port = htons(atoi(ipPort));
	inet_aton(ipAddr, &servAddr.sin_addr);

	servFd = socket(AF_INET,SOCK_STREAM,0);
	if(servFd < 0){
		fprintf(stderr,"Error while creating socket\n");
	}

	if(bind(servFd,(struct sockaddr*)&servAddr,sizeof(servAddr)) < 0){
		fprintf(stderr,"Error while binding address.\n");
	}

	if(listen(servFd, MAX_CLIENTS) < 0){
		fprintf(stderr,"Error while initialization of listening.\n");
	}

	pthread_attr_t thAttr;
	pthread_attr_init(&thAttr);
	pthread_attr_setdetachstate(&thAttr, PTHREAD_CREATE_DETACHED);

	while(1){
		cliFd = accept(servFd, NULL, NULL);
		if(cliFd < 0){
			fprintf(stderr,"Coudlnt connect client.\n");
		}else {

			pthread_t thClient;

			struct thData * ptrData = (struct thData *)malloc(sizeof(struct thData));
			ptrData->cliFd = cliFd;
			ptrData->rootDir = rootDir;

			if(pthread_create(&thClient, &thAttr,(void *) &Obsluz, ptrData) != 0){
				fprintf(stderr,"Error while closing thread.\n");
			}


		}
	}

	pthread_attr_destroy(&thAttr);
	close(servFd);

	return 0;
}
