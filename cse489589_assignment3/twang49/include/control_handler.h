#ifndef _CONTROL_HANDLER_H_
#define _CONTROL_HANDLER_H_

/* for init response */
extern uint16_t number_of_routers;
extern uint16_t updates_periodic_interval; 
extern uint16_t my_router_id; 
extern uint16_t this_router_i;                       
extern uint16_t my_data_port;                      
extern uint16_t my_router_port;                    
extern uint32_t my_router_ip; 
// extern struct timeval timeout;
extern uint16_t **distance_vector; 

extern std::vector<Router>router_list;
extern std::vector<Time>time_list;

extern int control_socket, router_socket, data_socket;



int init_time_list();

std::string getIPfromUint16_t(uint16_t ip);

int create_control_sock(uint16_t control_port);
int new_control_conn(int sock_index);
bool isControl(int sock_index);
bool control_recv_hook(int sock_index);

/* for author_response */
void author_response(int sock_index);

/* for init response */
int create_router_sock(uint16_t router_port);
int create_data_sock(uint16_t data_port);
int init_timeout();
int init_distance_vector();
int init_router_list(char *cntrl_payload, int offset);
int init_response(int sock_index, char *cntrl_payload, uint16_t payload_len);
int init_send_dv(int router_socket);
// int new_route_conn(uint16_t dest_id, uint32_t dest_ip, uint16_t dest_router_port);

/* for routing_table_response */
int routing_table_response(int sock_index, char *cntrl_payload, uint16_t payload_len);

/* for update_response */
int update_response(int sock_index, char *cntrl_payload, uint16_t payload_len);

/* for crash_response */
int crash_response(int sock_index, char *cntrl_payload, uint16_t payload_len);


bool isData(int sock_index);

/* for sendfile_response */
// int sendfile_response(int sock_index, char *cntrl_payload, uint16_t payload_len);

/* for sendfile_stats_respnse */
// int sendfile_stats_respnse(int sock_index, char *cntrl_payload, uint16_t payload_len);

/* for last_data_packet_response */
// int last_data_packet_response(int sock_index, char *cntrl_payload, uint16_t payload_len);

/* for penultimate_data_packet_response */
// int penultimate_data_packet_response(int sock_index, char *cntrl_payload, uint16_t payload_len);


#endif