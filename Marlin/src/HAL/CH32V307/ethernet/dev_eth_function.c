#include "dev_eth_function.h"
#include "lwipopts.h"
//#include "tcp_server.h"
//#include <delay.h>

#undef LWIP_DHCP

#define  LED_ON                         0
#define  LED_OFF                        1

/* PHY state @PHY_STAT */
#define PHY_LINK_SUCCESS                (1 << 2)          //PHY connection success
#define PHY_AUTO_SUCCESS                (1 << 5)          //PHY auto negotiation completed

#define ROM_CFG_USERADR_ID                      0x1FFFF7E8
uint16_t gPHYAddress;
uint32_t volatile LocalTime;
uint32_t ChipId = 0;

ETH_DMADESCTypeDef *pDMARxSet;
ETH_DMADESCTypeDef *pDMATxSet;

/* PHY negotiation function */
uint8_t phyLinkStatus = 0;
uint8_t phyStatus = 0;
uint8_t phyLinkCnt = 0;
uint8_t phySucCnt = 0;
uint8_t phyPN = PHY_PN_SWITCH_AUTO;
uint8_t TRDetectStep = 0;
uint8_t TRDetectCnt = 0;
uint8_t LinkTaskPeriod = 50;
uint32_t RandVal = 0;
uint8_t volatile phyLinkReset;
uint32_t volatile phyLinkTime;

/* PHY receive processing */
uint8_t ReInitMACFlag = 0;
uint8_t DuplexMode = 1;
uint8_t PhyPolarityDetect = 0;
uint32_t LinkSuccTime = 0;
//extern u8 MACAddr[6];

netconfig* netconfig_ptr;
static uint8_t set_cfg = 0;

LIST(ch307_mac_rec);
MEMB(ch307_mac_rec_frame_mem, FrameTypeDef, ETH_RXBUFNB);


__attribute__ ((aligned(4))) ETH_DMADESCTypeDef DMARxDscrTab[ETH_RXBUFNB];/* Receive ethernet packet descriptor table  */
__attribute__ ((aligned(4))) ETH_DMADESCTypeDef DMATxDscrTab[ETH_TXBUFNB];/* Transmit ethernet packet descriptor table */
//__attribute__ ((aligned(4))) uint8_t Rx_Buff[ETH_RXBUFNB*ETH_MAX_PACKET_SIZE];/* Receive ethernet packet data  */
//__attribute__ ((aligned(4))) uint8_t Tx_Buff[ETH_TXBUFNB*ETH_MAX_PACKET_SIZE];/* Transmit ethernet packet data  */
uint8_t Rx_Buff[ETH_RXBUFNB*ETH_MAX_PACKET_SIZE];/* Receive ethernet packet data  */
uint8_t Tx_Buff[ETH_TXBUFNB*ETH_MAX_PACKET_SIZE];/* Transmit ethernet packet data  */

uint8_t RxToLWIP[ETH_RXBUFNB*ETH_RX_BUF_SZE];/* Receive ethernet packet to Lwip core  */

#define  ETH_DMARxDesc_FrameLengthShift           16

#define define_O(a,b) \
GPIO_InitStructure.GPIO_Pin = b;\
GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;\
GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;\
GPIO_Init(a, &GPIO_InitStructure)

#define define_I(a,b) \
GPIO_InitStructure.GPIO_Pin = b;\
GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;\
GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;\
GPIO_Init(a, &GPIO_InitStructure)


struct netif WCH_NetIf;
ip_addr_t ipaddr = IPADDR4_INIT(0);
ip_addr_t netmask = IPADDR4_INIT(0);
ip_addr_t gw = IPADDR4_INIT(0);
uint8_t IP_ADDRESS[4];
uint8_t NETMASK_ADDRESS[4];
uint8_t GATEWAY_ADDRESS[4];
uint8_t MACAddr[6];


void lwip_init_success_callback(ip_addr_t *ip)
{
    printf("Server IP:%ld.%ld.%ld.%ld\r\n",  \
        ((ip->addr)&0x000000ff),       \
        (((ip->addr)&0x0000ff00)>>8),  \
        (((ip->addr)&0x00ff0000)>>16), \
        ((ip->addr)&0xff000000)>>24);
    //TCP_Init();
    //lwiperf_start_tcp_server(ip, 9527, NULL, NULL);
}

static void wait_dhcp(void *arg) 
{
    if(ip_addr_cmp(&(WCH_NetIf.ip_addr),&ipaddr))   // Wait for DHCP config /
    {
        sys_timeout(50, wait_dhcp, NULL);
    }
    else
    {
        //lwip_init_success_callback(&(WCH_NetIf.ip_addr)); /* notify callback about dhcp up */
    }
}
/*********************************************************************
 * @fn      Ethernet_LED_LINKSET
 *
 * @brief   set eth link led,setbit 0 or 1,the link led turn on or turn off
 *
 * @return  none
 */
void Ethernet_LED_LINKSET(uint8_t setbit)
{
     if(setbit){
         GPIO_SetBits(GPIOC, GPIO_Pin_1);
     }
     else {
         GPIO_ResetBits(GPIOC, GPIO_Pin_1);
    }
}


/*********************************************************************
 * @fn      Ethernet_LED_DATASET
 *
 * @brief   set eth data led,setbit 0 or 1,the data led turn on or turn off
 *
 * @return  none
 */
void Ethernet_LED_DATASET(uint8_t setbit)
{
     if(setbit){
         GPIO_SetBits(GPIOC, GPIO_Pin_0);
     }
     else {
         GPIO_ResetBits(GPIOC, GPIO_Pin_0);
    }
}


void set_net_config(netconfig* net_cfg)
{
    netconfig_ptr = net_cfg;
    set_cfg = 1;
}

void lwip_initialize(void) 
{
    printf("lwip init start\r\n");
    /* mem_init of lwip, init outside the lwip_init for user to using outside. */
    mem_init();
    memp_init();

    WCHNET_GetMacAddr(MACAddr);
    ETH_Init(MACAddr);

    netconfig_ptr->mac[0] = MACAddr[0];
    netconfig_ptr->mac[1] = MACAddr[1];
    netconfig_ptr->mac[2] = MACAddr[2];
    netconfig_ptr->mac[3] = MACAddr[3];
    netconfig_ptr->mac[4] = MACAddr[4];
    netconfig_ptr->mac[5] = MACAddr[5];

    printf("CH307_INIT_PHY ok\r\n");

    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_RNG, ENABLE);
    RNG_Cmd(ENABLE);

    printf("enable rng ok\r\n");

#if LWIP_DHCP
    ip_addr_set_zero_ip4(&(WCH_NetIf.ip_addr));
#else
    /* IP addresses initialization */
    if(set_cfg == 1)
    {
        IP4_ADDR(&ipaddr, netconfig_ptr->ip[0], netconfig_ptr->ip[1], netconfig_ptr->ip[2],  netconfig_ptr->ip[3]);
        IP4_ADDR(&netmask, netconfig_ptr->mask[0], netconfig_ptr->mask[1], netconfig_ptr->mask[2],  netconfig_ptr->mask[3]);
        IP4_ADDR(&gw, netconfig_ptr->gw[0], netconfig_ptr->gw[1], netconfig_ptr->gw[2],  netconfig_ptr->gw[3]);
    }
    else
    {
        set_cfg = 0;
        IP4_ADDR(&ipaddr, 192, 168, 1, 86);
        IP4_ADDR(&netmask, 255, 255, 254, 0);
        IP4_ADDR(&gw, 192, 168, 1, 1);
    }
#endif
    /* Initilialize the LwIP stack*/
    lwip_init();
    printf("lwip_init() ok\r\r\n");

    /* add the network interface (IPv4/IPv6)*/
    netif_add(&WCH_NetIf, ip_2_ip4(&ipaddr), ip_2_ip4(&netmask), ip_2_ip4(&gw), NULL, &ethernetif_init, &ethernet_input);

    /* Registers the default network interface */
    netif_set_default(&WCH_NetIf);

    if (netif_is_link_up(&WCH_NetIf))
    {
        /* When the netif is fully configured this function must be called */
        netif_set_up(&WCH_NetIf);
        
    #if LWIP_DHCP
        int err;
        /*  Creates a new DHCP client for this interface on the first call.
        Note: you must call dhcp_fine_tmr() and dhcp_coarse_tmr() at
        the predefined regular intervals after starting the client.
        You can peek in the netif->dhcp struct for the actual DHCP status.*/

        printf("dhcp_start()\r\r\n");
        err = dhcp_start(&WCH_NetIf);      // start dhcp /
        if(err == ERR_OK)
        {
        printf("1()\r\r\n");
            printf("lwip dhcp start success...\r\n\r\n");
        }
        else
        {
        printf("2()\r\r\n");
            printf("lwip dhcp start fail...\r\n\r\n");
        }
        // poll for dhcp status
        sys_timeout(50, wait_dhcp, NULL);

    #else
        printf("netif_set_up()\r\n");

    #endif

    #if LWIP_IPV6
        netif_create_ip6_linklocal_address(&WCH_NetIf, 1);
        netif_set_ip6_autoconfig_enabled(&WCH_NetIf, 1);
    #endif

    }
    else
    {
        /* When the netif link is down this function must be called */
        netif_set_down(&WCH_NetIf);
    }

    lwip_init_success_callback( & (WCH_NetIf.ip_addr) ); /* notify callback about static ip */
}

void lwip_reinitialize(netconfig* net_cfg)
{
    netconfig_ptr = net_cfg;
    printf("lwip reinit start\r\n");
    /* mem_init of lwip, init outside the lwip_init for user to using outside. */
    /*mem_init();
    memp_init();*/

#if LWIP_DHCP
    ip_addr_set_zero_ip4(&(WCH_NetIf.ip_addr));
#else
    /* IP addresses initialization */
    /*if(set_cfg == 1)
    {
        IP4_ADDR(&ipaddr, netconfig_ptr->ip[0], netconfig_ptr->ip[1], netconfig_ptr->ip[2],  netconfig_ptr->ip[3]);
        IP4_ADDR(&netmask, netconfig_ptr->mask[0], netconfig_ptr->mask[1], netconfig_ptr->mask[2],  netconfig_ptr->mask[3]);
        IP4_ADDR(&gw, netconfig_ptr->gw[0], netconfig_ptr->gw[1], netconfig_ptr->gw[2],  netconfig_ptr->gw[3]);
    }
    else
    {
        set_cfg = 0;
        IP4_ADDR(&ipaddr, 192, 168, 1, 86);
        IP4_ADDR(&netmask, 255, 255, 254, 0);
        IP4_ADDR(&gw, 192, 168, 1, 1);
    }*/
#endif
    /* Initilialize the LwIP stack*/
    //lwip_init();
    /* add the network interface (IPv4/IPv6)*/
    //netif_add(&WCH_NetIf, ip_2_ip4(&ipaddr), ip_2_ip4(&netmask), ip_2_ip4(&gw), NULL, &ethernetif_init, &ethernet_input);
    IP4_ADDR(&ipaddr, netconfig_ptr->ip[0], netconfig_ptr->ip[1], netconfig_ptr->ip[2],  netconfig_ptr->ip[3]);
    IP4_ADDR(&netmask, netconfig_ptr->mask[0], netconfig_ptr->mask[1], netconfig_ptr->mask[2],  netconfig_ptr->mask[3]);
    IP4_ADDR(&gw, netconfig_ptr->gw[0], netconfig_ptr->gw[1], netconfig_ptr->gw[2],  netconfig_ptr->gw[3]);
    netif_set_addr(&WCH_NetIf, ip_2_ip4(&ipaddr), ip_2_ip4(&netmask), ip_2_ip4(&gw));
    printf("lwip_reinit() ok\r\r\n");

    
    // Registers the default network interface
    /*netif_set_default(&WCH_NetIf);

    if (netif_is_link_up(&WCH_NetIf))
    {
        // When the netif is fully configured this function must be called 
        netif_set_up(&WCH_NetIf);
        
    #if LWIP_DHCP
        int err;
        /*  Creates a new DHCP client for this interface on the first call.
        Note: you must call dhcp_fine_tmr() and dhcp_coarse_tmr() at
        the predefined regular intervals after starting the client.
        You can peek in the netif->dhcp struct for the actual DHCP status.

        printf("dhcp_start()\r\r\n");
        err = dhcp_start(&WCH_NetIf);      // start dhcp /
        if(err == ERR_OK)
        {
        printf("1()\r\r\n");
            printf("lwip dhcp start success...\r\n\r\n");
        }
        else
        {
        printf("2()\r\r\n");
            printf("lwip dhcp start fail...\r\n\r\n");
        }
        // poll for dhcp status
        sys_timeout(50, wait_dhcp, NULL);

    #else
        printf("netif_set_up()\r\n");

    #endif

    #if LWIP_IPV6
        netif_create_ip6_linklocal_address(&WCH_NetIf, 1);
        netif_set_ip6_autoconfig_enabled(&WCH_NetIf, 1);
    #endif

    }
    else
    {
        // When the netif link is down this function must be called 
        netif_set_down(&WCH_NetIf);
    }

    lwip_init_success_callback( & (WCH_NetIf.ip_addr) ); // notify callback about static ip */
}

void lwip_loop(void)
{

    {
        //if(list_head(ch307_mac_rec) != NULL)
        {
            // received a packet 
            //printf("new received packet\r\n");
            ethernetif_input(&WCH_NetIf);
        }
        net_led_tmr();
        //OS_TASK_SET_STATE();
        sys_check_timeouts();
        WCHNET_MainTask();
        //OS_TASK_CWAITX(5);      /* check again in 5ms*/
    }
}

/*********************************************************************
 * @fn      CH30x_RNG_GENERATE
 *
 * @brief   CH30x_RNG_GENERATE api function for lwip.
 *
 * @param   None.
 *
 * @return  None.
 */
uint32_t CH30x_RNG_GENERATE()
{
    while(1)
    {
        if(RNG_GetFlagStatus(RNG_FLAG_DRDY) == SET)
        {
            break;
        }
        if(RNG_GetFlagStatus(RNG_FLAG_CECS) == SET)
        {
            /* clock error / 时钟错误 */
            RNG_ClearFlag(RNG_FLAG_CECS);
            delay_us(100);
        }
        if(RNG_GetFlagStatus(RNG_FLAG_SECS) == SET)
        {
            /* seed error / 种子错误 */
            RNG_ClearFlag(RNG_FLAG_SECS);
            RNG_Cmd(DISABLE);
            delay_us(100);
            RNG_Cmd(ENABLE);
            delay_us(100);
        }
    }
    return RNG_GetRandomNumber();
}

uint32_t sysTicks = 0;

/*********************************************************************
 * @fn      sys_now
 *
 * @brief   sys_now api function for lwip.
 *
 * @param   None.
 *
 * @return  None.
 */
u32_t sys_now(void)
{
    return sysTicks;
}

//void SysTick_Handler_real(void);
/*void SysTick_Handler(void) {
  __asm volatile ("call SysTick_Handler_real; mret");
}

__attribute__((used))void SysTick_Handler_real()
{
    /*sysTicks++;
    //OS_UPDATE_TIMERS();
    //SysTick->SR=0;
    SysTick->SR=0;
        printf("SysTick_Handler = 0; \r\n");*/
    /*if(SysTick->SR == 1)
    {
        SysTick->SR = 0;//clear State flag
        //SysTick->CNT = 0;    
        
        //printf("welcome to WCH\r\n");
        sysTicks++;
        //printf("Counter:%d\r\n",sysTicks);
      /* SysTick->SR &= ~(1 << 0);//clear State flag
    SysTick->CMP = 1000;
    SysTick->CNT = 0;
    SysTick->CTLR = 0xF;
        NVIC_EnableIRQ(SysTicK_IRQn);
    }
}*/

uint32_t getTime()
{
    return sysTicks;
}

volatile uint8_t net_data_led_require = 0;
/*********************************************************************
 * @fn      net_led_tmr
 *
 * @brief   Called every NET_LED_PERIOD_MSECS to set LED status
 * lwip timeouts period timer NET_LED_PERIOD_MSECS 
 *
 * @param   None.
 *
 * @return  None.
 */
void net_led_tmr(void)
{
    static uint8_t net_data_led = 0;

    if(net_data_led)
    {
        /* Currently on  */
        net_data_led = 0;
        ETH_LedDataSet(LED_OFF);/* turn off data led. */
    }
    else
    {
        /* If off, another packet will turn it on   */
        if(net_data_led_require != 0)
        {
            net_data_led = 1;
            ETH_LedDataSet(LED_ON);
            //Ethernet_LED_DATASET(0);/* turn on data led. */
            net_data_led_require = 0;
        }
    }
}
/*********************************************************************
 * @fn      ETH_TxPkt_ChainMode
 *
 * @brief   MAC send a ethernet frame in chain mode.
 *
 * @param   Send length
 *
 * @return  Send status.
 */
uint32_t ETH_TxPkt_ChainMode(uint16_t FrameLength)
{
    //printf("ETH_TxPkt_ChainMode:\r\n");
    /* Check if the descriptor is owned by the ETHERNET DMA (when set) or CPU (when reset) */
    if((DMATxDescToSet->Status & ETH_DMATxDesc_OWN) != (u32)RESET)
    {
        /* Return ERROR: OWN bit set */
        if ((ETH->DMASR & ETH_DMASR_TBUS) != (u32)RESET)
        {
            /* Clear TBUS ETHERNET DMA flag */
            ETH_DMAClearITPendingBit(ETH_DMASR_TBUS);
            //ETH->DMASR &= ~ETH_DMASR_TBUS;
            
            /* Resume DMA transmission*/            
            ETH->DMATPDR = 0;
        }
        printf("Error:ETH_DMATxDesc_OWN.\r\n");

        return ETH_ERROR;
    }

    /* Setting the Frame Length: bits[12:0] */
    DMATxDescToSet->ControlBufferSize = (FrameLength & ETH_DMATxDesc_TBS1);
#ifdef CHECKSUM_BY_HARDWARE
    /* Setting the last segment and first segment bits (in this case a frame is transmitted in one descriptor) */
    DMATxDescToSet->Status |= ETH_DMATxDesc_LS | ETH_DMATxDesc_FS | ETH_DMATxDesc_CIC_TCPUDPICMP_Full;
#else
    DMATxDescToSet->Status |= ETH_DMATxDesc_LS | ETH_DMATxDesc_FS;
#endif

    /* Set Own bit of the Tx descriptor Status: gives the buffer back to ETHERNET DMA */
    DMATxDescToSet->Status |= ETH_DMATxDesc_OWN;

      /* When Tx Buffer unavailable flag is set: clear it and resume transmission */
    //if ((ETH->DMASR & ETH_DMASR_TBUS) != (u32)RESET)
    {
        //printf("if ((ETH->DMASR & ETH_DMASR_TBUS) != (u32)RESET).\r\n");
        /* Clear TBUS ETHERNET DMA flag */
        ETH->DMASR = ETH_DMASR_TBUS;
        //ETH_DMAClearITPendingBit(ETH_DMASR_TBUS);
        /* Resume DMA transmission*/

        ETH->DMATPDR = 0;
        //printf("if ((ETH->DMASR & ETH_DMASR_TBUS) != (u32)RESET).\r\n");
    }


    /* Update the ETHERNET DMA global Tx descriptor with next Tx decriptor */
    /* Chained Mode */
    /* Selects the next DMA Tx descriptor list for next buffer to send */
    DMATxDescToSet = (ETH_DMADESCTypeDef*) (DMATxDescToSet->Buffer2NextDescAddr);

    /* Return SUCCESS */
    return ETH_SUCCESS;
}

uint16_t Rx_pointer = 0;

uint16_t getPointer()
{
    return Rx_pointer; 
}
/*********************************************************************
 * @fn      ETH_RxPkt_ChainMode
 *
 * @brief   MAC receive a ethernet frame in chain mode.
 *
 * @return  Frame information.
 */
void* ETH_RxPkt_ChainMode(void)
{
    uint32_t offset = 0, framelength = 0;
    //printf("ETH_RxPkt_ChainMode:\r\n");
    
    FrameTypeDef *rec_frame = NULL;

    if((DMARxDescToGet->Status & ETH_DMARxDesc_OWN) != (uint32_t)RESET)
    {
        return ETH_ERROR;
    }

    while((DMARxDescToGet->Status & ETH_DMARxDesc_OWN) == (uint32_t)RESET)
    {
        if(((DMARxDescToGet->Status & ETH_DMARxDesc_ES) == (uint32_t)RESET) &&
        ((DMARxDescToGet->Status & ETH_DMARxDesc_LS) != (uint32_t)RESET) &&
        ((DMARxDescToGet->Status & ETH_DMARxDesc_FS) != (uint32_t)RESET))
        {
            if(((DMARxDescToGet->Status & ETH_DMARxDesc_MAMPCE) != (uint32_t)RESET))
            {
                 //printf("ETH_DMARxDesc_MAMPCE\r\n");  
            }
            if(((DMARxDescToGet->Status & ETH_DMARxDesc_CE) != (uint32_t)RESET))
            {
                //printf("ETH_DMARxDesc_CE\r\n");  
            }
            if(((DMARxDescToGet->Status & ETH_DMARxDesc_DE) != (uint32_t)RESET))
            {
                //printf("ETH_DMARxDesc_DE\r\n");  
            }
            if(((DMARxDescToGet->Status & ETH_DMARxDesc_RE) != (uint32_t)RESET))
            {
                //printf("ETH_DMARxDesc_RE\r\n");  
            }
            if(((DMARxDescToGet->Status & ETH_DMARxDesc_RWT) != (uint32_t)RESET))
            {
                //printf("ETH_DMARxDesc_RWT\r\n");  
            }
            if(((DMARxDescToGet->Status & ETH_DMARxDesc_LC) != (uint32_t)RESET) )
            {
                //printf("ETH_DMARxDesc_LC\r\n");  
            }
            if(((DMARxDescToGet->Status & ETH_DMARxDesc_IPV4HCE) != (uint32_t)RESET))
            {
                //printf("ETH_DMARxDesc_IPV4HCE\r\n");  
            }
            if(((DMARxDescToGet->Status & ETH_DMARxDesc_LE) != (uint32_t)RESET))
            {
                //printf("ETH_DMARxDesc_LE\r\n");  
            }
            if(((DMARxDescToGet->Status & ETH_DMARxDesc_SAF) != (uint32_t)RESET))
            {
                //printf("ETH_DMARxDesc_SAF\r\n");  
            }
            if(((DMARxDescToGet->Status & ETH_DMARxDesc_AFM) != (uint32_t)RESET))
            {
                //printf("ETH_DMARxDesc_AFM\r\n");  
            }
            if(((DMARxDescToGet->Status & ETH_DMARxDesc_FT) != (uint32_t)RESET))
            {
                //printf("ETH_DMARxDesc_FT\r\n");  
            }
         
            //rec_frame = memb_alloc(&ch307_mac_rec_frame_mem);
            framelength = ((DMARxDescToGet->Status & ETH_DMARxDesc_FL) >> ETH_DMARXDESC_FRAME_LENGTHSHIFT) - 4;
            //printf("framelength:%3x \r\n", framelength);
            if(Rx_pointer + framelength >= ETH_RXBUFNB*ETH_RX_BUF_SZE)
            {
                Rx_pointer = 0;
            }
            for(offset = 0; offset < framelength; offset++)
            {
                //printf("data:%3x \r\n", (*(__IO uint8_t *)((DMARxDescToGet->Buffer1Addr) + offset)));
                (*(RxToLWIP + Rx_pointer + offset)) = (*(__IO uint8_t *)((DMARxDescToGet->Buffer1Addr) + offset));
            }
            //rec_frame->buffer = &RxToLWIP[0]; //DMARxDescToGet->Buffer1Addr; //(uint32_t) &RxToLWIP[0];
            //rec_frame->descriptor = DMARxDescToGet; 
            //rec_frame->length = framelength;
            Rx_pointer += framelength;
            //printf("data:%5d \r\n",Rx_pointer);
            //printf("if(((DMARxDescToGet->Status & ETH_DMARxDesc_ES) == (uint32_t)RESET) \r\n");

            DMARxDescToGet->Status = ETH_DMARxDesc_OWN;
        }
        else
        {
            framelength = ETH_ERROR;
            return ETH_ERROR;
        }

        if((DMARxDescToGet->ControlBufferSize & ETH_DMARxDesc_RCH) != (uint32_t)RESET)
        {
            DMARxDescToGet = (ETH_DMADESCTypeDef *)(DMARxDescToGet->Buffer2NextDescAddr);
    
            //printf("(DMARxDescToGet->ControlBufferSize &\r\n");
        }
        else if((DMARxDescToGet->ControlBufferSize & ETH_DMARxDesc_RER) != (uint32_t)RESET)
        {
            DMARxDescToGet = (ETH_DMADESCTypeDef *)(ETH->DMARDLAR);
            //printf("(DMARxDescToGet->ControlBufferSize\r\n");
        }
    }


    if((ETH->DMASR & ETH_DMASR_RBUS) != (uint32_t)RESET)
    {
        ETH->DMASR = ETH_DMASR_RBUS;
        ETH->DMARPDR = 0;
        //printf("(ETH->DMASR & ETH_DMASR_RBUS) != (uint32_t)RESET\r\n");
    }

    return rec_frame;
}

/*********************************************************************
 * @fn      ETH_IRQHandler
 *
 * @brief   This function handles ETH exception.
 *
 * @return  none
 */
void ETH_IRQHandler(void) {
  __asm volatile ("call ETH_IRQHandler_real; mret");
}

__attribute__((used)) void ETH_IRQHandler_real(void) 
{
   
    uint32_t int_sta;
    int_sta = ETH->DMASR;
    if (int_sta & ETH_DMA_IT_AIS)
    {
        if (int_sta & ETH_DMA_IT_RBU)
        {
            //if((ChipId & 0xf0) == 0x10)
            //{
              //  ((ETH_DMADESCTypeDef *)(((ETH_DMADESCTypeDef *)(ETH->DMACHRDR))->Buffer2NextDescAddr))->Status = ETH_DMARxDesc_OWN;

                /* Resume DMA reception */
              //  ETH->DMARPDR = 0;
            //}
            //ETH->DMARPDR = 0;
            //ETH_DMAClearITPendingBit(ETH_DMA_IT_RBU);
                //if((ChipId & 0xf0) == 0x10)
                //{
                    //((ETH_DMADESCTypeDef *)(((ETH_DMADESCTypeDef *)(ETH->DMACHRDR))->Buffer2NextDescAddr))->Status = ETH_DMARxDesc_OWN;

                    /* Resume DMA reception */
                    //ETH->DMARPDR = 0;
                //}
                ETH_DMAClearITPendingBit(ETH_DMA_IT_RBU);
            
            //ETH_DMAClearITPendingBit(ETH_DMA_IT_AIS);
        }
        //if (int_sta & ETH_DMA_IT_TBU)
        //{
            //ETH->DMASR &= ~ETH_DMASR_TBUS;
            //ETH->DMATPDR = 0;
            //ETH->DMASR = ETH_DMASR_TBUS;
            /* Resume DMA transmission*/
            //ETH->DMATPDR = 0;
            //ETH_DMAClearITPendingBit(ETH_DMA_IT_TBU);
        //}
        //ETH_DMAClearITPendingBit(ETH_DMA_IT_AIS);
        ETH_DMAClearITPendingBit(ETH_DMA_IT_AIS);
    }

    if( int_sta & ETH_DMA_IT_NIS )
    {
        if( int_sta & ETH_DMA_IT_R )
        {
            /*If you don't use the Ethernet library,
             * you can do some data processing operations here*/
            
            void* p;
            p = ETH_RxPkt_ChainMode();
            //if(p != NULL)
            //{
            //    list_add(ch307_mac_rec, p); /* add to rec list. */
            //}
            ETH_DMAClearITPendingBit(ETH_DMA_IT_R);
        }
        if( int_sta & ETH_DMA_IT_T )
        {
            ETH_DMAClearITPendingBit(ETH_DMA_IT_T);
            /* Clear TBUS ETHERNET DMA flag */
            //ETH->DMASR &= ~ETH_DMASR_TBUS;
            //ETH_DMAClearITPendingBit(ETH_DMASR_TBUS);
            /* Resume DMA transmission*/

            //ETH->DMATPDR = 0;
            //printf("ETH_IRQHandler transmitt ok:\r\n");
        }
        if( int_sta & ETH_DMA_IT_PHYLINK)
        {
            ETH_PHYLink();
            ETH_DMAClearITPendingBit(ETH_DMA_IT_PHYLINK);
        }
        ETH_DMAClearITPendingBit(ETH_DMA_IT_NIS);
    }
    //NVIC_EnableIRQ(ETH_IRQn);
    //printf("ETH_IRQHandler end:\r\n");
}
/*********************************************************************
 * @fn      mac_send
 *
 * @brief   MAC send a ethernet frame in chain mode.
 *
 * @param   Send length and send pointer.
 *
 * @return  none
 */
void mac_send(uint8_t * content_ptr, uint16_t content_len)
{
    printf("mac_send.\r\n");
    u8 *buffer =  (u8 *)ETH_GetCurrentTxBufferAddress();

    memcpy(buffer, content_ptr, content_len);
    
    if( !ETH_TxPkt_ChainMode(content_len))
    {
        printf("Send failed.\r\n");
    }
}

/*********************************************************************
 * @fn      PHY_control_pin_init
 *
 * @brief   PHY interrupt GPIO Initialization.
 *
 * @return  none
 */
void PHY_control_pin_init(void)
{
    GPIO_InitTypeDef  GPIO_InitStructure;
    EXTI_InitTypeDef EXTI_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO|RCC_APB2Periph_GPIOB,ENABLE);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_15;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    GPIO_EXTILineConfig(GPIO_PortSourceGPIOA,GPIO_PinSource15);
    EXTI_InitStructure.EXTI_Line=EXTI_Line15;
    EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;
    EXTI_InitStructure.EXTI_LineCmd = ENABLE;
    EXTI_Init(&EXTI_InitStructure);

    NVIC_InitStructure.NVIC_IRQChannel = EXTI15_10_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
    NVIC_EnableIRQ(EXTI15_10_IRQn);
}

/*********************************************************************
 * @fn      GETH_pin_init
 *
 * @brief   PHY RGMII interface GPIO initialization.
 *
 * @return  none
 */
void GETH_pin_init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    /* PB12/13 set as push-pull multiplexed output  */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA|RCC_APB2Periph_GPIOB|RCC_APB2Periph_GPIOC, ENABLE);
    GPIOB->CFGHR&=~(0xff<<16);
    GPIOB->CFGHR|= (0xbb<<16);
    GPIOB->CFGLR&=~(0xff<<4);

    define_O(GPIOA,GPIO_Pin_2);
    define_O(GPIOA,GPIO_Pin_3);
    define_O(GPIOA,GPIO_Pin_7);
    define_O(GPIOC,GPIO_Pin_4);
    define_O(GPIOC,GPIO_Pin_5);
    define_O(GPIOB,GPIO_Pin_0);

    define_I(GPIOC,GPIO_Pin_0);
    define_I(GPIOC,GPIO_Pin_1);
    define_I(GPIOC,GPIO_Pin_2);
    define_I(GPIOC,GPIO_Pin_3);
    define_I(GPIOA,GPIO_Pin_0);
    define_I(GPIOA,GPIO_Pin_1);

    define_I(GPIOB,GPIO_Pin_1);/* 125m in */
}

/*********************************************************************
 * @fn      FETH_pin_init
 *
 * @brief   PHY MII/RMII interface GPIO initialization.
 *
 * @return  none
 */
void FETH_pin_init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

#ifdef USE_RMII
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA|RCC_APB2Periph_GPIOB|RCC_APB2Periph_GPIOC|RCC_APB2Periph_AFIO, ENABLE);
    GPIO_ETH_MediaInterfaceConfig(GPIO_ETH_MediaInterface_RMII);
    define_O(GPIOA,GPIO_Pin_2);/* MDC */
    define_O(GPIOC,GPIO_Pin_1);/* MDIO */

    define_O(GPIOB,GPIO_Pin_11);//txen
    define_O(GPIOB,GPIO_Pin_12);//txd0
    define_O(GPIOB,GPIO_Pin_13);//txd1

    define_I(GPIOA,GPIO_Pin_1);/* PA1 REFCLK */
    define_I(GPIOA,GPIO_Pin_7);/* PA7 CRSDV */
    define_I(GPIOC,GPIO_Pin_4);/* RXD0 */
    define_I(GPIOC,GPIO_Pin_5);/* RXD1 */

#else
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA|RCC_APB2Periph_GPIOB|RCC_APB2Periph_GPIOC, ENABLE);

    define_O(GPIOA,GPIO_Pin_2);/* MDC */
    define_O(GPIOC,GPIO_Pin_1);/* MDIO */

    define_I(GPIOC,GPIO_Pin_3);//txclk
    define_O(GPIOB,GPIO_Pin_11);//txen
    define_O(GPIOB,GPIO_Pin_12);//txd0
    define_O(GPIOB,GPIO_Pin_13);//txd1
    define_O(GPIOC,GPIO_Pin_2); //txd2
    define_O(GPIOB,GPIO_Pin_8);//txd3
/* RX组 */
    define_I(GPIOA,GPIO_Pin_1);/* PA1 RXC */
    define_I(GPIOA,GPIO_Pin_7);/* PA7 RXDV */
    define_I(GPIOC,GPIO_Pin_4);/* RXD0 */
    define_I(GPIOC,GPIO_Pin_5);/* RXD1 */
    define_I(GPIOB,GPIO_Pin_0);/* RXD2 */
    define_I(GPIOB,GPIO_Pin_1);/* RXD3 */
    define_I(GPIOB,GPIO_Pin_10);/* RXER */

    define_O(GPIOA,GPIO_Pin_0);/* PA0 */
    define_O(GPIOA,GPIO_Pin_3);/* PA3 */
#endif
}

void ReInitMACReg(void);

/*********************************************************************
 * @fn      WCHNET_GetMacAddr
 *
 * @brief   Get the MAC address
 *
 * @return  none.
 */
void WCHNET_GetMacAddr( uint8_t *p )
{
    uint8_t i;
    uint8_t *macaddr=(uint8_t *)(ROM_CFG_USERADR_ID+5);

    for(i=0;i<6;i++)
    {
        *p = *macaddr;
        p++;
        macaddr--;
    }
}

/*********************************************************************
 * @fn      WCHNET_TimeIsr
 *
 * @brief
 *
 * @return  none.
 */
void WCHNET_TimeIsr( uint16_t timperiod )
{
    LocalTime += timperiod;
}

/*********************************************************************
 * @fn      WCHNET_PhyPNProcess
 *
 * @brief   Phy PN Polarity related processing
 *
 * @param   none.
 *
 * @return  none.
 */
void WCHNET_PhyPNProcess(void)
{
    //printf("WCHNET_PhyPNProcess(void)r\n");
    uint32_t PhyVal;

    LinkSuccTime = sysTicks;
    if((ETH->MMCRGUFCR == 0) && (ETH->MMCRFCECR >= 3))
    {
        PhyVal = ETH_ReadPHYRegister(gPHYAddress, PHY_MDIX);
        if((PhyVal >> 2) & 0x01)
            PhyVal &= ~(3 << 2);                //change PHY PN Polarity to normal
        else
            PhyVal |= 1 << 2;                   //change PHY PN Polarity to reverse
        ETH_WritePHYRegister(gPHYAddress, PHY_MDIX, PhyVal);
        ETH->MMCCR |= ETH_MMCCR_CR;             //Counters Reset
        while(ETH->MMCCR & ETH_MMCCR_CR);       //Wait for counters reset to complete
    }
    if(ETH->MMCRGUFCR != 0)
    {
        PhyPolarityDetect = 0;
        /* enable Filter function */
        ETH->MACFFR &= ~(ETH_ReceiveAll_Enable | ETH_PromiscuousMode_Enable);
    }
}

/*********************************************************************
 * @fn      WCHNET_RecProcess
 *
 * @brief   Receiving related processing
 *
 * @param   none.
 *
 * @return  none.
 */
void WCHNET_RecProcess(void)
{
    if(((ChipId & 0xf0) == 0x20) && \
            ((ETH->DMAMFBOCR & 0x1FFE0000) != 0))
    {
        ReInitMACReg();
    }
}

/*********************************************************************
 * @fn      WCHNET_LinkProcess
 *
 * @brief   link process.
 *
 * @param   none.
 *
 * @return  none.
 */
void WCHNET_LinkProcess( void )
{
    uint16_t phy_anlpar, phy_bmsr, phy_mdix, RegVal;

    phy_anlpar = ETH_ReadPHYRegister(gPHYAddress, PHY_ANLPAR);
    phy_bmsr = ETH_ReadPHYRegister( gPHYAddress, PHY_BMSR);

    if( (phy_anlpar&PHY_ANLPAR_SELECTOR_FIELD) )
    {
        if(TRDetectStep == 0)
        {
            TRDetectStep = 1;
            TRDetectCnt = 1;
            PHY_TR_SWITCH();
            LinkTaskPeriod = RandVal%100 + 50;
            return;
        }
        else if(TRDetectStep == 1)
        {
            TRDetectStep = 2;
            TRDetectCnt = 0;
        }
        if( !(phyLinkStatus&PHY_LINK_WAIT_SUC) )
        {
            if( phyPN == PHY_PN_SWITCH_AUTO )
            {
                PHY_PN_SWITCH(PHY_PN_SWITCH_P);
            }
            else if( phyPN == PHY_PN_SWITCH_P )
            {
                phyLinkStatus = PHY_LINK_WAIT_SUC;
            }
            else
            {
                phyLinkStatus = PHY_LINK_WAIT_SUC;
            }
        }
        else{
            if((phySucCnt++ == 5) && ((phy_bmsr&(1<<5)) == 0))
            {
                phySucCnt = 0;
                if(phyPN == PHY_PN_SWITCH_N)
                    PHY_PN_SWITCH(PHY_PN_SWITCH_P);
                else PHY_PN_SWITCH(PHY_PN_SWITCH_N);
            }
        }
        phyLinkCnt = 0;
    }
    else
    {
        if(TRDetectStep == 1)
        {
            TRDetectCnt++;
            if(TRDetectCnt == 8)
            {
                TRDetectCnt = 0;
                TRDetectStep = 0;
                ETH_WritePHYRegister(gPHYAddress, PHY_MDIX, PHY_PN_SWITCH_AUTO);
                return;
            }
            PHY_TR_SWITCH();
            return;
        }
        if( phyLinkStatus == PHY_LINK_WAIT_SUC )
        {
            if(phyLinkCnt++ == 15 )
            {
                phyLinkCnt = 0;
                phySucCnt = 0;
                TRDetectStep = 0;
                phyLinkStatus = PHY_LINK_INIT;
                PHY_PN_SWITCH(PHY_PN_SWITCH_AUTO);
            }
        }
        else
        {
            if( phyPN == PHY_PN_SWITCH_P )
            {
                if(phyLinkCnt++ == 4 )
                {
                    phyLinkCnt = 0;
                    PHY_PN_SWITCH(PHY_PN_SWITCH_N);
                }
            }
            else if( phyPN == PHY_PN_SWITCH_N )
            {
                if(phyLinkCnt++ == 15 )
                {
                    phyLinkCnt = 0;
                    phySucCnt = 0;
                    TRDetectStep = 0;
                    phyLinkStatus = PHY_LINK_INIT;
                    PHY_PN_SWITCH(PHY_PN_SWITCH_AUTO);
                }
            }
            else
            {
                if(phyLinkCnt++ == (5000 / PHY_LINK_TASK_PERIOD))
                    PHY_LINK_RESET( );
            }
        }
    }
}

/*********************************************************************
 * @fn      WCHNET_HandlePhyNegotiation
 *
 * @brief   Handle PHY Negotiation.
 *
 * @param   none.
 *
 * @return  none.
 */
void WCHNET_HandlePhyNegotiation(void)
{
    if(phyLinkReset)              /* After the PHY link is disconnected, wait 500ms before turning on the PHY clock*/
    {
        if( sysTicks - phyLinkTime >= 500 )
        {
            phyLinkReset = 0;
            EXTEN->EXTEN_CTR |= EXTEN_ETH_10M_EN;
            PHY_LINK_RESET();
        }
    }
    else 
    {
        if( !phyStatus )          /* Handling PHY Negotiation Exceptions */
        {
            ACCELERATE_LINK_PROCESS();
            if( sysTicks - phyLinkTime >= LinkTaskPeriod )
            {
                UPDATE_LINKTASKPERIOD();
                phyLinkTime = sysTicks;
                WCHNET_LinkProcess( );//Ethernet_LED_LINKSET(1);
            }
            if(ReInitMACFlag) ReInitMACFlag = 0;
        }
        else
        {                     /* PHY link complete */
            if(ReInitMACFlag)
            {
                if( sysTicks - phyLinkTime >= 5 * PHY_LINK_TASK_PERIOD )
                {
                    u32 phy_stat;
                    ReInitMACFlag = 0;
                    phy_stat = ETH_ReadPHYRegister( gPHYAddress, PHY_BMSR);
                    if((phy_stat&PHY_Linked_Status) == 0)
                    {
                        //WCHNET_PhyStatus( phy_stat );
                        //TH_PHYLink();
                        PHY_LINK_RESET();
                    }
                }
            }
            if(PhyPolarityDetect)
            {
                if( sysTicks - LinkSuccTime >= 2 * PHY_LINK_TASK_PERIOD )
                {
                    WCHNET_PhyPNProcess();
                }
            }
        }
    }
}

/*********************************************************************
 * @fn      WCHNET_MainTask
 *
 * @brief   library main task function
 *
 * @param   none.
 *
 * @return  none.
 */
void WCHNET_MainTask(void)
{
    //WCHNET_NetInput( );                     /* Ethernet data input */
    //WCHNET_PeriodicHandle( );               /* Protocol stack time-related task processing */
    WCHNET_HandlePhyNegotiation();
    //WCHNET_RecProcess();
    //sys_check_timeouts();
}

/*********************************************************************
 * @fn      ETH_LedLinkSet
 *
 * @brief   set eth link led,setbit 0 or 1,the link led turn on or turn off
 *
 * @return  none
 */
void ETH_LedLinkSet( uint8_t mode )
{
    if( mode == LED_OFF )
    {
        GPIO_SetBits(GPIOC, GPIO_Pin_1);
    }
    else
    {
        GPIO_ResetBits(GPIOC, GPIO_Pin_1);
    }
}

/*********************************************************************
 * @fn      ETH_LedDataSet
 *
 * @brief   set eth data led,setbit 0 or 1,the data led turn on or turn off
 *
 * @return  none
 */
void ETH_LedDataSet( uint8_t mode )
{
    if( mode == LED_OFF )
    {
        GPIO_SetBits(GPIOC, GPIO_Pin_0);
    }
    else
    {
        GPIO_ResetBits(GPIOC, GPIO_Pin_0);
    }
}

/*********************************************************************
 * @fn      ETH_LedConfiguration
 *
 * @brief   set eth data and link led pin
 *
 * @param   none.
 *
 * @return  none.
 */
void ETH_LedConfiguration(void)
{
    GPIO_InitTypeDef  GPIO={0};

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC,ENABLE);
    GPIO.GPIO_Pin = GPIO_Pin_0|GPIO_Pin_1;
    GPIO.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOC,&GPIO);
    ETH_LedDataSet(LED_OFF);
    ETH_LedLinkSet(LED_OFF);
}

/*********************************************************************
 * @fn      ETH_SetClock
 *
 * @brief   Set ETH Clock(60MHZ).
 *
 * @param   none.
 *
 * @return  none.
 */
void ETH_SetClock(void)
{
    RCC_PLL3Cmd(DISABLE);
    RCC_PREDIV2Config(RCC_PREDIV2_Div2);                             /* HSE = 8M */
    RCC_PLL3Config(RCC_PLL3Mul_15);                                  /* 4M*15 = 60MHz */
    RCC_PLL3Cmd(ENABLE);
    while(RESET == RCC_GetFlagStatus(RCC_FLAG_PLL3RDY));
}

/*********************************************************************
 * @fn      ETH_LinkUpCfg
 *
 * @brief   When the PHY is connected, configure the relevant functions.
 *
 * @param   regval  BMSR register value
 *
 * @return  none.
 */
void ETH_LinkUpCfg(uint16_t regval)
{
    printf("ETH_LinkUpCfg(uint16_t regval)\r\n");
   // WCHNET_PhyStatus( regval );
    ETH->MACCR &= ~(ETH_Speed_100M|ETH_Speed_1000M);
    phyStatus = PHY_Linked_Status;

    /* disable Filter function */
    ETH->MACFFR |= (ETH_ReceiveAll_Enable | ETH_PromiscuousMode_Enable);

    ETH->MMCCR |= ETH_MMCCR_CR;             //Counters Reset
    while(ETH->MMCCR & ETH_MMCCR_CR);       //Wait for counters reset to complete
    PhyPolarityDetect = 1;
    LinkSuccTime = sysTicks;
    ETH_Start( );
    //Ethernet_LED_LINKSET(1);
}

/*********************************************************************
 * @fn      ETH_PHYLink
 *
 * @brief   Configure MAC parameters after the PHY Link is successful.
 *
 * @param   none.
 *
 * @return  none.
 */
void ETH_PHYLink( void )
{
    printf("ETH_PHYLink(void)\r\n");
    u16 phy_bsr, phy_stat, phy_anlpar, phy_bcr;

    phy_bsr = ETH_ReadPHYRegister( gPHYAddress, PHY_BSR);
    phy_bcr = ETH_ReadPHYRegister( gPHYAddress, PHY_BCR);
    phy_anlpar = ETH_ReadPHYRegister( gPHYAddress, PHY_ANLPAR);

    if(phy_bsr & PHY_Linked_Status)   //LinkUp
    {
        ETH_LedLinkSet(LED_ON);
        if(phy_bcr & PHY_AutoNegotiation)   //determine whether auto-negotiation is enable
        {
            if(phy_anlpar == 0)
            {
                if(phy_bsr & PHY_AutoNego_Complete)
                {
                    ETH->MACCR &= ~ETH_Mode_FullDuplex;
                    ETH_LinkUpCfg(phy_bsr);
                }
                else{
                    PHY_LINK_RESET();
                }
            }
            else {
                if(phy_bsr & PHY_AutoNego_Complete)
                {
                    phy_stat = ETH_ReadPHYRegister( gPHYAddress, PHY_STATUS );
                    if( phy_stat & (1<<2) )
                    {
                        ETH->MACCR |= ETH_Mode_FullDuplex;
                    }
                    else
                    {
                        if( (phy_anlpar&PHY_ANLPAR_SELECTOR_FIELD) != PHY_ANLPAR_SELECTOR_VALUE )
                        {
                            ETH->MACCR |= ETH_Mode_FullDuplex;
                        }
                        else
                        {
                            ETH->MACCR &= ~ETH_Mode_FullDuplex;
                        }
                    }
                    ETH_LinkUpCfg(phy_bsr);//Ethernet_LED_LINKSET(1);
                }
                else{
                    //WCHNET_PhyStatus( phy_bsr );
                    EXTEN->EXTEN_CTR &= ~EXTEN_ETH_10M_EN;
                    phyLinkReset = 1;
                    phyLinkTime = sysTicks;
                }
            }
        }
        else {
            ETH->MACCR &= ~ETH_Mode_FullDuplex;
            ETH_LinkUpCfg(phy_bsr);;
        }
    }
    else {                              //LinkDown
        ETH_LedLinkSet(LED_OFF);
        //WCHNET_PhyStatus( phy_bsr );
        EXTEN->EXTEN_CTR &= ~EXTEN_ETH_10M_EN;
        phyLinkReset = 1;
        phyLinkTime = sysTicks;
    }
    DuplexMode = (ETH->MACCR >> 11) & 0x01;  /* Record duplex mode*/
    //Ethernet_LED_LINKSET(1);
}

/*********************************************************************
 * @fn      ReInitMACReg
 *
 * @brief   Reinitialize MAC register.
 *
 * @param   none.
 *
 * @return  none.
 */
void ReInitMACReg(void)
{
    printf("ReInitMACReg(void)\r\n");
    ETH_InitTypeDef ETH_InitStructure;
    uint16_t timeout = 10000;
    uint16_t RegVal;
    uint32_t tmpreg = 0;

    /* Wait for sending data to complete */
    while((ETH->DMASR & (7 << 20)) != ETH_DMA_TransmitProcess_Suspended);

    PHY_TR_REVERSE();

    /* Software reset */
    ETH_SoftwareReset();
    /* Wait for software reset */
    do{
        delay_us(10);
        if( !--timeout )  break;
    }while(ETH->DMABMR & ETH_DMABMR_SR);

    /* ETHERNET Configuration */
    /* Call ETH_StructInit if you don't like to configure all ETH_InitStructure parameter */
    ETH_StructInit(&ETH_InitStructure);
    /* Fill ETH_InitStructure parameters */
    /*------------------------   MAC   -----------------------------------*/
    ETH_InitStructure.ETH_Mode = ETH_Mode_FullDuplex;
    ETH_InitStructure.ETH_Speed = ETH_Speed_10M;
#ifdef CHECKSUM_BY_HARDWARE
    ETH_InitStructure.ETH_ChecksumOffload = ETH_ChecksumOffload_Enable;
#endif
    ETH_InitStructure.ETH_AutoNegotiation = ETH_AutoNegotiation_Enable;
    ETH_InitStructure.ETH_LoopbackMode = ETH_LoopbackMode_Enable;
    ETH_InitStructure.ETH_RetryTransmission = ETH_RetryTransmission_Disable;
    ETH_InitStructure.ETH_AutomaticPadCRCStrip = ETH_AutomaticPadCRCStrip_Disable;
    /* Filter function configuration */
    ETH_InitStructure.ETH_ReceiveAll = ETH_ReceiveAll_Disable;
    ETH_InitStructure.ETH_PromiscuousMode = ETH_PromiscuousMode_Disable;
    ETH_InitStructure.ETH_BroadcastFramesReception = ETH_BroadcastFramesReception_Enable;
    ETH_InitStructure.ETH_MulticastFramesFilter = ETH_MulticastFramesFilter_Perfect;
    ETH_InitStructure.ETH_UnicastFramesFilter = ETH_UnicastFramesFilter_Perfect;
    /*------------------------   DMA   -----------------------------------*/
    /* When we use the Checksum offload feature, we need to enable the Store and Forward mode:
    the store and forward guarantee that a whole frame is stored in the FIFO, so the MAC can insert/verify the checksum,
    if the checksum is OK the DMA can handle the frame otherwise the frame is dropped 
    ETH_InitStructure.ETH_DropTCPIPChecksumErrorFrame = ETH_DropTCPIPChecksumErrorFrame_Enable;
    ETH_InitStructure.ETH_TransmitStoreForward = ETH_TransmitStoreForward_Enable;
    ETH_InitStructure.ETH_ForwardErrorFrames = ETH_ForwardErrorFrames_Enable;
    ETH_InitStructure.ETH_ForwardUndersizedGoodFrames = ETH_ForwardUndersizedGoodFrames_Enable;
    
    
    /*------------------------   DMA   -----------------------------------*/
    /* When we use the Checksum offload feature, we need to enable the Store and Forward mode:
    the store and forward guarantee that a whole frame is stored in the FIFO, so the MAC can insert/verify the checksum,
    if the checksum is OK the DMA can handle the frame otherwise the frame is dropped */
    ETH_InitStructure.ETH_DropTCPIPChecksumErrorFrame = ETH_DropTCPIPChecksumErrorFrame_Enable;
    ETH_InitStructure.ETH_ReceiveStoreForward = ETH_ReceiveStoreForward_Enable;
    ETH_InitStructure.ETH_TransmitStoreForward = ETH_TransmitStoreForward_Enable;
    ETH_InitStructure.ETH_ForwardErrorFrames = ETH_ForwardErrorFrames_Enable;
    ETH_InitStructure.ETH_ForwardUndersizedGoodFrames = ETH_ForwardUndersizedGoodFrames_Enable;
    ETH_InitStructure.ETH_SecondFrameOperate = ETH_SecondFrameOperate_Disable;
    ETH_InitStructure.ETH_AddressAlignedBeats = ETH_AddressAlignedBeats_Enable;
    ETH_InitStructure.ETH_FixedBurst = ETH_FixedBurst_Enable;
    ETH_InitStructure.ETH_RxDMABurstLength = ETH_RxDMABurstLength_32Beat;
    ETH_InitStructure.ETH_TxDMABurstLength = ETH_TxDMABurstLength_32Beat;
    ETH_InitStructure.ETH_DMAArbitration = ETH_DMAArbitration_RoundRobin_RxTx_2_1;
    /* Configure Ethernet */
    /*---------------------- Physical layer configuration -------------------*/
    /* Set the SMI interface clock, set as the main frequency divided by 42  */
    tmpreg = ETH->MACMIIAR;
    tmpreg &= MACMIIAR_CR_MASK;
    tmpreg |= (uint32_t)ETH_MACMIIAR_CR_Div42;
    ETH->MACMIIAR = (uint32_t)tmpreg;

    /*------------------------ MAC register configuration  ----------------------- --------------------*/
    tmpreg = ETH->MACCR;
    tmpreg &= MACCR_CLEAR_MASK;
    tmpreg |= (uint32_t)(ETH_InitStructure.ETH_Watchdog |
                    ETH_InitStructure.ETH_Jabber |
                    ETH_InitStructure.ETH_InterFrameGap |
                    ETH_InitStructure.ETH_CarrierSense |
                    ETH_InitStructure.ETH_Speed |
                    ETH_InitStructure.ETH_ReceiveOwn |
                    ETH_InitStructure.ETH_LoopbackMode |
                    ETH_InitStructure.ETH_Mode |
                    ETH_InitStructure.ETH_ChecksumOffload |
                    ETH_InitStructure.ETH_RetryTransmission |
                    ETH_InitStructure.ETH_AutomaticPadCRCStrip |
                    ETH_InitStructure.ETH_BackOffLimit |
                    ETH_InitStructure.ETH_DeferralCheck);
    /* Write MAC Control Register */
    ETH->MACCR = (uint32_t)tmpreg;
    ETH->MACCR |= ETH_Internal_Pull_Up_Res_Enable;  /*Turn on the internal pull-up resistor*/
    ETH->MACFFR = (uint32_t)(ETH_InitStructure.ETH_ReceiveAll |
                          ETH_InitStructure.ETH_SourceAddrFilter |
                          ETH_InitStructure.ETH_PassControlFrames |
                          ETH_InitStructure.ETH_BroadcastFramesReception |
                          ETH_InitStructure.ETH_DestinationAddrFilter |
                          ETH_InitStructure.ETH_PromiscuousMode |
                          ETH_InitStructure.ETH_MulticastFramesFilter |
                          ETH_InitStructure.ETH_UnicastFramesFilter);
    /*--------------- ETHERNET MACHTHR and MACHTLR Configuration ---------------*/
    /* Write to ETHERNET MACHTHR */
    ETH->MACHTHR = (uint32_t)ETH_InitStructure.ETH_HashTableHigh;
    /* Write to ETHERNET MACHTLR */
    ETH->MACHTLR = (uint32_t)ETH_InitStructure.ETH_HashTableLow;
    /*----------------------- ETHERNET MACFCR Configuration --------------------*/
    /* Get the ETHERNET MACFCR value */
    tmpreg = ETH->MACFCR;
    /* Clear xx bits */
    tmpreg &= MACFCR_CLEAR_MASK;
    tmpreg |= (uint32_t)((ETH_InitStructure.ETH_PauseTime << 16) |
                     ETH_InitStructure.ETH_ZeroQuantaPause |
                     ETH_InitStructure.ETH_PauseLowThreshold |
                     ETH_InitStructure.ETH_UnicastPauseFrameDetect |
                     ETH_InitStructure.ETH_ReceiveFlowControl |
                     ETH_InitStructure.ETH_TransmitFlowControl);
    ETH->MACFCR = (uint32_t)tmpreg;

    ETH->MACVLANTR = (uint32_t)(ETH_InitStructure.ETH_VLANTagComparison |
                               ETH_InitStructure.ETH_VLANTagIdentifier);

    tmpreg = ETH->DMAOMR;
    tmpreg &= DMAOMR_CLEAR_MASK;
    tmpreg |= (uint32_t)(ETH_InitStructure.ETH_DropTCPIPChecksumErrorFrame |
                    ETH_InitStructure.ETH_ReceiveStoreForward |
                    ETH_InitStructure.ETH_FlushReceivedFrame |
                    ETH_InitStructure.ETH_TransmitStoreForward |
                    ETH_InitStructure.ETH_TransmitThresholdControl |
                    ETH_InitStructure.ETH_ForwardErrorFrames |
                    ETH_InitStructure.ETH_ForwardUndersizedGoodFrames |
                    ETH_InitStructure.ETH_ReceiveThresholdControl |
                    ETH_InitStructure.ETH_SecondFrameOperate);
    ETH->DMAOMR = (uint32_t)tmpreg;

    ETH->DMABMR = (uint32_t)(ETH_InitStructure.ETH_AddressAlignedBeats |
                            ETH_InitStructure.ETH_FixedBurst |
                            ETH_InitStructure.ETH_RxDMABurstLength | /* !! if 4xPBL is selected for Tx or Rx it is applied for the other */
                            ETH_InitStructure.ETH_TxDMABurstLength |
                           (ETH_InitStructure.ETH_DescriptorSkipLength << 2) |
                            ETH_InitStructure.ETH_DMAArbitration |
                            ETH_DMABMR_USP);
    /* Configure MAC address */
    ETH->MACA0HR = (uint32_t)((MACAddr[5]<<8) | MACAddr[4]);
    ETH->MACA0LR = (uint32_t)(MACAddr[0] | (MACAddr[1]<<8) | (MACAddr[2]<<16) | (MACAddr[3]<<24));

    /* Mask the interrupt that Tx good frame count counter reaches half the maximum value */
    ETH->MMCTIMR = ETH_MMCTIMR_TGFM;
    /* Mask the interrupt that Rx good unicast frames counter reaches half the maximum value */
    /* Mask the interrupt that Rx crc error counter reaches half the maximum value */
    ETH->MMCRIMR = ETH_MMCRIMR_RGUFM | ETH_MMCRIMR_RFCEM;

    /*ETH_DMAITConfig(ETH_DMA_IT_NIS |
                    ETH_DMA_IT_R |
                    ETH_DMA_IT_T ,
                    ENABLE);*/

    ETH_DMAITConfig(ETH_DMA_IT_NIS |
                    ETH_DMA_IT_R |
                    ETH_DMA_IT_T |
                    ETH_DMA_IT_AIS |
                    ETH_DMA_IT_RBU |
                    //ETH_DMA_IT_TBU |   
                    ETH_DMA_IT_PHYLINK,
                    ENABLE);
    
    ETH_DMATxDescChainInit(DMATxDscrTab, Tx_Buff, ETH_TXBUFNB);
    ETH_DMARxDescChainInit(DMARxDscrTab, Rx_Buff, ETH_RXBUFNB);
    pDMARxSet = DMARxDscrTab;
    pDMATxSet = DMATxDscrTab;

    ETH->MACCR &= ~ETH_Mode_FullDuplex;     //configure working mode based on the link result
    if(DuplexMode)
    {
        ETH->MACCR |= ETH_Mode_FullDuplex;
    }

    ETH->MACCR &= ~(ETH_Speed_100M|ETH_Speed_1000M);

    ETH_Start( );

    PHY_TR_REVERSE();

    if(!phyStatus)
    {
        PHY_LINK_RESET();
    }

    ReInitMACFlag = 1;
    phyLinkTime = sysTicks;
}

/*********************************************************************
 * @fn      ETH_RegInit
 *
 * @brief   ETH register initialization.
 *
 * @param   ETH_InitStruct:initialization struct.
 *          PHYAddress:PHY address.
 *
 * @return  Initialization status.
 */
uint32_t ETH_RegInit( ETH_InitTypeDef* ETH_InitStruct, uint16_t PHYAddress )
{
    printf("ETH_RegInit( ETH_InitTypeDef* ETH_InitStruct, uint16_t PHYAddress )");
    uint32_t tmpreg = 0;

    /*---------------------- Physical layer configuration -------------------*/
    /* Set the SMI interface clock, set as the main frequency divided by 42  */
    tmpreg = ETH->MACMIIAR;
    tmpreg &= MACMIIAR_CR_MASK;
    tmpreg |= (uint32_t)ETH_MACMIIAR_CR_Div42;
    ETH->MACMIIAR = (uint32_t)tmpreg;

    /*------------------------ MAC register configuration  ----------------------- --------------------*/
    tmpreg = ETH->MACCR;
    tmpreg &= MACCR_CLEAR_MASK;
    tmpreg |= (uint32_t)(ETH_InitStruct->ETH_Watchdog |
                    ETH_InitStruct->ETH_Jabber |
                    ETH_InitStruct->ETH_InterFrameGap |
                    ETH_InitStruct->ETH_CarrierSense |
                    ETH_InitStruct->ETH_Speed |
                    ETH_InitStruct->ETH_ReceiveOwn |
                    ETH_InitStruct->ETH_LoopbackMode |
                    ETH_InitStruct->ETH_Mode |
                    ETH_InitStruct->ETH_ChecksumOffload |
                    ETH_InitStruct->ETH_RetryTransmission |
                    ETH_InitStruct->ETH_AutomaticPadCRCStrip |
                    ETH_InitStruct->ETH_BackOffLimit |
                    ETH_InitStruct->ETH_DeferralCheck);
    /* Write MAC Control Register */
    ETH->MACCR = (uint32_t)tmpreg;
    ETH->MACCR |= ETH_Internal_Pull_Up_Res_Enable;  /*Turn on the internal pull-up resistor*/
    ETH->MACFFR = (uint32_t)(ETH_InitStruct->ETH_ReceiveAll |
                          ETH_InitStruct->ETH_SourceAddrFilter |
                          ETH_InitStruct->ETH_PassControlFrames |
                          ETH_InitStruct->ETH_BroadcastFramesReception |
                          ETH_InitStruct->ETH_DestinationAddrFilter |
                          ETH_InitStruct->ETH_PromiscuousMode |
                          ETH_InitStruct->ETH_MulticastFramesFilter |
                          ETH_InitStruct->ETH_UnicastFramesFilter);
    /*--------------- ETHERNET MACHTHR and MACHTLR Configuration ---------------*/
    /* Write to ETHERNET MACHTHR */
    ETH->MACHTHR = (uint32_t)ETH_InitStruct->ETH_HashTableHigh;
    /* Write to ETHERNET MACHTLR */
    ETH->MACHTLR = (uint32_t)ETH_InitStruct->ETH_HashTableLow;
    /*----------------------- ETHERNET MACFCR Configuration --------------------*/
    /* Get the ETHERNET MACFCR value */
    tmpreg = ETH->MACFCR;
    /* Clear xx bits */
    tmpreg &= MACFCR_CLEAR_MASK;
    tmpreg |= (uint32_t)((ETH_InitStruct->ETH_PauseTime << 16) |
                     ETH_InitStruct->ETH_ZeroQuantaPause |
                     ETH_InitStruct->ETH_PauseLowThreshold |
                     ETH_InitStruct->ETH_UnicastPauseFrameDetect |
                     ETH_InitStruct->ETH_ReceiveFlowControl |
                     ETH_InitStruct->ETH_TransmitFlowControl);
    ETH->MACFCR = (uint32_t)tmpreg;

    ETH->MACVLANTR = (uint32_t)(ETH_InitStruct->ETH_VLANTagComparison |
                               ETH_InitStruct->ETH_VLANTagIdentifier);

    tmpreg = ETH->DMAOMR;
    tmpreg &= DMAOMR_CLEAR_MASK;
    tmpreg |= (uint32_t)(ETH_InitStruct->ETH_DropTCPIPChecksumErrorFrame |
                    ETH_InitStruct->ETH_ReceiveStoreForward |
                    ETH_InitStruct->ETH_FlushReceivedFrame |
                    ETH_InitStruct->ETH_TransmitStoreForward |
                    ETH_InitStruct->ETH_TransmitThresholdControl |
                    ETH_InitStruct->ETH_ForwardErrorFrames |
                    ETH_InitStruct->ETH_ForwardUndersizedGoodFrames |
                    ETH_InitStruct->ETH_ReceiveThresholdControl |
                    ETH_InitStruct->ETH_SecondFrameOperate);
    ETH->DMAOMR = (uint32_t)tmpreg;

    ETH->DMABMR = (uint32_t)(ETH_InitStruct->ETH_AddressAlignedBeats |
                            ETH_InitStruct->ETH_FixedBurst |
                            ETH_InitStruct->ETH_RxDMABurstLength | /* !! if 4xPBL is selected for Tx or Rx it is applied for the other */
                            ETH_InitStruct->ETH_TxDMABurstLength |
                           (ETH_InitStruct->ETH_DescriptorSkipLength << 2) |
                            ETH_InitStruct->ETH_DMAArbitration |
                            ETH_DMABMR_USP);

    /* Reset the physical layer */
    ETH_WritePHYRegister(PHYAddress, PHY_BCR, PHY_Reset);
    ETH_WritePHYRegister(PHYAddress, PHY_MDIX, PHY_PN_SWITCH_AUTO);
    return ETH_SUCCESS;
}

/*********************************************************************
 * @fn      ETH_Configuration
 *
 * @brief   Ethernet configure.
 *
 * @return  none
 */
void ETH_Configuration( uint8_t *macAddr )
{
    printf("ETH_Configuration( uint8_t *macAddr )");
    ETH_InitTypeDef ETH_InitStructure;
    uint16_t timeout = 10000;

    /* Enable Ethernet MAC clock */
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_ETH_MAC | \
                          RCC_AHBPeriph_ETH_MAC_Tx | \
                          RCC_AHBPeriph_ETH_MAC_Rx, ENABLE);

    gPHYAddress = PHY_ADDRESS;
    ETH_SetClock( );

    /* Enable internal 10BASE-T PHY*/
    EXTEN->EXTEN_CTR |= EXTEN_ETH_10M_EN;    /* Enable 10M Ethernet physical layer   */

    /* Reset ETHERNET on AHB Bus */
    ETH_DeInit();

    /* Software reset */
    ETH_SoftwareReset();

    /* Wait for software reset */
    do{
        delay_us(10);
        if( !--timeout )  break;
    }while(ETH->DMABMR & ETH_DMABMR_SR);

    /* ETHERNET Configuration */
    /* Call ETH_StructInit if you don't like to configure all ETH_InitStructure parameter */
    ETH_StructInit(&ETH_InitStructure);
    /* Fill ETH_InitStructure parameters */
    /*------------------------   MAC   -----------------------------------*/
    ETH_InitStructure.ETH_Mode = ETH_Mode_FullDuplex;
    ETH_InitStructure.ETH_Speed = ETH_Speed_10M;
#ifdef CHECKSUM_BY_HARDWARE
    ETH_InitStructure.ETH_ChecksumOffload = ETH_ChecksumOffload_Enable;
#endif
    ETH_InitStructure.ETH_AutoNegotiation = ETH_AutoNegotiation_Enable;
    ETH_InitStructure.ETH_LoopbackMode = ETH_LoopbackMode_Disable;
    ETH_InitStructure.ETH_RetryTransmission = ETH_RetryTransmission_Disable;
    ETH_InitStructure.ETH_AutomaticPadCRCStrip = ETH_AutomaticPadCRCStrip_Disable;
    /* Filter function configuration */
    ETH_InitStructure.ETH_ReceiveAll = ETH_ReceiveAll_Disable;
    ETH_InitStructure.ETH_PromiscuousMode = ETH_PromiscuousMode_Disable;
    ETH_InitStructure.ETH_BroadcastFramesReception = ETH_BroadcastFramesReception_Enable;
    ETH_InitStructure.ETH_MulticastFramesFilter = ETH_MulticastFramesFilter_Perfect;
    ETH_InitStructure.ETH_UnicastFramesFilter = ETH_UnicastFramesFilter_Perfect;
    /*------------------------   DMA   -----------------------------------*/
    /* When we use the Checksum offload feature, we need to enable the Store and Forward mode:
    the store and forward guarantee that a whole frame is stored in the FIFO, so the MAC can insert/verify the checksum,
    if the checksum is OK the DMA can handle the frame otherwise the frame is dropped 
    ETH_InitStructure.ETH_DropTCPIPChecksumErrorFrame = ETH_DropTCPIPChecksumErrorFrame_Enable;
    ETH_InitStructure.ETH_TransmitStoreForward = ETH_TransmitStoreForward_Enable;
    ETH_InitStructure.ETH_ForwardErrorFrames = ETH_ForwardErrorFrames_Enable;
    ETH_InitStructure.ETH_ForwardUndersizedGoodFrames = ETH_ForwardUndersizedGoodFrames_Enable;
    
    
    /*------------------------   DMA   -----------------------------------*/
    /* When we use the Checksum offload feature, we need to enable the Store and Forward mode:
    the store and forward guarantee that a whole frame is stored in the FIFO, so the MAC can insert/verify the checksum,
    if the checksum is OK the DMA can handle the frame otherwise the frame is dropped */
    ETH_InitStructure.ETH_DropTCPIPChecksumErrorFrame = ETH_DropTCPIPChecksumErrorFrame_Enable;
    ETH_InitStructure.ETH_ReceiveStoreForward = ETH_ReceiveStoreForward_Enable;
    ETH_InitStructure.ETH_TransmitStoreForward = ETH_TransmitStoreForward_Enable;
    ETH_InitStructure.ETH_ForwardErrorFrames = ETH_ForwardErrorFrames_Enable;
    ETH_InitStructure.ETH_ForwardUndersizedGoodFrames = ETH_ForwardUndersizedGoodFrames_Enable;
    ETH_InitStructure.ETH_SecondFrameOperate = ETH_SecondFrameOperate_Disable;
    ETH_InitStructure.ETH_AddressAlignedBeats = ETH_AddressAlignedBeats_Enable;
    ETH_InitStructure.ETH_FixedBurst = ETH_FixedBurst_Enable;
    ETH_InitStructure.ETH_RxDMABurstLength = ETH_RxDMABurstLength_32Beat;
    ETH_InitStructure.ETH_TxDMABurstLength = ETH_TxDMABurstLength_32Beat;
    ETH_InitStructure.ETH_DMAArbitration = ETH_DMAArbitration_RoundRobin_RxTx_2_1;
    /* Configure Ethernet */
    ETH_RegInit( &ETH_InitStructure, gPHYAddress );

    /* Configure MAC address */
    ETH->MACA0HR = (uint32_t)((macAddr[5]<<8) | macAddr[4]);
    ETH->MACA0LR = (uint32_t)(macAddr[0] | (macAddr[1]<<8) | (macAddr[2]<<16) | (macAddr[3]<<24));

    /* Mask the interrupt that Tx good frame count counter reaches half the maximum value */
    ETH->MMCTIMR = ETH_MMCTIMR_TGFM;
    /* Mask the interrupt that Rx good unicast frames counter reaches half the maximum value */
    /* Mask the interrupt that Rx crc error counter reaches half the maximum value */
    ETH->MMCRIMR = ETH_MMCRIMR_RGUFM | ETH_MMCRIMR_RFCEM;

    ETH_DMAITConfig(ETH_DMA_IT_NIS |
                ETH_DMA_IT_R |
                ETH_DMA_IT_T |
                ETH_DMA_IT_AIS |
                ETH_DMA_IT_RBU |
                //ETH_DMA_IT_TBU |
                ETH_DMA_IT_PHYLINK,
                ENABLE);
    /*ETH_DMAITConfig(ETH_DMA_IT_NIS |
                ETH_DMA_IT_R |
                ETH_DMA_IT_T ,
                ENABLE);*/
}

/*********************************************************************
 * @fn      ETH_TxPktChainMode
 *
 * @brief   Ethernet sends data frames in chain mode.
 *
 * @param   len     Send data length
 *          pBuff   send buffer pointer
 *
 * @return  Send status.
 */
uint32_t ETH_TxPktChainMode(uint16_t len, uint8_t *pBuff )
{
    uint32_t offset = 0;

    if((DMATxDescToSet->Status & ETH_DMATxDesc_OWN) != (uint32_t)RESET)
    {
        return ETH_ERROR;
    }

    for(offset = 0; offset < len; offset++)
    {
        (*(__IO uint8_t *)((DMATxDescToSet->Buffer1Addr) + offset)) = (*(pBuff + offset));
    }

    DMATxDescToSet->ControlBufferSize = (len & ETH_DMATxDesc_TBS1);
    DMATxDescToSet->Status |= ETH_DMATxDesc_LS | ETH_DMATxDesc_FS | ETH_DMATxDesc_CIC_TCPUDPICMP_Full;
    
    DMATxDescToSet->Status |= ETH_DMATxDesc_OWN;
    if((ETH->DMASR & ETH_DMASR_TBUS) != (uint32_t)RESET)
    {
        //printf("if((ETH->DMASR & ETH_DMASR_TBUS) != (uint32_t)RESET)");
        ETH->DMASR = ETH_DMASR_TBUS;
        ETH->DMATPDR = 0;
    }

    if((DMATxDescToSet->Status & ETH_DMATxDesc_TCH) != (uint32_t)RESET)
    {
        //printf("if((DMATxDescToSet->Status & ETH_DMATxDesc_TCH) != (uint32_t)RESET)");
        DMATxDescToSet = (ETH_DMADESCTypeDef *)(DMATxDescToSet->Buffer2NextDescAddr);
    }
    else
    {
        if((DMATxDescToSet->Status & ETH_DMATxDesc_TER) != (uint32_t)RESET)
        {
            //printf("if((DMATxDescToSet->Status & ETH_DMATxDesc_TER) != (uint32_t)RESET))");
            DMATxDescToSet = (ETH_DMADESCTypeDef *)(ETH->DMATDLAR);
        }
        else
        {   
            //printf("else");

            DMATxDescToSet = (ETH_DMADESCTypeDef *)((uint32_t)DMATxDescToSet + 0x10 + ((ETH->DMABMR & ETH_DMABMR_DSL) >> 2));
        }
    }

    //DMATxDescToSet->Status |= ETH_DMATxDesc_OWN;
    //ETH->DMATPDR = 0;

    return ETH_SUCCESS;
}

/*********************************************************************
 * @fn      WCHNET_ETHIsr
 *
 * @brief   Ethernet Interrupt Service Routine
 *
 * @return  none
 */
void WCHNET_ETHIsr(void)
{
    printf("ETH_Configuration( uint8_t *macAddr )");
    uint32_t int_sta;

    int_sta = ETH->DMASR;
    if (int_sta & ETH_DMA_IT_AIS)
    {
        if (int_sta & ETH_DMA_IT_RBU)
        {
            if((ChipId & 0xf0) == 0x10)
            {
                ((ETH_DMADESCTypeDef *)(((ETH_DMADESCTypeDef *)(ETH->DMACHRDR))->Buffer2NextDescAddr))->Status = ETH_DMARxDesc_OWN;

                /* Resume DMA reception */
                ETH->DMARPDR = 0;
            }
            ETH_DMAClearITPendingBit(ETH_DMA_IT_RBU);
        }
        ETH_DMAClearITPendingBit(ETH_DMA_IT_AIS);
    }

    if( int_sta & ETH_DMA_IT_NIS )
    {
        if( int_sta & ETH_DMA_IT_R )
        {
            /*If you don't use the Ethernet library,
             * you can do some data processing operations here*/
            ETH_DMAClearITPendingBit(ETH_DMA_IT_R);
        }
        if( int_sta & ETH_DMA_IT_T )
        {
            ETH_DMAClearITPendingBit(ETH_DMA_IT_T);
        }
        if( int_sta & ETH_DMA_IT_PHYLINK)
        {
            ETH_PHYLink( );
            ETH_DMAClearITPendingBit(ETH_DMA_IT_PHYLINK);
        }
        ETH_DMAClearITPendingBit(ETH_DMA_IT_NIS);
    }
}

/*********************************************************************
 * @fn      ETH_Init
 *
 * @brief   Ethernet initialization.
 *
 * @return  none
 */
void ETH_Init( uint8_t *macAddr )
{
    printf("ETH_Init");
    delay_ms(100);
    ChipId = DBGMCU_GetDEVID();
    ETH_LedConfiguration( );

    RandVal = (macAddr[3]^macAddr[4]^macAddr[5]) * 214017 + 2531017;

    ETH_Configuration(macAddr);
    
    ETH_DMATxDescChainInit(DMATxDscrTab, Tx_Buff, ETH_TXBUFNB);
    ETH_DMARxDescChainInit(DMARxDscrTab, Rx_Buff, ETH_RXBUFNB);

    pDMARxSet = DMARxDscrTab;
    pDMATxSet = DMATxDscrTab;

    NVIC_SetPriority(ETH_IRQn, 14);
    NVIC_EnableIRQ(ETH_IRQn);
}


