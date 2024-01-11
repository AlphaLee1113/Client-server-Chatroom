#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include "chatroom.h"

#define MAX 1024 // max buffer size
#define PORT 6789  // port number

static int sockfd;

void generate_menu(){
	printf("Hello dear user pls select one of the following options:\n");
	printf("EXIT\t-\t Send exit message to server - unregister ourselves from server\n");
    printf("WHO\t-\t Send WHO message to the server - get the list of current users except ourselves\n");
    printf("#<user>: <msg>\t-\t Send <MSG>> message to the server for <user>\n");
    printf("Or input messages sending to everyone in the chatroom.\n");
}

void recv_server_msg_handler() {
    /********************************/
	/* receive message from the server and display on the screen*/
	/**********************************/
	while(1){
		char buffer[MAX];
		bzero(buffer, sizeof(buffer));
		// printf("Hey my sockfd is %d\n",sockfd);
		if (recv(sockfd, buffer, sizeof(buffer), 0)==-1){
			perror("recv");
		}
		printf("%s", buffer);
	}
}

int main(){
    int n;
	int nbytes;
	struct sockaddr_in server_addr, client_addr;
	char buffer[MAX];
	
	/******************************************************/
	/* create the client socket and connect to the server */
	/******************************************************/
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd == -1){
		printf("Socket creation failed...\n");
		exit(0);
	}
	else
		printf("Socket successfully created...\n");
		
	bzero(&server_addr, sizeof(server_addr));
	
	// assign IP, PORT
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	server_addr.sin_port = htons(PORT);
	
	// connect the client socket to the server socket
	if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) != 0) {
		printf("Connection with the server failed...\n");
		exit(0);
	}
	else
		printf("Connected to the server...\n");
	/******************************************************/
	/* END OF　create the client socket and connect to the server */
	/******************************************************/


	generate_menu();
	//printf(" sockfd is %d\n", sockfd);  // output is 3
	// recieve welcome message to enter the nickname
	//"Welcome to the chat room!"
    bzero(buffer, sizeof(buffer));
	// printf("wait my sockfd is %d\n", sockfd); //output is 3
	nbytes = recv(sockfd, buffer, sizeof(buffer), 0);
    if (nbytes==-1){
        perror("recv");
    }
    printf("%s", buffer);

	/*************************************/
	/* Input the nickname and send a message to the server */
	/* Note that we concatenate "REGISTER" before the name to notify the server it is 
	/          the register/login message*/
	/*******************************************/
	bzero(buffer, sizeof(buffer));
	char name[C_NAME_LEN];
	n = 0;
	while ((buffer[n++] = getchar()) != '\n'); // get the input and count how many words

	char back_up_buffer[MAX];
	strcpy(back_up_buffer, buffer);

	//following code get the username and pw
	int index_of_space = 0;
	for(int x = 0; x < strlen(buffer); x++){ 
		if(buffer[x] == ' '){
			index_of_space=x;
			break;
		}
	}
	
	char pw[MAX];

	bzero(name, sizeof(name));
	bzero(pw, sizeof(pw));

	buffer[strcspn(buffer, "\n")] = 0;  // remove \n char
	int space = 0;
	for(int x = 0; x < strlen(buffer); x++){ 
		if(buffer[x] == ' '){
			space=x;
			break;
		}
	}
	strncpy(name, buffer, space);
	strncpy(pw, buffer + (space+1), strlen(buffer)-1);
	//end 


	// strcpy(name, buffer);  // copy the buffer to the name 
	char register_name[255];
	// char * register_name;
	strcpy(register_name, "REGISTER");
	strcat(register_name, back_up_buffer);

	if (send(sockfd, register_name, strlen(register_name), 0)<0){  // need to add 8 because count register as well
		printf("Sending nickname message failed...");
		exit(1);
	}
	printf("Register/Login message sent to server.\n\n");
	/*************************************/
	/* END OF     Input the nickname 
	/*******************************************/

    // receive welcome message "welcome xx to joint the chatroom. A new account has been created." (registration case)
	//  or "welcome back! The message box contains:..." (login case)
    bzero(buffer, sizeof(buffer));
	// printf("receiving message\n");
    if (recv(sockfd, buffer, sizeof(buffer), 0)==-1){
        perror("recv");
    }
	if(strncmp(buffer, "EXITNOW", 7) == 0){
		printf("%s", "Your password is not correct or not in correct format.\nWe will log you out now\n");
		pthread_exit(&recv_server_msg_handler);
		close(sockfd);
		return 0;
	}
    printf("%s", buffer);

    /*****************************************************/
	/* Create a thread to receive message from the server*/
	/* pthread_t recv_server_msg_thread;*/
	/*****************************************************/
	pthread_t recv_server_msg_thread;
	pthread_create(&recv_server_msg_thread,NULL, (void * (*)(void *))recv_server_msg_handler, NULL);
	// pthread_join(recv_server_msg_thread, NULL);

	/*****************************************************/
	/* END OF Create a thread to receive message from the server*/
	/*****************************************************/
    
	// chat with the server
	for (;;) {
		bzero(buffer, sizeof(buffer));
		n = 0;
		while ((buffer[n++] = getchar()) != '\n')
			;


		if ((strncmp(buffer, "EXIT", 4)) == 0) {
			printf("Client Exit...\n");
			/********************************************/
			/* Send exit message to the server and exit */
			/* Remember to terminate the thread and close the socket */
			/********************************************/
			if (send(sockfd, buffer, sizeof(buffer), 0)<0){
				puts("Sending EXIT MESSAGE failed");
				exit(1);
			}

			pthread_exit(&recv_server_msg_handler);
			// oculd be pthread_exit(NULL);

			// close the socket
			close(sockfd);
			break;
			/********************************************/
			/* END OF Send exit message to the server and exit */
			/* Remember to terminate the thread and close the socket */
			/********************************************/
		}
		else if (strncmp(buffer, "WHO", 3) == 0) {
			printf("Getting user list, pls hold on...\n");
			if (send(sockfd, buffer, sizeof(buffer), 0)<0){
				puts("Sending MSG_WHO failed");
				exit(1);
			}
			printf("If you want to send a message to one of the users, pls send with the format: '#username:message'\n");
		}
		else if (strncmp(buffer, "#", 1) == 0) {
			// If the user want to send a direct message to another user, e.g., aa wants to send direct message 
			//"Hello" to bb, aa needs to input "#bb:Hello"
			if (send(sockfd, buffer, sizeof(buffer), 0)<0){
				printf("Sending direct message failed...");
				exit(1);
			}
		}
		else {
			/*************************************/
			/* Sending broadcast message. The send message should be of the format "username: message"*/
			/**************************************/
			char message[MAX];
			strcpy(message, name);
			message[strcspn(message, "\n")] = 0;
			strcat(message, " : ");
			strcat(message, buffer);
			// then the message become "username: message"
			if (send(sockfd, message, sizeof(message), 0)<0){
				printf("Sending broadcast message failed...");
				exit(1);
			}
			/*************************************/
			/* END OF Sending broadcast message. The send message should be of the format "username: message"*/
			/**************************************/		
		}
	}
	return 0;
}

