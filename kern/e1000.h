#ifndef JOS_KERN_E1000_H
#define JOS_KERN_E1000_H

#include <kern/pci.h>

#define E1000_MMIOADDR KSTACKTOP
#define E1000_VENDORID 0x8086
#define E1000_DEVICEID 0x100E

#define E1000_STATUS 0x0008 / 4

// Transmit def
#define E1000_TXDESC 64
#define TX_PKT_SIZE 1518

#define E1000_TDBAL 0x03800 / 4
#define E1000_TDBAH 0x03804 / 4
#define E1000_TDLEN 0x03808 / 4
#define E1000_TDH   0x03810 / 4
#define E1000_TDT   0x03818 / 4

#define E1000_TCTL 0x00400 / 4
#define E1000_TCTL_EN   0x00000002
#define E1000_TCTL_PSP  0x00000008
#define E1000_TCTL_CT   0x00000ff0
#define E1000_TCTL_COLD 0x003ff000

#define E1000_TIPG 0x00410 / 4

#define E1000_TXD_STAT_DD 0x00000001

#define E1000_TXD_CMD_EOP 0x01
#define E1000_TXD_CMD_RS  0x08

// Receive def
#define E1000_RXDESC 128
#define RX_PKT_SIZE 2048

#define E1000_EERD 0x00014 / 4
#define E1000_RAL 0x05400 / 4
#define E1000_RAH 0x05404 / 4
#define E1000_RAH_AV    0x80000000

#define E1000_MTA 0x05200 / 4

#define E1000_RDBAL 0x02800 / 4
#define E1000_RDBAH 0x02804 / 4
#define E1000_RDLEN 0x02808 / 4

#define E1000_RDH   0x02810 / 4
#define E1000_RDT   0x02818 / 4

#define E1000_RCTL 0x00100 / 4
#define E1000_RCTL_EN         0x00000002
#define E1000_RCTL_LPEMASK    0x00000020
#define E1000_RCTL_LBMMASK    0x000000C0
#define E1000_RCTL_RDMTSMASK  0x00000300
#define E1000_RCTL_MO         0x00003000
#define E1000_RCTL_BAM        0x00008000
#define E1000_RCTL_BSIZEMASK  0x00030000 // 2048 byte buffer
#define E1000_RCTL_SECRC      0x04000000

#define E1000_RXD_STAT_DD    0x01
#define E1000_RXD_STAT_EOP   0x02

int e1000_attach(struct pci_func *f);
int e1000_transmit(char *data, int len);
int e1000_receive(char *data);

// Definition of tx_desc from lab6
struct tx_desc {
    uint64_t addr;
    uint16_t length;
    uint8_t cso;
    uint8_t cmd;
    uint8_t status;
    uint8_t css;
    uint16_t special;
} __attribute__((packed));

struct tx_pkt {
    uint8_t buf[TX_PKT_SIZE];
} __attribute__((packed));

// Definition of recv_desc
struct rx_desc {
    uint64_t buffer_addr;
    uint16_t length;
    uint16_t csum;
    uint8_t status;
    uint8_t errors;
    uint16_t special;
} __attribute__((packed));

struct rx_pkt {
    uint8_t buf[RX_PKT_SIZE];
} __attribute__((packed));


#endif	// JOS_KERN_E1000_H
