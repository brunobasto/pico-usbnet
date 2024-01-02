#include "TCP.h"

TCP::TCP() {
    
}

TCP::~TCP() {
    if (pcb) {
        this->close();
    }
}

void TCP::init() {
    // Initialize the TCP control block
    pcb = tcp_new();

    if (!pcb) {
        errorWrapper(this, ERR_MEM);
    }

    tcp_arg(pcb, this);
}

void TCP::keepAlive(bool enable, uint32_t interval) {
    if (enable) {
        pcb->so_options |= SOF_KEEPALIVE;
        pcb->keep_intvl = interval;
    } else {
        pcb->so_options &= ~SOF_KEEPALIVE;
    }
}

void TCP::bind(const ip_addr_t *ipaddr, uint16_t port) {
    tcp_bind(pcb, ipaddr, port);
}

void TCP::listen() {
    struct tcp_pcb* listen = tcp_listen(pcb);
    pcb = listen;
    tcp_accept(listen, acceptWrapper);
}

void TCP::close() {
    closeWrapper(this);
}

err_t TCP::send(const void *data, uint16_t len) {
    err_t result = this->write(data, len);    

    if (result != ERR_OK) {
        return result;
    }

    // Flush the data sent (actually send the TCP segment)
    return tcp_output(pcb);

    return result; // Returns ERR_OK if everything went fine
}

err_t TCP::send() {
    // Flush the data sent (actually send the TCP segment)
    return tcp_output(pcb);
}

err_t TCP::write(const void *data, uint16_t len) {
    if (!pcb) {
        return ERR_CONN; // No valid connection
    }

    // Try to write the data to the TCP send buffer
    err_t result = tcp_write(pcb, data, len, TCP_WRITE_FLAG_COPY);

    if (result != ERR_OK) {
        // Handle error or return the error code
        return result;
    }

    return ERR_OK;
}

void TCP::onReceive(void (*callback)(struct pbuf *p)) {
    receiveCallback = callback;
}

void TCP::onError(void (*callback)(err_t err)) {
    errorCallback = callback;
}

void TCP::onClose(void (*callback)()) {
    closeCallback = callback;
}

void TCP::onAccept(void (*callback)(struct tcp_pcb *newpcb, err_t err)) {
    acceptCallback = callback;
}

// Implement the static callback wrappers
err_t TCP::receiveWrapper(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err) {
    // Assuming arg is an instance of TCP
    TCP *instance = static_cast<TCP*>(arg);

    if (err != ERR_OK && p != NULL) {
        if (instance && instance->errorCallback) {
            instance->errorCallback(err);
        }

        return err;
    }

    tcp_recved(tpcb, p->tot_len);
    tcp_sent(tpcb, NULL);

    // The connection is closed if the client sends "X".
    if (((char*)p->payload)[0] == 'X') {
        instance->close();
    }

    if (instance && instance->receiveCallback) {
        instance->receiveCallback(p);
    }

    return ERR_OK;
}

void TCP::errorWrapper(void *arg, err_t err) {
    TCP *instance = static_cast<TCP*>(arg);

    if (instance && instance->errorCallback) {
        instance->errorCallback(err);
    }

    instance->close();
}

err_t TCP::acceptWrapper(void *arg, struct tcp_pcb *newpcb, err_t err) {
    TCP *instance = static_cast<TCP*>(arg);

    if (err != ERR_OK) {
        if (instance && instance->errorCallback) {
            instance->errorCallback(err);
        }

        return err;
    }

    tcp_setprio(newpcb, TCP_PRIO_MAX);
    tcp_recv(newpcb, receiveWrapper);
    tcp_err(newpcb, errorWrapper);
    tcp_poll(newpcb, NULL, 4);

    instance->pcb = newpcb;
    
    if (instance && instance->acceptCallback) {
        instance->acceptCallback(newpcb, err);
    }

    return err;
}

void TCP::closeWrapper(void *arg) {
    TCP *instance = static_cast<TCP*>(arg);

    tcp_close(instance->pcb);

    if (instance && instance->closeCallback) {
        instance->closeCallback();
    }
}