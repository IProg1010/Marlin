#include "lwip/opt.h"
#include "lwip/mem.h"
#include "lwip/memp.h"
#include "lwip/timeouts.h"
#include "netif/ethernet.h"
#include "netif/etharp.h"
#include "lwip/ethip6.h"
#include "ethernetif.h"
#include <string.h>
//#include "lwip_task.h"
#include "dev_eth_function.h"
#include "debug.h"

/* Network interface name */
#define IFNAME0 'c'
#define IFNAME1 'h'

struct ethernetif {
    struct eth_addr *ethaddr;
    /* Add whatever per-interface state that is needed here. */
};

static void low_level_init(struct netif *netif) 
{
    //printf("low_level_init: low_level_init\r\n");
    //uint8_t mac[6] = {0x71,0x8B,0xCD,0x9E,0x07,0x0F};
    uint8_t* mac = (uint8_t *)(0x1FFFF7E8+5);
    printf("mac:%d.%d.%d.%d.%d.%d.\r\n", *mac, *(mac-1), *(mac-2), *(mac-3), *(mac-4), *(mac-5));

#if LWIP_ARP || LWIP_ETHERNET

    /* set MAC hardware address length */
    netif->hwaddr_len = ETH_HWADDR_LEN;

    /* set MAC hardware address */
    netif->hwaddr[0] =  *mac;
    mac--;
    netif->hwaddr[1] =  *mac;
    mac--;
    netif->hwaddr[2] =  *mac;
    mac--;
    netif->hwaddr[3] =  *mac;
    mac--;
    netif->hwaddr[4] =  *mac;
    mac--;
    netif->hwaddr[5] =  *mac;


    /*netif->hwaddr[0] =  mac[0];
    netif->hwaddr[1] =  mac[1];
    netif->hwaddr[2] =  mac[2];
    netif->hwaddr[3] =  mac[3];
    netif->hwaddr[4] =  mac[4];
    netif->hwaddr[5] =  mac[5];*/

    /* maximum transfer unit */
    netif->mtu = NETIF_MTU;

#if LWIP_ARP
    netif->flags |= NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP;
#else
    netif->flags |= NETIF_FLAG_BROADCAST;
#endif /* LWIP_ARP */

#endif /* LWIP_ARP || LWIP_ETHERNET */
    netif->flags |= NETIF_FLAG_LINK_UP | NETIF_FLAG_UP;
}

static err_t low_level_output(struct netif *netif, struct pbuf *p) 
{
           //printf("low_level_output: low_level_output\r\n");
    struct pbuf *q;
    uint16_t len = 0;
    //uint32_t *buffer = (uint32_t *)ETH_GetCurrentTxBufferAddress();

#if ETH_PAD_SIZE
    pbuf_remove_header(p, ETH_PAD_SIZE); /* drop the padding word */
#endif

    for (q = p; q != NULL; q = q->next) 
    {
        /* Send the data from the pbuf to the interface, one pbuf at a
           time. The size of the data in each pbuf is kept in the ->len
           variable. */
        //memcpy(buffer + len, q->payload, q->len);
        //len = len + q->len;
    //}

        if (!ETH_TxPktChainMode(q->len, q->payload))
        //if (!ETH_TxPkt_ChainMode(len))
        {
            printf("Send failed.\r\n");
            return ERR_ALREADY;
        }
        else
        {
            printf("Send ok.\r\n");
            net_data_led_require = 1;
        }
    }
    /* increase ifoutdiscards or ifouterrors on error */

#if ETH_PAD_SIZE
    pbuf_add_header(p, ETH_PAD_SIZE); /* reclaim the padding word */
#endif

    return ERR_OK;
}

//extern uint16_t getPointer();
extern uint16_t Rx_pointer;
uint16_t Rx_current_pointer = 0;
extern uint8_t RxToLWIP[ETH_RXBUFNB*ETH_RX_BUF_SZE]; //receive buffer

static struct pbuf *low_level_input(struct netif *netif) 
{
    //printf("*low_level_input.\r\n");
    struct pbuf *p, *q;
    u32_t len;
    u16_t read_index, once_size;
    FrameTypeDef *rec_data;
    /* Obtain the size of the packet and put it into the "len"
       variable. */
    //NVIC_DisableIRQ(ETH_IRQn);
    //ETH_RxPkt_ChainMode();
    //rec_data = list_pop(ch307_mac_rec);
    //NVIC_EnableIRQ(ETH_IRQn);
    uint16_t curr = getPointer();
    //printf("*curr = %d\r\n", curr);
    //check if curr < Rx_current_pointer
    uint16_t len_arr;
    if (curr > Rx_current_pointer) 
    {
        len_arr = curr - Rx_current_pointer;
    } 
    else
    {
        len_arr = Rx_current_pointer - curr;
    }
    if (len_arr == 0) 
    {
        return NULL;
    }
    else if (len_arr != 0) 
    {
        net_data_led_require = 1; /* require for led. */

        len = len_arr;

#if ETH_PAD_SIZE
        len += ETH_PAD_SIZE; /* allow room for Ethernet padding */
#endif

        /* We allocate a pbuf chain of pbufs from the pool. */
        p = pbuf_alloc(PBUF_RAW, len, PBUF_POOL);

        if (p != NULL) 
        {

#if ETH_PAD_SIZE
            pbuf_remove_header(p, ETH_PAD_SIZE); /* drop the padding word */
#endif
            read_index  = 0;
            /* We iterate over the pbuf chain until we have read the entire
             * packet into the pbuf. */
            for (q = p; q != NULL; q = q->next) 
            {
                /* Read enough bytes to fill this pbuf in the chain. The
                 * available data in the pbuf is given by the q->len
                 * variable.
                 * This does not necessarily have to be a memcpy, you can also preallocate
                 * pbufs for a DMA-enabled MAC and after receiving truncate it to the
                 * actually received size. In this case, ensure the tot_len member of the
                 * pbuf is the sum of the chained pbuf len members.
                 */
                if (len >= q->len) 
                {
                    once_size = q->len;
                } 
                else 
                {
                    once_size = len;
                }
                len = len - once_size;
                
                //memcpy(q->payload, (void *)(rec_data->buffer + read_index), once_size);
                /*if(Rx_current_pointer + read_index >= ETH_RXBUFNB*ETH_RX_BUF_SZE)
                {
                    read_index = ETH_RXBUFNB*ETH_RX_BUF_SZE-Rx_current_pointer;
                } 
                else
                {

                }*/
                memcpy(q->payload, (void *)(&RxToLWIP[Rx_current_pointer] + read_index), once_size);
                
                read_index = read_index + once_size;
               // printf("low_level_input: curr %4d\r\n", curr);
            }
            Rx_current_pointer = curr;
            //acknowledge that packet has been read();
        } 
        else 
        {
            //drop packet
        }
    } 
    else 
    {
        p = NULL;
        return p;
    }

    //NVIC_DisableIRQ(ETH_IRQn);
    //DMARxDescToGet->Status |= ETH_DMARxDesc_OWN;
    //rec_data->descriptor->Status |= ETH_DMARxDesc_OWN;
    //memb_free(&ch307_mac_rec_frame_mem, rec_data);
    //NVIC_EnableIRQ(ETH_IRQn);

    return p;
}

void ethernetif_input(struct netif *netif) {
    //printf("ethernetif_input: ethernetif_input\r\n");
    err_t err;
    struct pbuf *p;

    while(1) {
        /* move received packet into a new pbuf */
        p = low_level_input(netif);

        /* no packet could be read, silently ignore this */
        if (p == NULL) return;

        /* entry point to the LwIP stack */
        err = netif->input(p, netif);

        if (err != ERR_OK) {
            printf("ethernetif_input: IP input error\r\n");
            //LWIP_DEBUGF(NETIF_DEBUG, ("ethernetif_input: IP input error\r\n"));
            pbuf_free(p);
            p = NULL;
            return;
        }
    }
}

#if !LWIP_ARP
static err_t low_level_output_arp_off(struct netif *netif, struct pbuf *q, const ip4_addr_t *ipaddr) {
    err_t errval;
    errval = ERR_OK;

    return errval;

}
#endif /* LWIP_ARP */

#if LWIP_IPV6_MLD
static void ip6_to_multicast(const ip6_addr_t *group, uint8_t *mac_address) {
    // multicast mac address is 33:33:[lowest 32 bits of v6 address]
    mac_address[0] = 0x33;
    mac_address[1] = 0x33;
    memcpy(mac_address + 2, group->addr + 3, 4);
}

static uint32_t i_to_eth_mac_address(uint8_t i) {
    switch(i) {
        case 0:
            return ETH_MAC_Address0;
        case 1:
            return ETH_MAC_Address1;
        case 2:
            return ETH_MAC_Address2;
        case 3:
            return ETH_MAC_Address3;
        default:
            return 0;
    }
}

static uint8_t used_filters = 0;

err_t ethernet_mld_mac_filter(struct netif *netif, const ip6_addr_t *group, enum netif_mac_filter_action action)
{
    uint8_t address[6];
    ip6_to_multicast(group, address);

    if (action == NETIF_DEL_MAC_FILTER) {
        uint8_t config_mac[6];
        for(uint8_t i = 0; i < 4; i++) {
            if(!(used_filters & (1 << i))) {
                // skip any unused filter
                continue;
            }
            uint32_t eth_mac = i_to_eth_mac_address(i);
            ETH_GetMACAddress(eth_mac, config_mac);
            if(memcmp(address, config_mac, sizeof(address)) == 0) {
                ETH_MACAddressPerfectFilterCmd(eth_mac, DISABLE);
                used_filters &= ~(1 << i);
                /*
                 * char group_str[IP6ADDR_STRLEN_MAX];
                 * ip6addr_ntoa_r(group, group_str, sizeof(group_str));
                 * printf("DEBUG: removed filter %s from %d\r\n", group_str, i);
                 */
                return ERR_OK;
            }
        }
        return ERR_OK;
    }

    for(uint8_t i = 0; i < 4; i++) {
        if(used_filters & (1 << i)) {
            // skip any used filter
            continue;
        }
        uint32_t eth_mac = i_to_eth_mac_address(i);
        ETH_MACAddressConfig(eth_mac, address);
        ETH_MACAddressPerfectFilterCmd(eth_mac, ENABLE);
        used_filters |= 1 << i;
        /*
         * char group_str[IP6ADDR_STRLEN_MAX];
         * ip6addr_ntoa_r(group, group_str, sizeof(group_str));
         * printf("DEBUG: added filter %s to %d\r\n", group_str, i);
         */
        return ERR_OK;
    }

    // ran out of address filters
    printf("DEBUG: ran out of MLD filters\r\n");
    return ERR_IF;
}
#endif

err_t ethernetif_init(struct netif *netif) {
            //LWIP_DEBUGF(NETIF_DEBUG, ("ethernetif_init: ethernetif_init\r\n"));
    printf("ethernetif_init: ethernetif_init\r\n");
    struct ethernetif *ethernetif;

    LWIP_ASSERT("netif != NULL", (netif != NULL));

    ethernetif = mem_malloc(sizeof(struct ethernetif));
    if (ethernetif == NULL) 
    {
        printf("ethernetif_init: out of memory\r\n");
        //LWIP_DEBUGF(NETIF_DEBUG, ("ethernetif_init: out of memory\r\n"));
        return ERR_MEM;
    }

#if LWIP_NETIF_HOSTNAME
    /* Initialize interface hostname */
    netif->hostname = "ch32v307";
#endif /* LWIP_NETIF_HOSTNAME */

    /*
     * Initialize the snmp variables and counters inside the struct netif.
     * The last argument should be replaced with your link speed, in units
     * of bits per second.
     */

    netif->state = ethernetif;
    netif->name[0] = IFNAME0;
    netif->name[1] = IFNAME1;
    /* We directly use etharp_output() here to save a function call.
     * You can instead declare your own function an call etharp_output()
     * from it if you have to do some checks before sending (e.g. if link
     * is available...) */
#if LWIP_IPV4
    netif->output = etharp_output;
#endif /* LWIP_IPV4 */

#if LWIP_IPV6
    netif->output_ip6 = ethip6_output;
#endif /* LWIP_IPV6 */

#if LWIP_IPV6_MLD
    netif->flags |= NETIF_FLAG_MLD6;
    netif->mld_mac_filter = ethernet_mld_mac_filter;
#endif

    netif->linkoutput = low_level_output;

    ethernetif->ethaddr = (struct eth_addr *) & (netif->hwaddr[0]);

    /* initialize the hardware */
    low_level_init(netif);

#if LWIP_IPV6_MLD
    // we need to listen to the all nodes multicast address for route advertisements
    ip6_addr_t ip6_allnodes_ll;
    ip6_addr_set_allnodes_linklocal(&ip6_allnodes_ll);
    netif->mld_mac_filter(netif, &ip6_allnodes_ll, NETIF_ADD_MAC_FILTER);
#endif

    return ERR_OK;
}