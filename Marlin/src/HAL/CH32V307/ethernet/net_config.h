#ifndef NET_CONFIG_H
#define NET_CONFIG_H

typedef struct
{
    uint8_t ip[4];
    uint8_t mask[4];
    uint8_t gw[4];
    uint8_t mac[6];
} netconfig;


#endif /*NET_CONFIG_H*/