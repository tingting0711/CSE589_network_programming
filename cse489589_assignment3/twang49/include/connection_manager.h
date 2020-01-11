#ifndef _CONNECTION_MANAGER_H_
#define _CONNECTION_MANAGER_H_

extern uint16_t CONTROL_PORT;
extern int control_socket, router_socket, data_socket;
extern int head_fd;
extern fd_set master_list, watch_list;
extern struct timeval timeout;
// extern std::vector<int> data_sock_list;

void init(uint16_t control_port);
int update_time_list(struct timeval current);
struct timeval get_timeout();

struct Router find_router(uint32_t ip);
int send_dist_vector(int router_socket, uint32_t ip);

//new add
int ipToIndex(uint32_t ipaddr);
int IDtoIndex(uint16_t ID);
void update_dv_table();
void print_dv_table();
int update_dv(int router_socket);

//data
int new_data_conn(int data_socket);

#endif