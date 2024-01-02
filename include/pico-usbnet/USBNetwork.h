#ifndef PICONET_MANAGER_H
#define PICONET_MANAGER_H
#include <vector>

extern "C"
{
// HIPPY FIX
#include "dhserver.h"
#include "dnserver.h"
#include "lwip/apps/httpd.h"
#include "lwip/init.h"
#include "lwip/tcp.h"
#include "lwip/timeouts.h"
#include "pico/stdlib.h"
#include "pico/sync.h"
#include "tusb.h"
}

class USBNetwork
{
public:
    USBNetwork(
        const ip_addr_t &ipaddr,
        const ip_addr_t &netmask,
        const ip_addr_t &gateway,
        const std::vector<dhcp_entry_t> &dhcpEntries = std::vector<dhcp_entry_t>());
    void init();
    void waitForNetworkUp();
    void startDhcpServer();
    void work();

    // TinyUSB network callback handlers
    static void networkInitHandler();
    static bool networkReceiveHandler(const uint8_t *src, uint16_t size);
    static uint16_t networkTransmitHandler(uint8_t *dst, void *ref, uint16_t size);

private:
    void serviceTraffic();

    std::vector<dhcp_entry_t> dhcp_entries;
    dhcp_config_t dhcp_config;

    struct netif netif_data;
    // Network configuration
    ip_addr_t ipaddr;
    ip_addr_t netmask;
    ip_addr_t gateway;

    void initNetworkInterface();

    static struct pbuf *received_frame;
    // Link output function for lwIP
    static err_t linkoutput_fn(struct netif *netif, struct pbuf *p);
    // Standard output function for lwIP
    static err_t output_fn(struct netif *netif, struct pbuf *p, const ip_addr_t *addr);
    static err_t netifInitCallback(struct netif *netif);
};

#endif // PICONET_MANAGER_H