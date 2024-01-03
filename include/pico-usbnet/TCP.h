#ifndef PICONET_TCP_H
#define PICONET_TCP_H

#include "lwip/tcp.h"

class TCP {
public:
    TCP();
    ~TCP();

    void init();
    void keepAlive(bool enable, uint32_t interval);
    void bind(const ip_addr_t *ipaddr, uint16_t port);
    void listen();
    void close();
    err_t send(const void *data, uint16_t len);
    err_t send();
    err_t write(const void *data, uint16_t len);
    uint16_t getAvailableSize();

    // Callback setters
    void onAccept(void (*callback)(struct tcp_pcb *newpcb, err_t err));
    void onReceive(void (*callback)(struct pbuf *p));
    void onSent(void (*callback)(err_t err));
    void onError(void (*callback)(err_t err));
    void onClose(void (*callback)());

private:
    struct tcp_pcb* pcb;
    struct tcp_pcb* client;

    // Callbacks
    void (*receiveCallback)(struct pbuf *p);
    void (*errorCallback)(err_t err);
    void (*closeCallback)();
    void (*acceptCallback)(struct tcp_pcb *newpcb, err_t err);
    void (*sentCallback)(err_t err);

    // Static callback wrappers
    static err_t receiveWrapper(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err);
    static void errorWrapper(void *arg, err_t err);
    static err_t acceptWrapper(void *arg, struct tcp_pcb *newpcb, err_t err);
    static void closeWrapper(void *arg);
    static err_t sentWrapper(void *arg, struct tcp_pcb *tpcb, u16_t len);
};

#endif // PICONET_TCP_H
