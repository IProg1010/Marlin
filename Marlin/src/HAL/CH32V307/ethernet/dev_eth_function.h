#ifndef DEV_ETH_FUNCTION
#define DEV_ETH_FUNCTION

#include "debug.h"
#include "lwipboardconfig.h"
#include "string.h"
#include <lwip/opt.h>
//#include <lwip/arch.h>
#include "lwip/init.h"
#include "lwip/netif.h"
//#include "lwip/dhcp.h"
#include "ethernetif.h"
#include "netif/ethernet.h"
//#include "ethernetif.h"
#include "lwip/def.h"
#include "lwip/timeouts.h"
#include "ch32v30x_rng.h"
#include "ch32v30x_eth.h"
#include "list.h"
#include "memb.h"
#include "net_config.h"

#define ETH_RXBUFNB        2
#define ETH_TXBUFNB        2
#define ETH_RX_BUF_SZE     1524

typedef struct
{
    void *next;
    u32 length;
    u8* buffer;
    ETH_DMADESCTypeDef *descriptor;
} FrameTypeDef;

LIST_EXTERN(ch307_mac_rec);
MEMB_EXTERN(ch307_mac_rec_frame_mem);

extern ETH_DMADESCTypeDef  *DMATxDescToSet;
extern ETH_DMADESCTypeDef  *DMARxDescToGet;

extern ETH_DMADESCTypeDef DMARxDscrTab[ETH_RXBUFNB];
extern ETH_DMADESCTypeDef DMATxDscrTab[ETH_TXBUFNB];

#ifdef __cplusplus
extern "C" {
#endif

uint32_t ETH_TxPkt_ChainMode(uint16_t FrameLength);
void mac_send(uint8_t * content_ptr, uint16_t content_len);
void* ETH_RxPkt_ChainMode(void);
void PHY_control_pin_init(void);
void GETH_pin_init(void);
void FETH_pin_init(void);

void lwip_initialize(void);
void lwip_reinitialize(netconfig* net_cfg);
void lwip_loop(void);
void init_phy_embed10M(void);
extern void lwip_init_success_callback(ip_addr_t *ip);
uint32_t getTime();
uint16_t getPointer();
void WCHNET_GetMacAddr( uint8_t *p );

extern volatile uint8_t net_data_led_require;
#define NET_LED_PERIOD_MSECS 100
void net_led_tmr(void);

void set_net_config(netconfig* net_cfg);


#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
 extern "C" {
#endif 

 /* 1: interrupt 0: polling in RMII or RGMII mode */
#define LINK_STAT_ACQUISITION_METHOD            0

#define PHY_ADDRESS                             1

#define ETH_DMARxDesc_FrameLengthShift          16

#define ROM_CFG_USERADR_ID                      0x1FFFF7E8

#define PHY_LINK_TASK_PERIOD                    50

#define PHY_ANLPAR_SELECTOR_FIELD               0x1F
#define PHY_ANLPAR_SELECTOR_VALUE               0x01       /* 5B'00001 */

#define PHY_LINK_INIT                           0x00
#define PHY_LINK_SUC_P                          (1<<0)
#define PHY_LINK_SUC_N                          (1<<1)
#define PHY_LINK_WAIT_SUC                       (1<<7)

#define PHY_PN_SWITCH_P                         (0<<2)
#define PHY_PN_SWITCH_N                         (1<<2)
#define PHY_PN_SWITCH_AUTO                      (2<<2)

#ifndef WCHNETTIMERPERIOD
#define WCHNETTIMERPERIOD                       10   /* Timer period, in Ms. */
#endif

#define QUERY_STAT_FLAG  ((LastQueryPhyTime == (LocalTime / 1000)) ? 0 : 1)

#define ACCELERATE_LINK_PROCESS() do{\
    if((TRDetectStep < 2) && (ETH_ReadPHYRegister(gPHYAddress, PHY_ANLPAR) & PHY_ANLPAR_SELECTOR_FIELD))\
        LinkTaskPeriod = 0;\
}while(0)

#define UPDATE_LINKTASKPERIOD() do{\
    if(TRDetectStep == 1)\
    {\
        RandVal = RandVal * 214017 + 2531017;\
        LinkTaskPeriod = RandVal%100 + 50;\
    }\
    else {\
        LinkTaskPeriod = 50;\
    }\
}while(0)

#define PHY_RESTART_AUTONEGOTIATION()       do{\
    RegVal = ETH_ReadPHYRegister(gPHYAddress, PHY_BCR);\
    RegVal &= ~0x01;\
    RegVal |= PHY_Restart_AutoNegotiation;\
    ETH_WritePHYRegister( gPHYAddress, PHY_BCR, RegVal);\
    RegVal = ETH_ReadPHYRegister(gPHYAddress, PHY_BCR);\
    RegVal |= 0x03 | PHY_Restart_AutoNegotiation;\
    ETH_WritePHYRegister( gPHYAddress, PHY_BCR, RegVal);\
}while(0)

#define PHY_TR_SWITCH()    do{\
    phy_mdix = ETH_ReadPHYRegister(gPHYAddress, PHY_MDIX);\
    if(phy_mdix & 0x01)\
    {\
        phy_mdix &= ~0x03;\
        phy_mdix |= 1 << 1;\
    }\
    else\
    {\
        phy_mdix &= ~0x03;\
        phy_mdix |= 1 << 0;\
    }\
    ETH_WritePHYRegister(gPHYAddress, PHY_MDIX, phy_mdix);\
    PHY_RESTART_AUTONEGOTIATION();\
}while(0)

#define PHY_TR_REVERSE()       do{\
    if(phyStatus)\
    {\
        RegVal = ETH_ReadPHYRegister(gPHYAddress, PHY_MDIX);\
        if(RegVal & 0x01)\
        {\
            RegVal &= ~0x03;\
            RegVal |= 1 << 1;\
        }\
        else{\
            RegVal &= ~0x03;\
            RegVal |= 1 << 0;\
        }\
        ETH_WritePHYRegister(gPHYAddress, PHY_MDIX, RegVal);\
    }\
}while(0)

#define PHY_PN_SWITCH(PNMode)   do{\
    if(PNMode == PHY_PN_SWITCH_AUTO)\
    {\
         phyPN = PHY_PN_SWITCH_AUTO;\
    }\
    else{\
        phyPN = (ETH_ReadPHYRegister(gPHYAddress, PHY_MDIX) & (~(0x03 << 2))) | PNMode;\
    }\
    ETH_WritePHYRegister(gPHYAddress, PHY_MDIX, phyPN);\
    phyPN = PNMode;\
    PHY_RESTART_AUTONEGOTIATION();\
}while(0)

#define PHY_NEGOTIATION_PARAM_INIT()    do{\
    phyStatus = 0;\
    phySucCnt = 0;\
    phyLinkCnt = 0;\
    TRDetectStep = 0;\
    PhyPolarityDetect = 0;\
    phyLinkStatus = PHY_LINK_INIT;\
    phyPN = PHY_PN_SWITCH_AUTO;\
    ETH_WritePHYRegister(gPHYAddress, PHY_MDIX, phyPN);\
}while(0)

#define PHY_LINK_RESET()       do{\
    ETH_WritePHYRegister(gPHYAddress, PHY_BCR, PHY_Reset);\
    PHY_NEGOTIATION_PARAM_INIT();\
}while(0)

//extern SOCK_INF SocketInf[ ];

void ETH_PHYLink( void );
void WCHNET_ETHIsr( void );
void WCHNET_MainTask( void );
void ETH_LedConfiguration(void);
void ETH_Init( uint8_t *macAddr );
void ETH_LedLinkSet( uint8_t mode );
void ETH_LedDataSet( uint8_t mode );
void WCHNET_TimeIsr( uint16_t timperiod );
void ETH_Configuration( uint8_t *macAddr );
uint32_t ETH_TxPktChainMode(uint16_t len, uint8_t *pBuff );
//uint8_t ETH_LibInit( uint8_t *ip, uint8_t *gwip, uint8_t *mask, uint8_t *macaddr);

#ifdef __cplusplus
}
#endif


#endif
