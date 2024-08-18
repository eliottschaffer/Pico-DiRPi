#ifndef NET_UTIL_H
#define NET_UTIL_H


int setup(uint32_t country, const char *ssid, 
   const char *pass, uint32_t auth, 
     const char *hostname, ip_addr_t *ip,
                   ip_addr_t *mask, ip_addr_t *gw);

static void ntp_result(NTP_T* state, int status, time_t *result);

static void ntp_request(NTP_T *state);

static int64_t ntp_failed_handler(alarm_id_t id, void *user_data);

static void ntp_dns_found(const char *hostname, const ip_addr_t *ipaddr, void *arg);

static void ntp_recv(void *arg, struct udp_pcb *pcb, struct pbuf *p, const ip_addr_t *addr, u16_t port);

static NTP_T* ntp_init(void);

void ntp_setup(void);

void set_epoch_time(const datetime_t *t);


#endif