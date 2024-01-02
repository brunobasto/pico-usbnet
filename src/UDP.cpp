#include "pico-usbnet/UDP.h"

UDP::UDP() : pcb(nullptr), receiveCallback(nullptr) {}

UDP::~UDP() {
    close();
}

void UDP::init() {
    pcb = udp_new();
    if (!pcb) {
        // Handle error, e.g., throw exception or set error status
    }
}

void UDP::bind(const ip_addr_t *ipaddr, uint16_t port) {
    if (pcb) {
        udp_bind(pcb, ipaddr, port);
    }
}

void UDP::send(const void *data, uint16_t len) {
    if (pcb) {
        struct pbuf *p = pbuf_alloc(PBUF_TRANSPORT, len, PBUF_RAM);

        if (!p) {
            // Handle error, e.g., throw exception or set error status
            return;
        }

        memcpy(p->payload, data, len);
        udp_send(pcb, p);
        pbuf_free(p);
    }
}

void UDP::close() {
    if (pcb) {
        udp_remove(pcb);
        pcb = nullptr;
    }
}

void UDP::onReceive(void (*callback)(struct pbuf *p, const ip_addr_t *addr, uint16_t port)) {
    receiveCallback = callback;

    if (pcb) {
        udp_recv(pcb, receiveWrapper, this);
    }
}

// Static callback wrapper
void UDP::receiveWrapper(void *arg, struct udp_pcb *upcb, struct pbuf *p,
                         const ip_addr_t *addr, uint16_t port) {
    UDP *instance = static_cast<UDP*>(arg);
    if (instance && instance->receiveCallback && p != NULL) {
        instance->receiveCallback(p, addr, port);
    }
    if (p) {
        pbuf_free(p); // Free the pbuf
    }
}
