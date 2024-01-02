#include "USBNetwork.h"

struct pbuf* USBNetwork::received_frame = nullptr;

/* this is used by this code, ./class/net/net_driver.c, and usb_descriptors.c */
/* ideally speaking, this should be generated from the hardware's unique ID (if available) */
/* it is suggested that the first byte is 0x02 to indicate a link-local address */
const uint8_t tud_network_mac_address[6] = {0x02,0x02,0x84,0x6A,0x96,0x00};

USBNetwork::USBNetwork(
    const ip_addr_t &ipaddr,
    const ip_addr_t &netmask,
    const ip_addr_t &gateway,
    const std::vector<dhcp_entry_t>& dhcpEntries
) : ipaddr(ipaddr), netmask(netmask), gateway(gateway), dhcp_entries(dhcpEntries) {
    // If no DHCP entries are provided, use a default entry
    /* database IP addresses that can be offered to the host; this must be in RAM to store assigned MAC addresses */
    if (dhcp_entries.empty()) {
        dhcp_entry_t defaultEntry = { {0}, IPADDR4_INIT_BYTES(192, 168, 5, 3), 24 * 60 * 60 }; // 24-hour lease time
        dhcp_entries.push_back(defaultEntry);
    }

    // Initialize the DHCP server configuration
    dhcp_config =
    {
        /* router address (if any) */
        .router = gateway,
        /* listen port */
        .port = 67,
        /* dns server (if any) */
        .dns = IPADDR4_INIT_BYTES(0, 0, 0, 0),
        /* dns suffix */
        "usb",
        /* dhcp database */
        static_cast<int>(dhcp_entries.size()),
        dhcp_entries.data()
    };
}

void USBNetwork::init() {
    // Initialize tinyUSB
    tusb_init();

    // Initialize lwip
    lwip_init();

    // Initialize network interface
    initNetworkInterface();

    // wait for network to be up
    waitForNetworkUp();

    // Start DHCP server
    startDhcpServer();
}

void USBNetwork::waitForNetworkUp() {
    while (!netif_is_up(&netif_data));
}

void USBNetwork::startDhcpServer() {
    // Initialize and start DHCP server with the provided configuration
    while (dhserv_init(&dhcp_config) != ERR_OK);
}

void USBNetwork::initNetworkInterface() {
    // Initialize and add network interface
    struct netif *netif = &netif_data;
    netif->hwaddr_len = sizeof(tud_network_mac_address);
    memcpy(netif->hwaddr, tud_network_mac_address, sizeof(tud_network_mac_address));
    netif->hwaddr[5] ^= 0x01; // Toggle LSbit to ensure different MAC address from the host

    netif_add(netif, &ipaddr, &netmask, &gateway, NULL, netifInitCallback, ip_input);
    netif_set_default(netif);
    netif_set_up(netif);
}

void USBNetwork::serviceTraffic() {
    // handle any packet received by tud_network_recv_cb()
    if (received_frame) {
        ethernet_input(received_frame, &netif_data);
        pbuf_free(received_frame);
        received_frame = NULL;
        tud_network_recv_renew();
    }

    // Process lwIP timeouts
    sys_check_timeouts();
}

void USBNetwork::work() {
    // Handle USB tasks
    tud_task();

    // Process network traffic and handle timeouts
    serviceTraffic();
}

void USBNetwork::networkInitHandler() {
    // Initialization logic that was previously in tud_network_init_cb
    if (received_frame) {
        pbuf_free(received_frame);
        received_frame = nullptr;
    }
}

bool USBNetwork::networkReceiveHandler(const uint8_t *src, uint16_t size) {
    // Handle received network packet
    /* this shouldn't happen, but if we get another packet before 
    parsing the previous, we must signal our inability to accept it */
    if (received_frame) return false;
    
    if (size)
    {
        struct pbuf *p = pbuf_alloc(PBUF_RAW, size, PBUF_POOL);

        if (p)
        {
            /* pbuf_alloc() has already initialized struct; all we need to do is copy the data */
            memcpy(p->payload, src, size);
        
            /* store away the pointer for service_traffic() to later handle */
            received_frame = p;
        }
    }

    return true;
}

uint16_t USBNetwork::networkTransmitHandler(uint8_t *dst, void *ref, uint16_t arg) {
    // Handle network packet transmission
    struct pbuf *p = (struct pbuf *)ref;
    struct pbuf *q;
    uint16_t len = 0;

    (void)arg; /* unused for this example */

    /* traverse the "pbuf chain"; see ./lwip/src/core/pbuf.c for more info */
    for(q = p; q != NULL; q = q->next)
    {
        memcpy(dst, (char *)q->payload, q->len);
        dst += q->len;
        len += q->len;
        if (q->len == q->tot_len) break;
    }

    return len;
}

err_t USBNetwork::netifInitCallback(struct netif *netif) {
    LWIP_ASSERT("netif != NULL", (netif != NULL));
    netif->mtu = CFG_TUD_NET_MTU;
    netif->flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_LINK_UP | NETIF_FLAG_UP;
    netif->state = NULL;
    netif->name[0] = 'E';
    netif->name[1] = 'X';
    netif->linkoutput = linkoutput_fn;
    netif->output = output_fn;
    return ERR_OK;
}

// Implement linkoutput_fn and output_fn as in your original code
err_t USBNetwork::linkoutput_fn(struct netif *netif, struct pbuf *p) {
    (void)netif;
    
    for (;;)
    {
      /* if TinyUSB isn't ready, we must signal back to lwip that there is nothing we can do */
      if (!tud_ready())
        return ERR_USE;
    
      /* if the network driver can accept another packet, we make it happen */
      // HIPPY FIX
      // Provided a size
      if (tud_network_can_xmit(p->tot_len))
      {
        tud_network_xmit(p, 0 /* unused for this example */);
        return ERR_OK;
      }
    
      /* transfer execution to TinyUSB in the hopes that it will finish transmitting the prior packet */
      tud_task();
    }
}

err_t USBNetwork::output_fn(struct netif *netif, struct pbuf *p, const ip_addr_t *addr) {
    return etharp_output(netif, p, addr);
}

/* lwip platform specific routines for Pico */
static mutex_t lwip_mutex;
static int lwip_mutex_count = 0;

extern "C" {

void tud_network_init_cb(void) {
    // Initialization logic...
    USBNetwork::networkInitHandler();
}

bool tud_network_recv_cb(const uint8_t *src, uint16_t size) {
    // Receive logic...
    return USBNetwork::networkReceiveHandler(src, size);
}

uint16_t tud_network_xmit_cb(uint8_t *dst, void *ref, uint16_t arg) {
    // Transmit logic...
    return USBNetwork::networkTransmitHandler(dst, ref, arg);
}

sys_prot_t sys_arch_protect(void)
{
    uint32_t owner;
    if (!mutex_try_enter(&lwip_mutex, &owner))
    {
        if (owner != get_core_num())
        {
            // Wait until other core releases mutex
            mutex_enter_blocking(&lwip_mutex);
        }
    }

    lwip_mutex_count++;
    
    return 0;
}

void sys_arch_unprotect(sys_prot_t pval)
{
    (void)pval;
    
    if (lwip_mutex_count)
    {
        lwip_mutex_count--;
        if (!lwip_mutex_count)
        {
            mutex_exit(&lwip_mutex);
        }
    }
}

uint32_t sys_now(void)
{
    return to_ms_since_boot( get_absolute_time() );
}

} // extern "C"
