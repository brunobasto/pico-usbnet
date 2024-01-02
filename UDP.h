#ifndef PICONET_UDP_H
#define PICONET_UDP_H

#include <cstring>

extern "C" {
    #include "lwip/udp.h"
    #include "lwip/ip_addr.h"
}

class UDP {
public:
    UDP();
    ~UDP();

    void init();
    void bind(const ip_addr_t *ipaddr, uint16_t port);
    void send(const void *data, uint16_t len);
    void close();

    void onReceive(void (*callback)(struct pbuf *p, const ip_addr_t *addr, uint16_t port));

private:
    struct udp_pcb *pcb;
    void (*receiveCallback)(struct pbuf *p, const ip_addr_t *addr, uint16_t port);

    static void receiveWrapper(void *arg, struct udp_pcb *upcb, struct pbuf *p,
                               const ip_addr_t *addr, uint16_t port);
};

#endif // PICONET_UDP_H
