/**
 * @author ejrbuss
 * @date 2017
 */
#ifndef RDP_receiveR_HEADER
#define RDP_receiveR_HEADER

extern void rdp_receiver(const char* receiver_ip, const char* receiver_port);
extern void rdp_receiver_receive();
extern void rdp_receiver_stats();

#endif