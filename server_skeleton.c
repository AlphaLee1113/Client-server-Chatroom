#include <stdio.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <ctype.h>
#include "chatroom.h"
#include <poll.h>

#define MAX 1024 // max buffer size
#define PORT 6789 // server port number
#define MAX_USERS 50 // max number of users
static unsigned int users_count = 0; // number of registered users

static user_info_t *listOfUsers[MAX_USERS] = {0}; // list of users


/* Add user to userList */
void user_add(user_info_t *user);
/* Get user name from userList */
char * get_username(int sockfd);
/* Get user sockfd by name */
int get_sockfd(char *name);

/* Add user to userList */
void user_add(user_info_t *user){
	if(users_count ==  MAX_USERS){
		printf("sorry the system is full, please try again later\n");
		return;
	}
	/***************************/
	/* add the user to the list */
	/**************************/
	// printf("users_count now is %d\n", users_count);
	listOfUsers[users_count] = user;
	// if(users_count!=0)
	// 	printf("listOfUsers[users_count-1]->username now is %s\n", listOfUsers[users_count-1]->username);
	users_count++;
}

/* Determine whether the user has been registered  */
int isNewUser(char* name) { 
	int i;
	int flag = -1;
	/*******************************************/
	/* Compare the name with existing usernames */
	/*******************************************/
	for(i = 0; i<users_count; i++){
		if(strncmp(listOfUsers[i]->username, name, C_NAME_LEN) == 0){
			//found name in the list and it is old account
			flag = 1;
			break;
		}
		else
		{
			/* cannot find the name and its a new account */
			continue;
		}
	}
	return flag;
}

/* Get user name from userList */
char * get_username(int ss){
	int i;
	static char uname[MAX];
	strcpy(uname, "CANNOT FIND THIS NAME");
	/*******************************************/
	/* Get the user name by the user's sock fd */
	/*******************************************/
	// printf("now ss is %d\n", ss);
	for(i = 0; i<users_count; i++){
		if(listOfUsers[i]->sockfd == ss){
			//found socketfd in the list
			strcpy(uname, listOfUsers[i]->username);
			break;
		}
		else
		{
			/* cannot find the socketfd */
			continue;
		}		
	}
	return uname;
}

/* Get user sockfd by name */
int get_sockfd(char *name){

	// for(int x = 0; x < users_count; x++){
	// 	printf("INPORTANT listOfUsers[x]->sockfd is %d\n",listOfUsers[x]->sockfd);
	// }


	int i;
	int sock = -1;
	/*******************************************/
	/* Get the user sockfd by the user name */
	/*******************************************/
	for(i = 0; i<users_count; i++){
		if(strncmp(listOfUsers[i]->username, name, C_NAME_LEN) == 0){
			//found name in the list
			sock = listOfUsers[i]->sockfd;
			break;
		}
		else
		{
			/* cannot find the socketfd */
			continue;
		}		
	}
	return sock;
}
// The following two functions are defined for poll()
// Add a new file descriptor to the set
void add_to_pfds(struct pollfd* pfds[], int newfd, int* fd_count, int* fd_size)
{
	// If we don't have room, add more space in the pfds array
	if (*fd_count == *fd_size) {
		*fd_size *= 2; // Double it

		*pfds = realloc(*pfds, sizeof(**pfds) * (*fd_size));
	}

	(*pfds)[*fd_count].fd = newfd;
	(*pfds)[*fd_count].events = POLLIN; // Check ready-to-read

	(*fd_count)++;
}
// Remove an index from the set
void del_from_pfds(struct pollfd pfds[], int i, int* fd_count)
{
	// Copy the one from the end over this one
	pfds[i] = pfds[*fd_count - 1];

	(*fd_count)--;
}



int main(){
	int listener;     // listening socket descriptor
	int newfd;        // newly accept()ed socket descriptor
	int addr_size;     // length of client addr
	struct sockaddr_in server_addr, client_addr;
	
	char buffer[MAX]; // buffer for client data
	int nbytes;
	int fd_count = 0;
	int fd_size = 5;
	struct pollfd* pfds = malloc(sizeof * pfds * fd_size);
	
	int yes=1;        // for setsockopt() SO_REUSEADDR, below
    int i, j, u, rv;

    
	/**********************************************************/
	/*create the listener socket and bind it with server_addr*/
	/**********************************************************/
	listener = socket(AF_INET, SOCK_STREAM, 0);
	if (listener == -1) {
		printf("Socket creation failed...\n");
		exit(0);
	}
	else
		printf("Socket successfully created..\n");
	
	bzero(&server_addr, sizeof(server_addr));
	
	// asign IP, PORT
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	server_addr.sin_port = htons(PORT);

	setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
	
	// Binding newly created socket to given IP and verification
	if ((bind(listener, (struct sockaddr*)&server_addr, sizeof(server_addr))) != 0) {
		printf("Socket bind failed...\n");
		exit(0);
	}
	else{
		printf("Socket successfully binded..\n");
	}
	/**********************************************************/
	/* END OF create the listener socket and bind it with server_addr*/
	/**********************************************************/

	// Now server is ready to listen and verification
	if ((listen(listener, 5)) != 0) {
		printf("Listen failed...\n");
		exit(3);
	}
	else
		printf("Server listening..\n");
		
	// Add the listener to set
	pfds[0].fd = listener;
	pfds[0].events = POLLIN; // Report ready to read on incoming connection
	fd_count = 1; // For the listener
	
	// main loop
	for(;;) {
		/***************************************/
		/* use poll function */
		/**************************************/
		int poll_count = poll(pfds, fd_count, -1);
		if(poll_count == -1){
			perror("poll");
			exit(1);
		}
		/***************************************/
		/* END OF use poll function */
		/**************************************/


		// run through the existing connections looking for data to read
        	for(i = 0; i < fd_count; i++) {
            	  if (pfds[i].revents & POLLIN) { // we got one!!
                    if (pfds[i].fd == listener) {
						/**************************/
						/* we are the listener and we need to handle new connections from clients */
						/****************************/

						//handle new conncction
						addr_size = sizeof(client_addr);
						int client_socket = accept(listener, (struct sockaddr*)&client_addr, &addr_size);
						//printf("newfd now is %d", newfd);

						if(client_socket == -1){
							perror("accept");
						}
						else{
							add_to_pfds(&pfds, client_socket, &fd_count, &fd_size);
							// original is following
							printf("pollserver: new connection from %s on socket %d\n",inet_ntoa(client_addr.sin_addr),client_socket);
							// printf("IMPORTANT the client_socket is %d\n ",client_socket);  //client_socket is 4
							//printf("client_socket now is %d", client_socket);
							
						}
						/**************************/
						/* END OF  handle new connections from clients */
					  	/****************************/
						// send welcome message
						bzero(buffer, sizeof(buffer));
						strcpy(buffer, "Welcome to the chat room!\nPlease enter a nickname.\n");
						strcat(buffer, "After input your name please click the spcae bar and continue entering your password.\n");
						//ORIGINAL
						if (send(client_socket, buffer, sizeof(buffer), 0) == -1){  
							perror("send");
						}
                    } else {
                        // handle data from a client
						bzero(buffer, sizeof(buffer));
						int nbytes = recv(pfds[i].fd, buffer, sizeof(buffer), 0);
                        if (nbytes <= 0) {
                          // got error or connection closed by client
                          if (nbytes == 0) {
                            // connection closed
                            printf("pollserver: socket %d hung up\n", pfds[i].fd);
                          } else {
                            perror("recv");
                          }
						  close(pfds[i].fd); // Bye!
						  del_from_pfds(pfds, i, &fd_count);
                        } 
						else {
                            // we got some data from a client
							if (strncmp(buffer, "REGISTER", 8)==0){
								printf("Got register/login message\n");
								/********************************/
								/* Get the user name and add the user to the userlist*/
								/**********************************/

								//CORRECT is following
								// char * name;
								// strncpy(name, buffer + 8, strlen(buffer) - 8); //remove the "REGISTER"
								// name[strcspn(name, "\n")] = 0;  // remove \n char
								// printf("The name of the user of incoming connection is: %s\n", name);
								int index_of_space = 0;
								for(int x = 0; x < strlen(buffer); x++){ 
									if(buffer[x] == ' '){
										index_of_space=x;
										break;
									}
								}
								char truncated_message[MAX];
								char name[MAX];
								char pw[MAX];

								bzero(truncated_message, sizeof(truncated_message));
								bzero(name, sizeof(name));
								bzero(pw, sizeof(pw));

								strncpy(truncated_message, buffer + 8, strlen(buffer)-8);
								truncated_message[strcspn(truncated_message, "\n")] = 0;  // remove \n char
								int space = 0;
								for(int x = 0; x < strlen(truncated_message); x++){ 
									if(truncated_message[x] == ' '){
										space=x;
										break;
									}
								}
								if(space == 0){
									bzero(buffer, sizeof(buffer));
									strcpy(buffer, "EXITNOW");
									send(pfds[i].fd, buffer, sizeof(buffer), 0);  
									
									//close the socket and remove the socket from pfds[]
									close(pfds[i].fd);
									del_from_pfds(pfds, i, &fd_count);
									break;
								}

								strncpy(name, truncated_message, space);
								strncpy(pw, truncated_message + (space+1), strlen(truncated_message)-1);

								if (isNewUser(name) == -1) {
									/********************************/
									/* it is a new user and we need to handle the registration*/
									/**********************************/
									user_info_t * new_user = malloc(sizeof(user_info_t));
									// printf("IMPORTANT pfds[i].fd is %d\n",pfds[i].fd);//Alpha is 4 and Beta is 6
									// but the destsock of Alpha is 4197488
									new_user->sockfd = pfds[i].fd; 
									strcpy(new_user->username, name);  
									new_user->state = 1;
									strcpy(new_user->password, pw); ;  // set the pw to empty first later change
									user_add(new_user);
									/********************************/
									/* END OF it is a new user and we need to handle the registration*/
									/**********************************/

									/********************************/
									/* create message box (e.g., a text file) for the new user */
									/**********************************/
									FILE *fp;
									char filename[C_NAME_LEN+4];
									// filename[0] = '\0'; // clear the filename first
									strcpy(filename, name);
									strcat(filename,".txt");
									// the filename become name.txt
									fp = fopen( filename,"w");   // original is a 
									/********************************/
									/* END OF create message box (e.g., a text file) for the new user */
									/**********************************/

									// broadcast the welcome message (send to everyone except the listener)
									bzero(buffer, sizeof(buffer));
									strcpy(buffer, "Welcome ");
									strcat(buffer, name);
									strcat(buffer, " to join the chat room!\n");

									/*****************************/
									/* Broadcast the welcome message*/
									/*****************************/

									for(int x = 0; x < users_count; x++){
										int dest_fd = listOfUsers[x]->sockfd;
										if( dest_fd!=0){
											send(dest_fd, buffer, sizeof(buffer), 0);
										}
										
									}
									/*****************************/
									/* END OF Broadcast the welcome message*/
									/*****************************/

									/*****************************/
									/* send registration success message to the new user*/
									/*****************************/
									bzero(buffer, sizeof(buffer));
									strcpy(buffer, "A new account has been created.\n");
									send(pfds[i].fd, buffer, sizeof(buffer), 0);

									printf("add user name: %s\n", name);
									/*****************************/
									/* END OF send registration success message to the new user*/
									/*****************************/
									// bzero(name, sizeof(name)); // reset the name to NULL
								}
								else {
									/********************************/
									/* it's an existing user and we need to handle the login. Note the state of user,*/
									/**********************************/
									char correct_pw[MAX];

									for(int x = 0; x < users_count; x++){
										if(strcmp(listOfUsers[x]->username,name)==0){
											listOfUsers[x]->state = 1; // note as online
											listOfUsers[x]->sockfd = pfds[i].fd; // update the socet
											strcpy(correct_pw, listOfUsers[x]->password);
											break;
										}
									}

									if(strcmp(correct_pw, pw)!=0){ // password incorrect
										bzero(buffer, sizeof(buffer));
										strcpy(buffer, "EXITNOW");
										send(pfds[i].fd, buffer, sizeof(buffer), 0);

										int index_of_user = 0;
										for(int x = 0; x < users_count; x++) { // start from 1 become ignore listener
											if(listOfUsers[x]->sockfd == pfds[i].fd){
												index_of_user = x;
											}
										}
										listOfUsers[index_of_user]->state = 0;
										listOfUsers[index_of_user]->sockfd = 0;  
										
										//close the socket and remove the socket from pfds[]
										close(pfds[i].fd);
										del_from_pfds(pfds, i, &fd_count);
										break;
									}
									/********************************/
									/* END of it's an existing user and we need to handle the login. Note the state of user,*/
									/**********************************/


									/********************************/
									/* send the offline messages to the user and empty the message box*/
									/**********************************/
									char filename_of_user[MAX];
									char  line[MAX];
									bzero(filename_of_user, sizeof(filename_of_user));
									strcpy(filename_of_user, name);
									strcat(filename_of_user, ".txt");

									FILE *fp = fopen(filename_of_user, "r");

									fseek (fp, 0, SEEK_END);
  									int length = ftell (fp); // find length if file
									fseek (fp, 0, SEEK_SET); // reset the pointer to start

									if (fp == NULL){
										printf("Error reading the file %s", filename_of_user);
									}

									fread (line, 1, length, fp);
									// close the file


									///***following part is for chceking is file empty
									fseek (fp, 0, SEEK_END);
									int size = ftell(fp);
									// if (0 == size) {
									// 	printf("file is empty\n");
									// }

									fclose(fp);

									bzero(buffer, sizeof(buffer));
									strcpy(buffer, "Welcome back! The messages box contains:\n");
									strcat(buffer, line);

									//line[strcspn(line, "\n")] = 0;
									if(size == 0){
										bzero(buffer, sizeof(buffer));
										strcpy(buffer, "Welcome back! The messages box is empty.\n");
									}

									send(pfds[i].fd, buffer, sizeof(buffer), 0);

									fclose(fopen(filename_of_user, "w")); // remove all content in the file
									/********************************/
									/* END OF send the offline messages to the user and empty the message box*/
									/**********************************/

								
									// broadcast the welcome message (send to everyone except the listener)
									bzero(buffer, sizeof(buffer));
									strcpy(buffer, name);
									strcat(buffer, " is online!\n");
									/*****************************/
									/* Broadcast the welcome message*/
									/*****************************/
									for(int x = 0; x < users_count; x++){
										int dest_fd = listOfUsers[x]->sockfd;
										if(dest_fd != pfds[i].fd  && dest_fd!=0){ // skip the user who is calling 
											send(dest_fd, buffer, sizeof(buffer), 0);
										}
									}
									/*****************************/
									/* END OF Broadcast the welcome message*/
									/*****************************/
								}
							}
							else if (strncmp(buffer, "EXIT", 4)==0){
								printf("Got exit message. Removing user from system\n");
								// send leave message to the other members
                                bzero(buffer, sizeof(buffer));
								strcpy(buffer, get_username(pfds[i].fd));
								strcat(buffer, " has left the chatroom\n");
								/*********************************/
								/* Broadcast the leave message to the other users in the group*/
								/**********************************/
								for(int x = 0; x < users_count; x++){
									int dest_fd = listOfUsers[x]->sockfd;
									if(dest_fd != pfds[i].fd  && dest_fd!=0){ // skip the user who is calling 
										send(dest_fd, buffer, sizeof(buffer), 0);
									}
								}
								/*********************************/
								/* END OF Broadcast the leave message to the other users in the group*/
								/**********************************/

								/*********************************/
								/* Change the state of this user to offline*/
								/**********************************/
								int index_of_user = 0;
								for(int x = 0; x < users_count; x++) { // start from 1 become ignore listener
									if(listOfUsers[x]->sockfd == pfds[i].fd){
										index_of_user = x;
									}
								}
								listOfUsers[index_of_user]->state = 0;
								listOfUsers[index_of_user]->sockfd = 0;  //set to 0 so that no one can send to it
								/*********************************/
								/* END OF Change the state of this user to offline*/
								/**********************************/
								
								//close the socket and remove the socket from pfds[]
								close(pfds[i].fd);
								del_from_pfds(pfds, i, &fd_count);
							}
							else if (strncmp(buffer, "WHO", 3)==0){
								// concatenate all the user names except the sender into a char array
								printf("Got WHO message from client.\n");
								char ToClient[MAX];
								bzero(ToClient, sizeof(ToClient));
								/***************************************/
								/* Concatenate all the user names into the tab-separated char ToClient and send it to the requesting client*/
								/* The state of each user (online or offline)should be labelled.*/
								/***************************************/

								//* means the user online
								for(int x = 0; x < users_count; x++) { 
									if(x == 0 && listOfUsers[x]->sockfd!=pfds[i].fd){
										strcpy(ToClient, listOfUsers[x]->username);
										if(listOfUsers[x]->state == 1){
									   		 strcat(ToClient,"*") ;
										}
										strcat(ToClient,"\t") ;
										continue;
									}

									if(listOfUsers[x]->sockfd!=pfds[i].fd){
										strcat(ToClient, listOfUsers[x]->username);  
										if(listOfUsers[x]->state == 1){
											strcat(ToClient,"*") ;
										}
										strcat(ToClient,"\t") ;
									}
								}
								strcat(ToClient,"\n* means this user online\n") ;

								if(users_count == 1){
									strcpy(ToClient,"\nThere are no other user except you\n") ;
								}

								if (send(pfds[i].fd, ToClient, sizeof(ToClient), 0) == -1){
									perror("send");
								}
								/***************************************/
								/* END OF Concatenate all the user names into the tab-separated char ToClient and send it to the requesting client*/
								/* The state of each user (online or offline)should be labelled.*/
								/***************************************/
							}
							else if (strncmp(buffer, "#", 1)==0){
								// send direct message 
								// get send user name:
								printf("Got direct message.\n");
								// get which client sends the message
								char sendname[C_NAME_LEN];
								// get the destination username
								char destname[C_NAME_LEN];
								// get dest sock
								int destsock;
								// get the message
								char msg[MAX];

								// int check_message_flag = 0;
								/**************************************/
								/* Get the source name xx, the target username and its sockfd*/
								/*************************************/
								int index_of_colon = 0;
								for(int x = 0; x < strlen(buffer); x++){ // because we ignore the "#" and first char of the name
									if(buffer[x] == ':'){ // we check the name between "#" to ":"
										index_of_colon=x; // from next loop on start copying the message instead of name
										break;
									}
								}
								strncpy(destname, buffer + 1, index_of_colon);
								destname[strcspn(destname, ":")] = 0;  // remove : char

								strncpy(msg, buffer + (index_of_colon+1), strlen(buffer)-1);
								// msg[strcspn(msg, "\n")] = 0;  // dont need remove \n char because we still need the next line

								destsock = get_sockfd(destname); // get the dest socket fd

								strcpy(sendname,get_username(pfds[i].fd)); // get the send name

								printf("destname is %s\n", destname);
								printf("msg is %s", msg);
								printf("destsock is %d\n", destsock);
								printf("sendname is %s\n", sendname);
							
								/**************************************/
								/* END OF Get the source name xx, the target username and its sockfd*/
								/*************************************/


								if (destsock == -1) {
									/**************************************/
									/* The target user is not found. Send "no such user..." messsge back to the source client*/
									/*************************************/
									bzero(buffer, sizeof(buffer));
									strcpy(buffer,"There is no such server. Please check your input format.\n");
									if (send(pfds[i].fd, buffer, sizeof(buffer), 0) == -1){
										perror("send");
									}

									// int send_sock = get_sockfd(sendname);
									// printf("send_sock is %d\n", send_sock);
									// send(send_sock, buffer, sizeof(buffer), 0);
									/**************************************/
									/* END OF The target user is not found. Send "no such user..." messsge back to the source client*/
									/*************************************/

								}
								else {
									// The target user exists.
									// concatenate the message in the form "xx to you: msg"
									char sendmsg[MAX];
									strcpy(sendmsg, sendname);
									strcat(sendmsg, " to you: ");
									strcat(sendmsg, msg);
									// printf("Hey sendmsg is %s\n", sendmsg);  //no problem can output correctly

									/**************************************/
									/* According to the state of target user, send the msg to online user or write the msg into offline user's message box*/
									/* For the offline case, send "...Leaving message successfully" message to the source client*/
									/*************************************/

									//get the index of the dest user so that we can check he is online or offline
									int index_of_dest = 0;
									for(int x = 0; x < users_count; x++){
										if(strcmp(listOfUsers[x]->username,destname)==0){
											//same name and get the index of the dest name
											index_of_dest = x;
										}
									}
									//if user is offline
									if(listOfUsers[index_of_dest]->state == 0){ // the dest user is offline
										//write in the file
										char filename_of_dest[MAX];
										strcpy(filename_of_dest, destname);
										strcat(filename_of_dest, ".txt");

										FILE *fp = fopen(filename_of_dest, "a+");
										// printf(" before fseek %ld", ftell(fp));
										fseek( fp, 0, SEEK_END);
										// printf(" After fseek %ld", ftell(fp));

										if (fp == NULL){
											printf("Error opening the file %s", filename_of_dest);
										}
										// for (int i = 0; i < strlen(sendmsg); i++){
										// 	fprintf(fp, sendmsg[i]);
										// }
										fputs(sendmsg, fp);
										// fprintf(fp, "%s", sendmsg);
										// close the file
										fclose(fp);

										//send "...Leaving message successfully" message to the source client
										bzero(buffer, sizeof(buffer));
										strcpy(buffer,destname);
										strcat(buffer," is offline. Leaving message successfully\n");
										if (send(pfds[i].fd, buffer, sizeof(buffer), 0) == -1){
											perror("send");
										}
									}
									else if(listOfUsers[index_of_dest]->state == 1){ // the dest user is online
										//send the message
										// if (send(destsock, sendmsg, sizeof(sendmsg), 0) == -1){
										// 	perror("send");

										//above line cannot output the string but sendmsg is correct
										//This line cauese send;bad file descriptor
										if (send(destsock, sendmsg, sizeof(sendmsg), 0) == -1){
											perror("send");
										}
									}
								
								}
								// ORIGINAL have this line
								//This line cauese send;bad file descriptor because sendmsg is not defined
								// if (send(destsock, sendmsg, sizeof(sendmsg), 0) == -1){
								// 	perror("send");
								// }

							}
							else{
								printf("Got broadcast message from user\n");
								/*********************************************/
								/* Broadcast the message to all users except the one who sent the message*/
								/*********************************************/
								for(int x = 0; x < users_count; x++){
									int dest_fd = listOfUsers[x]->sockfd;
									if(dest_fd != pfds[i].fd && dest_fd!=0){ // skip the user who is calling 
										send(dest_fd, buffer, sizeof(buffer), 0);
									}
									
								}
							}   

                        }
                    } // end handle data from client
                  } // end got new incoming connection
                } // end looping through file descriptors
        } // end for(;;) 
	return 0;
}
