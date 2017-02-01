#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
	
#define SERVERPORT 5432
#define BUFSIZE 1024
#define MAX_NAME_LEN 16
#define TOTAL_USERS 32
#define TOTAL_GROUPS 32
#define MAX_USERS_PER_GROUP 32
#define DEFAULT_GRP_ID 0

int add_usr_to_grp(int grp_id, int user_port);
int get_user_id_by_port(int user_port);
int set_target_group(int user_port, char * group_name);

//TODO remove newlines from names received

enum {
	SUCCESS = 1,
	FALIURE = -1,
	LIMIT_REACHED = -2,
	GROUP_EXISTS = -3,
	USER_EXISTS = -4
} ERROR_CODES;

enum {
	SEND_TO_GROUP = 1,
	SEND_TO_USER = 2, 
	NOT_SET = -1
} TARGET_SET;

struct group {
	char name[MAX_NAME_LEN];
	int member_ports[MAX_USERS_PER_GROUP];
	int num_users;
};

struct user_data {
	int port;
	char name[MAX_NAME_LEN];
	int target_group;
	int target_user;
	int target_set;
};

struct group groups[TOTAL_GROUPS];
struct user_data users[TOTAL_USERS];

/** init mem to NULL or 0 */
int init_mem() {
	strcpy(groups[DEFAULT_GRP_ID].name, "default");
	
	for(int i=0; i < TOTAL_USERS; i++) {
		users[i].port = 0;
		memset ((void *) users[i].name, 0, MAX_NAME_LEN);
		users[i].target_group = NOT_SET;
		users[i].target_user = NOT_SET;
		users[i].target_set = NOT_SET;
	}
	
	for(int i=0; i < TOTAL_GROUPS; i++) {
		for(int j=0; j < MAX_USERS_PER_GROUP; j++) {
			groups[i].member_ports[j] = 0;
			//groups[i].name[0] = '\n';
			memset ((void *) groups[i].name, 0, MAX_NAME_LEN);
			groups[i].num_users = 0;
		}
	}
	
	return SUCCESS;	
}

int delete_user(int user_port) {
	printf("Deleteing user %d\n", user_port);
	int user_id = get_user_id_by_port(user_port);
	if(user_id == FALIURE)
		return FALIURE;
	for(int i=0; i < TOTAL_GROUPS; i++) {
		for(int j=0; j < MAX_USERS_PER_GROUP; j++) {
			if(groups[i].member_ports[j] == user_port)
				groups[i].member_ports[j] = 0;
		}
	}
	
	users[user_id].port = 0;
	memset ((void *) users[user_id].name, 0, MAX_NAME_LEN);
	users[user_id].target_group = NOT_SET;
	users[user_id].target_user = NOT_SET;
	users[user_id].target_set = NOT_SET;
	
	return SUCCESS;
}

int delete_group(int group_id) {
	char * msg = "Info: Current group set to default";
	for(int i=0; i<TOTAL_USERS; i++) {
		if(users[i].target_group == group_id) {
			send(users[i].port, msg, strlen(msg), 0);
		}
	}
	for(int i=0; i < MAX_USERS_PER_GROUP; i++) {
			groups[group_id].member_ports[i] = 0;
	}
	memset ((void *) groups[group_id].name, 0, MAX_NAME_LEN);
	groups[group_id].num_users = 0;
	return SUCCESS;
}

int get_free_userid() {
	for(int i=0; i < TOTAL_USERS; i++) {
		if(users[i].port == 0) {
			//printf("Found free used id at %d\n", i);
			return i;
		}
	}
	return -1;
}

int get_free_groupid() {
	/** Leave the default group out of search */
	for(int i=1; i < TOTAL_GROUPS; i++) {
		if(groups[i].num_users == 0) {
			printf("Empty group at id %d found\n", i);
			return i;
		}
	}
	return -1;
}

int msg_to_user(int client_port, int return_type) {
	char reply_char;
	
	switch(return_type) {
		case FALIURE:
			reply_char = 'f';
			printf("FALIURE\n");
		break;
		case SUCCESS:
			printf("SUCCESS\n");
			reply_char = 's';
		break;
		case LIMIT_REACHED:
			printf("LIMIT_REACHED\n");
			reply_char = 'l';
		break;
		default:
			printf("DEFAULT\n");
			reply_char = 'f';
		break;
	}

	int send_ret = send(client_port, &reply_char, sizeof(reply_char), MSG_EOR|MSG_NOSIGNAL);
	if (send_ret<0)
		printf("Send failed %d\n", send_ret);
	else if (send_ret==0)
			printf("the peer shut down.\n");
	
	printf("Sent msg_to_user %d %c reply_char\n", client_port, reply_char);
	
	return 0;
}

int get_group_id_by_name(char * group_name) {
	for(int i = 0; i < TOTAL_GROUPS; i++) {
		printf("Comparing group %s with name %s\n", groups[i].name, group_name);
		if(!strcmp(groups[i].name, group_name)){
			printf("Group %s found on id %d\n", group_name, i);
			return i;
		}
	}
	return -1;
}

int get_user_id_by_name(char * user_name) {
	for(int i = 0; i < TOTAL_USERS; i++) {
		if(!strcmp(users[i].name, user_name)){
			return i;
		}
	}
	return FALIURE;
}

int get_user_port_by_name(char * user_name) {
	for(int i = 0; i < TOTAL_USERS; i++) {
		if(!strcmp(users[i].name, user_name)){
			return users[i].port;
		}
	}
	return FALIURE;
}

int find_user_in_group(int group_id, int user_port) {
	int i = 0;
	for(i = 0; i < MAX_USERS_PER_GROUP; i++) {
		if(groups[group_id].member_ports[i] == user_port)
			return i;
	}
	return FALIURE;
}

int get_user_id_by_port(int user_port) {
	if(user_port) {
		for(int i = 0; i < TOTAL_USERS; i++) {
			if(users[i].port == user_port){
				return i;
			}
		}
	}
	return FALIURE;
}

int create_group(int user_port, char * group_name) {
	if(group_name == NULL)
		return FALIURE;
	
	/** check if group name already exists **/
	int temp_group_id = get_group_id_by_name(group_name);
	
	if(temp_group_id >= 0) {
		return GROUP_EXISTS;
	}
	else {
		temp_group_id = get_free_groupid();
		if(temp_group_id == -1) {
			return LIMIT_REACHED;
		}
		
		printf("Reserving group id %d for group %s\n", temp_group_id, group_name);
		strcpy(groups[temp_group_id].name, group_name);
		groups[temp_group_id].member_ports[0] = user_port;
		groups[temp_group_id].num_users = 1;
	}
	if(set_target_group(user_port, group_name) != SUCCESS) {
		return FALIURE;
	}
	return SUCCESS;
}

int get_target_group_id(int user_port) {
	for(int i = 0; i < TOTAL_USERS; i++){
		if(users[i].port == user_port)
			return users[i].target_group;
	}
	return -1; 
}

int set_target_group(int user_port, char * group_name) {
	int temp_group_id = get_group_id_by_name(group_name);
	int user_id = get_user_id_by_port(user_port);
	
	printf("set_target_group user_port %d, group_id %d, user_id %d, grp name %s\n", user_port, temp_group_id, user_id, group_name);
	
	if(temp_group_id < 0 || user_id < 0) {
		/** User or group doesn't exist */
		return FALIURE;
	}
	
	if(add_usr_to_grp(temp_group_id, user_port) == LIMIT_REACHED) {
		return LIMIT_REACHED;
	}
	
	users[user_id].target_group = temp_group_id;
	users[user_id].target_user = NOT_SET;
	users[user_id].target_set = SEND_TO_GROUP;
	
	return SUCCESS;
}

int set_target_user(int user_port, char * user_name) {
	int temp_user_port = get_user_port_by_name(user_name);
	int user_id = get_user_id_by_port(user_port);
	
	printf("set_target_user %d, user_id %d\n", temp_user_port, user_id);
	
	if(temp_user_port <= 0 || user_id < 0) {
		/** User or group doesn't exist */
		return FALIURE;
	}
	
	users[user_id].target_group = NOT_SET;
	users[user_id].target_user = temp_user_port;
	users[user_id].target_set = SEND_TO_USER;
	
	return SUCCESS;
}

char * get_target_group_name(int user_port) {
	int user_id = get_user_id_by_port(user_port);
	if(user_id >= 0) {
		return groups[users[user_id].target_group].name;
	}
	return NULL;
}

int set_username(char * username, int user_port) {
	printf("received username %s, client port %d\n", username, user_port);
	int user_id = get_user_id_by_port(user_port);

	if(user_id < 0) { /** New client */
		user_id = get_free_userid();
		if(user_id == -1)
			return LIMIT_REACHED;
	}
	
	strcpy(users[user_id].name, username);
	users[user_id].port = user_port;
	return SUCCESS;
}

void print_mem_ports(int group_id) {
	for(int i=0; i<MAX_USERS_PER_GROUP; i++){
		printf("member %d port %d\n", i, groups[group_id].member_ports[i]);
	}
	return;
}

int add_usr_to_grp(int grp_id, int user_port) {
	
	printf("try to add user %d to group %d\n", user_port, grp_id);
	
	for(int i = 0; i < TOTAL_USERS; i++) {
		if(groups[grp_id].member_ports[i] == user_port) {
			printf("user already in group\n");
			return USER_EXISTS; /** user already in group */
		}
	}
	
	for(int i = 0; i < TOTAL_USERS; i++) {
		if(groups[grp_id].member_ports[i] == 0) {
			groups[grp_id].member_ports[i] = user_port;
			printf("user added at index %d\n", i);
			groups[grp_id].num_users++;
			if(groups[grp_id].num_users > MAX_USERS_PER_GROUP) {
				printf("Something wrong with num_users %d in group %d\n", groups[grp_id].num_users, grp_id);
			}
			return SUCCESS;
		}
	}
	return LIMIT_REACHED; /** User ports filled to max cap */
}

int del_usr_from_grp(int grp_id, int user_port) {
	print_mem_ports(grp_id);
	printf("try to del user %d from group %d\n", user_port, grp_id);
	
	for(int i = 0; i < TOTAL_USERS; i++) {
		if(groups[grp_id].member_ports[i] == user_port) {
			groups[grp_id].member_ports[i] = 0;
			print_mem_ports(grp_id);
			return SUCCESS;
		}
	}
	return FALIURE; /** uset not in the group */
}

int add_user_to_curr_grp(int user_port, char * usr_name_to_add) {
	int user_id_to_add = get_user_id_by_name(usr_name_to_add);
	if(user_id_to_add < 0)
		return FALIURE;	/** User name doesn't exist */
	
	return add_usr_to_grp( get_target_group_id(user_port), users[user_id_to_add].port);
}

int del_user_from_curr_grp(int user_port, char * usr_name_to_del) {
	int user_id_to_del = get_user_id_by_name(usr_name_to_del);
	if(user_id_to_del < 0)
		return FALIURE;	/** User name doesn't exist */
	
	return del_usr_from_grp( get_target_group_id(user_port), users[user_id_to_del].port);
}

int print_all_group(int user_port) {
	printf("Inside print all\n");
	int i = 0;
	char group_name[200];
	memset(group_name,0,200);
	while(i<MAX_NAME_LEN)
	{
		if(groups[i].num_users > 0)
		{	
			printf("Group names %s\n",groups[i].name);
			strcat(group_name,groups[i].name);
			strcat(group_name,"\n");
			printf("group name = %s\n",group_name);

		}
	i++;
	}
				
	if (send(user_port, group_name, sizeof(group_name), 0) == -1)
		perror("print all groups");
	
	return SUCCESS;
}



int process_cmd(int client_port, int nbytes_recvd, char * recv_buf) {
	int ret_val = 0;
	
	if(recv_buf[0] == 'n') {
		ret_val = set_username(&recv_buf[1], client_port);
	}
	else if(recv_buf[0] == 'g')
		ret_val = set_target_group(client_port, &recv_buf[1]);
	else if(recv_buf[0] == 'u')
		ret_val = set_target_user(client_port, &recv_buf[1]);
	else if(recv_buf[0] == 'c')
		ret_val = create_group(client_port, &recv_buf[1]);
	else if(recv_buf[0] == 'r')
		ret_val = delete_group(get_group_id_by_name(&recv_buf[1]));
	else if(recv_buf[0] == 'a')
		ret_val = add_user_to_curr_grp(client_port, &recv_buf[1]);
	else if(recv_buf[0] == 'h')
		ret_val = del_user_from_curr_grp(client_port, &recv_buf[1]);
	else if(recv_buf[0] == 'd')
		ret_val = add_usr_to_grp(DEFAULT_GRP_ID, client_port);
	else if(recv_buf[0] == 'p')
		ret_val = print_all_group(client_port);

	msg_to_user(client_port, ret_val);
	
	printf("command processed %s\n", recv_buf);
	return 0;
}

void send_to_all(int j, int i, int sockfd, int nbytes_recvd, char *recv_buf, fd_set *master)
{
	if (FD_ISSET(j, master)){
		if (j != sockfd && j != i) {
			if (send(j, recv_buf, nbytes_recvd, 0) == -1) {
				perror("send");
			}
		}
	}
}

void send_to_group(int usr_pos, int group_id, int nbytes_recvd, char *recv_buf)
{
	printf("send_to_group %d\n", group_id);
	for(int i=0; i < MAX_USERS_PER_GROUP; i++) {
		if (i != usr_pos && groups[group_id].member_ports[i] != 0) {
			printf("send to mem port %d\n", groups[group_id].member_ports[i]);
			if (send(groups[group_id].member_ports[i], recv_buf, nbytes_recvd, 0) == -1) {
				perror("send");
			}
		}
	}
}

void send_recv(int client_fd, fd_set *master, int sockfd, int fdmax)
{
	int nbytes_recvd, j, user_pos, user_id;
	char recv_buf[BUFSIZE], buf[BUFSIZE];
	
	if ((nbytes_recvd = recv(client_fd, recv_buf, BUFSIZE, 0)) <= 0) {
		if (nbytes_recvd == 0) {
			printf("Client with sockid %d closed connection\n", client_fd);
			delete_user(client_fd);
		}else {
			perror("recv");
		}
		close(client_fd);
		FD_CLR(client_fd, master);
	} else {
		printf("%s\n", recv_buf);
		
		if(recv_buf[0] == 'c') {
			printf("command received \n");
			process_cmd(client_fd, nbytes_recvd - 1, &recv_buf[1]);
		}
		else if(recv_buf[0] == 'm' || recv_buf[0] == 'f'){

			user_id = get_user_id_by_port(client_fd);
			if(user_id < 0)
				return;
			
			/* Don't transmit for default group or group not set*/
			if(users[user_id].target_set == SEND_TO_GROUP && users[user_id].target_group > 0) {
				user_pos = find_user_in_group(users[user_id].target_group, client_fd);
				if(user_pos == FALIURE) {
					return;
				}
				send_to_group(user_pos, users[user_id].target_group, nbytes_recvd, recv_buf);
			}
			else if(users[user_id].target_set == SEND_TO_USER && users[user_id].target_user > 0) {
				if (send(users[user_id].target_user, recv_buf, nbytes_recvd, 0) == -1) {
					perror("send");
				}
			}
			else {
				printf("Set target for user port %d\n", client_fd);
				return;
			}
		}
	}
	return;
}
		
int connection_accept(fd_set *master, int *fdmax, int sockfd, struct sockaddr_in *client_addr)
{
	socklen_t addrlen;
	int newsockfd;
	
	addrlen = sizeof(struct sockaddr_in);
	if((newsockfd = accept(sockfd, (struct sockaddr *)client_addr, &addrlen)) == -1) {
		perror("accept");
		exit(1);
	} else {
		FD_SET(newsockfd, master);
		if(newsockfd > *fdmax){
			*fdmax = newsockfd;
		}
		printf("new connection from %s on port %d with sockid %d\n",inet_ntoa(client_addr->sin_addr), ntohs(client_addr->sin_port), newsockfd);
	}
	return ntohs(client_addr->sin_port);
}
	
void connect_request(int *sockfd, struct sockaddr_in *my_addr)
{
	int yes = 1;
		
	if ((*sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		perror("Socket");
		exit(1);
	}
		
	my_addr->sin_family = AF_INET;
	my_addr->sin_port = htons(SERVERPORT);
	my_addr->sin_addr.s_addr = INADDR_ANY;
	memset(my_addr->sin_zero, '\0', sizeof my_addr->sin_zero);
		
	if (setsockopt(*sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
		perror("setsockopt");
		exit(1);
	}
		
	if (bind(*sockfd, (struct sockaddr *)my_addr, sizeof(struct sockaddr)) == -1) {
		perror("Unable to bind");
		exit(1);
	}
	if (listen(*sockfd, 10) == -1) {
		perror("listen");
		exit(1);
	}
	printf("\nTCPServer Waiting for client on port %d\n", SERVERPORT);
	fflush(stdout);
}

int main()
{
	fd_set master;
	fd_set read_fds;
	int fdmax, i;
	int sockfd= 0;
	struct sockaddr_in my_addr, client_addr;
	
	FD_ZERO(&master);
	FD_ZERO(&read_fds);
	connect_request(&sockfd, &my_addr);
	FD_SET(sockfd, &master);
	
	fdmax = sockfd;
	while(1){
		read_fds = master;
		if(select(fdmax+1, &read_fds, NULL, NULL, NULL) == -1){
			perror("select");
			exit(4);
		}
		
		for (i = 0; i <= fdmax; i++){
			if (FD_ISSET(i, &read_fds)){
				if (i == sockfd)
					connection_accept(&master, &fdmax, sockfd, &client_addr);
				else
					send_recv(i, &master, sockfd, fdmax);
			}
		}
	}
	return 0;
}