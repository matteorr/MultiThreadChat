/*
 * chatserverTCP.c
 *
 *  Created on: Feb 2013
 *      Author: Matteo Ruggero Ronchi
 *
 *  Copyright 2013 Matteo Ruggero Ronchi
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <signal.h>

#include <netdb.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <pthread.h>

#include <unistd.h>

#define MAXTHREADS 3

#define KILO 1024
#define BUFF_LENGTH 1000
#define PROTO_PORT 60000
#define QLEN 1

#define MAX_CONTACTS 3


int contacts = 0;
pthread_t tid[MAXTHREADS];	
int active_socket[MAXTHREADS];
int thread_retval = 0;
int sd;
int endloop;

typedef struct contact{
	char contactname[256];
	int contactsd;
}contact;

contact onlinecontacts[MAX_CONTACTS];

void chat(int sd2){
	int n,i, served = 0;	
	printf("CHAAAAAAAAAAAAT\n");
	
	char clientname[BUFF_LENGTH];
	char *var;
	char message[BUFF_LENGTH];
	char inbuf[BUFF_LENGTH];
	char outbuf[BUFF_LENGTH];
	
	while(served == 0){

		for(i = 0; i < BUFF_LENGTH; i++){
			inbuf[i] = 0;
			outbuf[i] = 0;
		}

		n = read(sd2, inbuf, sizeof(inbuf));
		
		if(!strcmp(inbuf, "QUIT")){
			sprintf(outbuf, "QUIT");
			write(sd2, outbuf, sizeof(outbuf));			
			served = 1;
		}else{
			if(!strncmp(inbuf, "<", 1)){
				var = strstr(inbuf, "<");
				i = 0;		
				var++;	
				while(*var != '>'){
					clientname[i] = *var;
					printf("%c", *var);				
					var++;
					i++;
				}
				clientname[i] = '\0';
				
				var = strstr(inbuf, ">");
				i = 0;		
				var++;	
				while(*var != '\0'){
					message[i] = *var;
					printf("%c", *var);				
					var++;
					i++;
				}
				message[i] = '\0';

				printf("\nMessage [%s] is for [%s]\n\n", message, clientname);

				for(i = 0; onlinecontacts[i].contactsd != sd2; i++);
				sprintf(outbuf, "<%s> wrote: [%s]", onlinecontacts[i].contactname, message);

				//strcpy(outbuf, message);

				i = 0;
				while(strcmp(onlinecontacts[i].contactname, clientname)){
					i++;
				}

				write(onlinecontacts[i].contactsd, outbuf, sizeof(outbuf));

			}else{
				for(i = 0; onlinecontacts[i].contactsd != sd2; i++);
				sprintf(message, "<%s> wrote: [%s]", onlinecontacts[i].contactname, inbuf);
				strcpy(outbuf, message);	
				for(i = 0; i < contacts; i++){
					if(onlinecontacts[i].contactsd != sd2)
						write(onlinecontacts[i].contactsd, outbuf, sizeof(outbuf));
					
				}
			}
			
		}
	}
	printf("\nchat has finished\n");
	
}

void interrupt_handler(int sig){
	endloop = 1;
	close(sd);
	printf("Interrupt recieved: shutting down server!\n");
	return;
}
	
void *manage_connection(void *sdp) {

	char    inbuf[BUFF_LENGTH];       /* buffer for incoming data  */
	char	outbuf[BUFF_LENGTH];	/* buffer for outgoing data  */
	
	int i;			
	int sd2 = *((int *)sdp);
	int j = ((int *)sdp)-active_socket;	/* use pointer arithmetic to get this thread's index in array */
	//int thiscontact = contacts;
	for(i = 0; i < BUFF_LENGTH; i++){
		inbuf[i] = 0;
		outbuf[i] = 0;
	}

	//printf("\ndentro thread contacts [%d]\n\n", contacts);

	for(i = 0; i < contacts; i++){
	//	printf(" dentro for thread%s - %d\n\n", onlinecontacts[i].contactname, onlinecontacts[i].contactsd);

		sprintf(outbuf, "[%d]: [%s]\n", i, onlinecontacts[i].contactname);
		//printf("[%s]\n\n", buffer);
		write(sd2, outbuf, sizeof(outbuf));
	}
		
	sprintf(outbuf, "END");
	write(sd2, outbuf, sizeof(outbuf));

	printf("-(IN THREAD)- sent online contacts\n");

	printf("-(IN THREAD)- simulazione di chat\n");
	

	chat(sd2);

	tid[j] = (pthread_t)-1; //è un cast...	/* free thread array entry */
	
	close(sd2);
	printf("-(IN THREAD)- chiuso sd2\n");
	contacts--;
	//onlinecontacts[thiscontact].contactname = "removed";
	//onlinecontacts[thiscontact].contactsd = 0;
	return &thread_retval;

}

int main(int argc, char** argv){
	
	if(argc < 2){
		printf("\nUsage: ./servertcp 'IP' ['port'] \nUse 127.0.0.1 as IP if you want to test program on localhost, port number is optional!\n\n");
		return;
	}

	struct sockaddr_in sad;
	struct sockaddr_in cad;
	socklen_t alen;
	
	contact newcontact;	
	
	int sd2, port, n, i, j = 0;
	char* var;
	char clientname[256];
	char busymsg[] = "BUSY";
	char buffer[BUFF_LENGTH];

	for(i=0;i<MAXTHREADS;i++) {
		tid[i] = (pthread_t)-1;
		active_socket[i] = -1;
	}

	for(i = 0; i < BUFF_LENGTH; i++){
		buffer[i] = 0;
	}
	
	if(argc == 3){
		port = atoi(argv[2]);
		while(port < 0 || port > 64 * KILO){
			printf("Bad port number, buond limits are (0,%d)\n\nEnter a new port number: ", 64 * KILO);
			scanf("%d", &port);
		}
	}else{
		port = PROTO_PORT;
	}

	memset((char*)&sad, 0, sizeof(sad));
	sad.sin_family = AF_INET;
	n = inet_aton(argv[1], &sad.sin_addr);
	sad.sin_port = htons((u_short)port);

	printf("Server IP address and service port: [%s]:[%d]\n", argv[1], port);
	printf("Server IP address and Service Port [%d]:[%d]\n\n", sad.sin_addr.s_addr, sad.sin_port);

	sd = socket(PF_INET, SOCK_STREAM, 0);
	if(sd < 0) {
		perror("Socket creation failed\n");
		exit(1);
	}
	printf("Socket created with sd: [%d]\n\n", sd);

	n = bind(sd, (struct sockaddr *)&sad, sizeof(sad));
	if(n == -1){
		perror("Error in Bind\n");
		exit(1);
	}

	n = listen(sd, QLEN);
	if(n < 0){
		perror("Listen failed\n");
		exit(1);
	}
	
	signal(SIGINT, interrupt_handler);

	printf("Server in the service loop\n");

	while(!endloop){

		alen = sizeof(cad);
	
		printf("Server is waiting for a Client to serve...\n");
		
		sd2 = accept(sd, (struct sockaddr *)&cad, &alen);
		if (sd2 < 0) {
			if(endloop) break;
			perror("Accept failed\n");
			exit(1);
		}	
	
		if(contacts < MAXTHREADS) {
			
			printf("Connection with Client: [%s]:[%hu] - sd:[%d]\n", inet_ntoa(cad.sin_addr), ntohs(cad.sin_port), sd2);
			printf("This server is serving [%d] client%c\n", contacts + 1, (contacts > 0) ? 's' : ' ');

			n = read(sd2, buffer, sizeof(buffer));
			printf("Message from client: [%s]\n", buffer);
		
			var = strstr(buffer, "<");
			i = 0;		
			var++;	
			while(*var != '>'){
				clientname[i] = *var;
				printf("%c", *var);				
				var++;
				i++;
			}
			clientname[i] = '\0';
				
			printf("\nClient name is:  [%s]\n\n", clientname);
	
			strcpy(newcontact.contactname, clientname);
			newcontact.contactsd = sd2;
			onlinecontacts[contacts] = newcontact;

			//look for the first empty slot in thread array 
			for(i=0;tid[i]!=(pthread_t)-1;i++);
			
			 /*the use of different variables for storing socket ids
			 * avoids potential race conditions between the access
			 * to the value of &sd2 in the new started thread and
			 * the assignement by the connect() function call above.
			 * By Murphy's laws it my happen that the thread reads
			 * the variable pointed by its argument value after
			 * accept has stored a new value in sd2, thus loosing the
			 * previously opened socket.
			 */
			
		
			//printf(" prima if pthread %d\n\n", contacts);		
			active_socket[i] = sd2;

			if(pthread_create(&tid[i], NULL, manage_connection, &active_socket[i])!=0) {
				perror("Thread creation");
				tid[i] = (pthread_t)-1; // to be sure we don't have unknown values... cast
				continue;
			}
			contacts++;
		
		} else {  //too many threads 
			printf("Maximum threads active: closing connection\n");
			write(sd2, busymsg, strlen(busymsg)+1);
			close(sd2);
		}

		printf("nuovo thread attivato\n");

	}
	printf("Server finished\n");
}
