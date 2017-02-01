#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>

#define SERVERPORT 5432

#define BUFSIZE 1024
#define FILENAMESIZE 64
#define CMD_OPT_SIZE 128
#define MAX_NAME_LEN 16

//TODO transfer diff file types binary format etc
//TODO additional features --chat history for each client, Ipv4 ipv6, doxygen

enum {
	SET_USERNAME,
	SET_GROUP_TO_SEND,
	SET_USER_TO_SEND,
	CREATE_GROUP,
	GET_CURR_GROUP,
	ADD_USER_TO_GROUP,
	DEL_USER_FROM_GROUP,
	DEFAULT_GROUP,
	DELETE_GROUP,
	PRINT_ALL_GROUPS
} CMD_OPTIONS;

enum {
	SUCCESS = 1,
	FALIURE = -1,
	LIMIT_REACHED = -2,
	GROUP_EXISTS = -3,
	USER_EXISTS = -4
} ERROR_CODES;

struct msg_data {
	char type;
	int filename_size;
	char file_name[FILENAMESIZE];
	char * msg;
};

char target_group[MAX_NAME_LEN];
char target_user[MAX_NAME_LEN];
char username[MAX_NAME_LEN];
char final_file_tx[1024];

int target_set = 0;

void usage() {
	printf("To send file: file filename\n");
	printf("To create new group: cmd create group\n");
	//TODO fill the usage properly
	return;
}

int send_cmd_server(int sockfd, int type, char * str) {
	char cmd_buff[CMD_OPT_SIZE];
	char recv_buff[1024];
	char server_reply;
	memset(cmd_buff, 0, CMD_OPT_SIZE);
	cmd_buff[0] = 'c';
	
	if(type == SET_USERNAME) {
		cmd_buff[1] = 'n';
		strncpy(&(cmd_buff[2]), str, sizeof(str));
		int send_ret = send(sockfd, cmd_buff, strlen(cmd_buff) + 1, 0);
		if(send_ret < 0) {
			perror("Command send error");
		}
	}
	else if(type == SET_GROUP_TO_SEND) {
		cmd_buff[1] = 'g';
		strncpy(&(cmd_buff[2]), str, sizeof(str));
		int send_ret = send(sockfd, cmd_buff, strlen(cmd_buff) + 1, 0);
		if(send_ret < 0) {
			perror("Command send error");
		}
	}
	else if(type == SET_USER_TO_SEND) {
		cmd_buff[1] = 'u';
		strncpy(&(cmd_buff[2]), str, sizeof(str));
		int send_ret = send(sockfd, cmd_buff, strlen(cmd_buff) + 1, 0);
		if(send_ret < 0) {
			perror("Command send error");
		}
	}
	else if(type == DEFAULT_GROUP) {
		cmd_buff[1] = 'd';
		int send_ret = send(sockfd, cmd_buff, strlen(cmd_buff), 0);
		if(send_ret < 0) {
			perror("Command send error");
		}
	}
	else if(type == CREATE_GROUP) {
		cmd_buff[1] = 'c';
		strncpy(&(cmd_buff[2]), str, sizeof(str));
		int send_ret = send(sockfd, cmd_buff, strlen(cmd_buff) + 1, 0);
		if(send_ret < 0) {
			perror("Command send error");
		}
	}
	else if(type == DELETE_GROUP) {
		cmd_buff[1] = 'r';
		strncpy(&(cmd_buff[2]), str, sizeof(str));
		int send_ret = send(sockfd, cmd_buff, strlen(cmd_buff) + 1, 0);
		if(send_ret < 0) {
			perror("Command send error");
		}
	}
	else if(type == PRINT_ALL_GROUPS) {
		printf("PRINT_ALL_GROUPS\n");
		cmd_buff[1] = 'p';
		strncpy(&(cmd_buff[2]), str, sizeof(str));
		printf("string %s\n",str);
		int send_ret = send(sockfd, cmd_buff, strlen(cmd_buff) + 1, 0);
		if(send_ret < 0) {
			perror("Command send error");
		}
		int recv_ret = recv(sockfd, &recv_buff, BUFSIZE, 0);
		printf("%s",recv_buff);
		server_reply = 's';
	}
	else if(type == ADD_USER_TO_GROUP) {
		cmd_buff[1] = 'a';
		strncpy(&(cmd_buff[2]), str, sizeof(str));
		int send_ret = send(sockfd, cmd_buff, strlen(cmd_buff) + 1, 0);
		if(send_ret < 0) {
			perror("Command send error");
		}	
	}
	else if(type == DEL_USER_FROM_GROUP) {
		cmd_buff[1] = 'h';
		strncpy(&(cmd_buff[2]), str, sizeof(str));
		int send_ret = send(sockfd, cmd_buff, strlen(cmd_buff) + 1, 0);
		if(send_ret < 0) {
			perror("Command send error");
		}
	}
	
	//receive reply from server
	int server_reply_val = recv(sockfd, &server_reply, BUFSIZE, 0);
	
	if(server_reply_val < 0) {
		perror("Command reply error");
	}
	
	if(server_reply == 's')
		return SUCCESS;
	else if(server_reply == 'f')
		return FALIURE;
	else if(server_reply == 'l')
		return LIMIT_REACHED;
}

int cmd_mode(int sockfd, char * cmd_opt) {
	//TODO remove \n from cmd strtok
	char * str_set_name = "set name ";
	char * str_set_group = "set target group ";
	char * str_set_user = "set target user ";
	char * str_new_group = "create group ";
	char * str_del_group = "delete group ";
	char * str_get_group = "get target group\n";
	char * str_get_user = "get target user\n";
	char * str_add_user = "add user ";
	char * str_del_user = "delete user ";
	char * str_print_groups = "print all groups\n";
	char * str_quit = "quit\n";
	
	int server_reply = 0;

	if (strncmp(cmd_opt , str_set_name, strlen(str_set_name)) == 0) {
		server_reply = send_cmd_server(sockfd, SET_USERNAME, &cmd_opt[9]);
	}
	else if (strncmp(cmd_opt , str_set_group, strlen(str_set_group)) == 0) {
		server_reply = send_cmd_server(sockfd, SET_GROUP_TO_SEND, &cmd_opt[strlen(str_set_group)]);
		if(server_reply == SUCCESS) {
			strcpy(target_group, &cmd_opt[strlen(str_set_group)]);
			target_set = 1;
			printf("Target group set to %s", target_group);
		}
	}
	else if (strncmp(cmd_opt , str_set_user, strlen(str_set_user)) == 0) {
		server_reply = send_cmd_server(sockfd, SET_USER_TO_SEND, &cmd_opt[strlen(str_set_user)]);
		if(server_reply == SUCCESS) {
			strcpy(target_user, cmd_opt);
			target_set = 2;
			printf("Target user set to %s", target_user);
		}
	}
	else if (strncmp(cmd_opt , str_new_group, strlen(str_new_group)) == 0) {
		server_reply = send_cmd_server(sockfd, CREATE_GROUP, &cmd_opt[strlen(str_new_group)]);
		if(server_reply == SUCCESS) {
			strcpy(target_group, &cmd_opt[strlen(str_new_group)]);
			target_set = 1;
			printf("Target group set to %s", target_group);
		}
	}
	else if (strncmp(cmd_opt , str_del_group, strlen(str_del_group)) == 0) {
		server_reply = send_cmd_server(sockfd, DELETE_GROUP, &cmd_opt[strlen(str_del_group)]);
	}
	else if (strcmp(cmd_opt , str_get_group) == 0) {
		printf("Current group name is %s\n", target_group);
	}
	else if (strcmp(cmd_opt , str_get_user) == 0) {
		printf("Current group name is %s\n", target_user);
	}
	else if (strncmp(cmd_opt , str_add_user, strlen(str_add_user)) == 0) {
		server_reply = send_cmd_server(sockfd, ADD_USER_TO_GROUP, &cmd_opt[strlen(str_add_user)]);
	}
	else if (strncmp(cmd_opt , str_del_user, strlen(str_del_user)) == 0) {
		server_reply = send_cmd_server(sockfd, DEL_USER_FROM_GROUP, &cmd_opt[strlen(str_del_user)]);
	}
	else if (strcmp(cmd_opt , str_print_groups) == 0) {
		server_reply = send_cmd_server(sockfd, PRINT_ALL_GROUPS, cmd_opt);
	}
	else if (strcmp(cmd_opt , str_quit) == 0) {
		printf("Bye :)\n");
		exit(0);
	}
	else {
		printf("Invalid command\n");
		return FALIURE;
	}
	
	if(server_reply == SUCCESS)
		printf("Command succeded\n");
	else if(server_reply == FALIURE)
		printf("Command failed\n");
	else if(server_reply == LIMIT_REACHED)
		printf("Limit reached\n");
	else if(server_reply == GROUP_EXISTS) {
		printf("GROUP EXISTS\n");
	}
	
	return 0;
}

int send_file(int sockfd, char * file_name) {
	strtok(file_name, "\n");

	FILE * f = fopen(file_name, "r");
	
	if(f == NULL) {
		printf("file not there\n");
		return FALIURE;
	}
	fseek(f, 0, SEEK_END);
	int fsize = ftell(f);
	fseek(f, 0, SEEK_SET);  //same as rewind(f);

	char * file_data = malloc(fsize + 1);
	fread(file_data, fsize, 1, f);
	fclose(f);
	file_data[fsize] = 0;
	
	struct msg_data final_msg;
	final_msg.type = 'f';
	final_msg.msg = file_data;
	
	char file_size_char[4];

	memset(final_file_tx, 0, 1024);
	final_file_tx[0] = final_msg.type;
	
	sprintf(file_size_char, "%04d", (int) strlen(file_name));
	strcat(&final_file_tx[1], file_size_char);
	
	strcat(&final_file_tx[strlen(file_size_char)], file_name);
	
	strcat(&final_file_tx[strlen(file_name)+strlen(file_size_char)], final_msg.msg);
	
	int send_ret = send(sockfd, final_file_tx, strlen(final_file_tx) + 1, 0);
	if(send_ret < 0) {
		perror("File send error");
	}
	
	free(file_data);
	
	return 0;
}

int recv_file(char * recv_buf, int bytes_rcvd) {
	int filename_size = (int)strtol(&recv_buf[2], NULL, 16);

	char file_name[FILENAMESIZE];
	memset(file_name, 0, FILENAMESIZE);
	
	strncpy(file_name, (char *) &recv_buf[5], filename_size);
	
	char data_rcvd[1024];
	
	strncpy(data_rcvd, (char *) &recv_buf[5+filename_size], bytes_rcvd - (5+filename_size));
	
	printf("file name size %d file_name %s, data %s\n", filename_size, file_name, data_rcvd);
	
	FILE * f_w = fopen(file_name, "w+");
	int write = fwrite(data_rcvd, sizeof(char), bytes_rcvd - (5+filename_size), f_w);
	fclose(f_w);

	return 0;
}

void send_recv(int i, int sockfd) {
	char send_buf[BUFSIZE];
	char recv_buf[BUFSIZE];
	
	int nbyte_recvd;
	
	if(i == 0) {
		fgets(&send_buf[1], BUFSIZE, stdin);
		if (strncmp(&send_buf[1] , "file ", 5) == 0){
			send_file(sockfd, &send_buf[6]);
		}
		else if(strncmp(&send_buf[1] , "cmd ", 4) == 0) {
			cmd_mode(sockfd, &send_buf[5]);
		}
		else {
			send_buf[0] = 'm';
			send(sockfd, send_buf, strlen(send_buf), 0);
		}
	}
	else {
		nbyte_recvd = recv(sockfd, recv_buf, BUFSIZE, 0);
		recv_buf[nbyte_recvd] = '\0';
		
		if (recv_buf[0]== 'f') {
			printf("file received\n");
			recv_file(recv_buf, nbyte_recvd);
		} else {
			printf("%s" , &recv_buf[1]);
		}
		
		fflush(stdout);
	}
}
		
void connect_request(int *sockfd, struct sockaddr_in *server_addr) {
	if ((*sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		perror("Socket");
		exit(1);
	}
	server_addr->sin_family = AF_INET;
	server_addr->sin_port = htons(SERVERPORT);
	server_addr->sin_addr.s_addr = inet_addr("127.0.0.1");
	memset(server_addr->sin_zero, '\0', sizeof server_addr->sin_zero);
	
	if(connect(*sockfd, (struct sockaddr *)server_addr, sizeof(struct sockaddr)) == -1) {
		perror("connect");
		exit(1);
	}
}
	
int main() {
	int sockfd, fdmax, server_reply = 0;
	struct sockaddr_in server_addr;
	char cmd_opt[CMD_OPT_SIZE];
	int username_not_set = 1, curr_group_not_set = 1, grp_cmd_rep, user_cmd_rep;
	
	fd_set master;
	fd_set read_fds;
	
	connect_request(&sockfd, &server_addr);
	FD_ZERO(&master);
	FD_ZERO(&read_fds);
	FD_SET(0, &master);
	FD_SET(sockfd, &master);
	fdmax = sockfd;
	
	while(username_not_set && curr_group_not_set) {
		printf("Enter USERNAME\n");
		fgets(cmd_opt, CMD_OPT_SIZE, stdin);
		user_cmd_rep = send_cmd_server(sockfd, SET_USERNAME, cmd_opt);
		
		if(user_cmd_rep == FALIURE) {
			printf("Username not valid, enter another name, rep %d\n", user_cmd_rep);
		}
		else if(user_cmd_rep == SUCCESS) {
			strcpy(target_group, "default");
			target_set = 1;
			printf("Username set successfully\n");
			printf("You will have to select a target user or group before massages can be sent\n");
			username_not_set = 0;
		}
		else if(user_cmd_rep == LIMIT_REACHED) {
			printf("User limit reached\n");
			exit(1);
		}
	}
	
	while(1){
		read_fds = master;
		if(select(fdmax+1, &read_fds, NULL, NULL, NULL) == -1){
			perror("select");
			exit(4);
		}
		
		for(int i=0; i <= fdmax; i++ ) {
			if(FD_ISSET(i, &read_fds))
				send_recv(i, sockfd);
		}
	}
	printf("client-quited\n");
	close(sockfd);
	return 0;
}