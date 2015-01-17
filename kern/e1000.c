#include <kern/e1000.h>
#include <kern/pmap.h>
#include <inc/string.h>
#include <inc/stdio.h>
#include <inc/assert.h>
#include <inc/error.h>

// LAB 6: Your driver code here
// register must store physical address
volatile uint32_t *e1000; // register
struct tx_desc tx_desc_array[E1000_TXDESC] __attribute__ ((aligned (16)));
struct tx_pkt tx_pkt_bufs[E1000_TXDESC]; 
struct rx_desc rx_desc_array[E1000_RXDESC] __attribute__ ((aligned (16)));
struct rx_pkt rx_pkt_bufs[E1000_RXDESC];

static void tx_init(void);

int e1000_attach(struct pci_func *f) {

    int i;

    pci_func_enable(f);

    // Map memory for PCI I/O
    e1000 = (uint32_t *) mmio_map_region(f->reg_base[0], f->reg_size[0]);

    cprintf("the register status is %08x\n", e1000[E1000_STATUS] );

    // Initialize tx_desc
    tx_init();
    
    // initialize rx_desc
    memset(rx_desc_array, 0, sizeof(struct rx_desc) * E1000_RXDESC);
    memset(rx_pkt_bufs, 0, sizeof(struct rx_pkt) * E1000_RXDESC);
    for (i = 0; i < E1000_RXDESC; i++) {
        rx_desc_array[i].buffer_addr = PADDR(rx_pkt_bufs[i].buf);
    }

    /* Receive initialization */
    e1000[E1000_RAH] = 0x00005634;
    e1000[E1000_RAL] = 0x12005452;
    e1000[E1000_RAH] |= E1000_RAH_AV;

    e1000[E1000_MTA] = 0x0;

    e1000[E1000_RDBAL] = PADDR(rx_desc_array); 
    e1000[E1000_RDBAH] = 0x0;
    e1000[E1000_RDLEN] = sizeof(struct rx_desc) * E1000_RXDESC;
    
    e1000[E1000_RDH] = 0x0;
    e1000[E1000_RDT] = 0x0;

    e1000[E1000_RCTL] |= E1000_RCTL_EN;
    e1000[E1000_RCTL] &= ~E1000_RCTL_LPEMASK; // long packet disabled
    e1000[E1000_RCTL] &= ~E1000_RCTL_LBMMASK; // no loop back
    e1000[E1000_RCTL] &= ~E1000_RCTL_RDMTSMASK; // RDMT half
    e1000[E1000_RCTL] &= ~E1000_RCTL_MO; // unknown func, use default
    e1000[E1000_RCTL] |= E1000_RCTL_BAM;
    e1000[E1000_RCTL] &= ~E1000_RCTL_BSIZEMASK;
    e1000[E1000_RCTL] |= E1000_RCTL_SECRC;

    return 0;
}

static void tx_init() {

    int i;

    memset(tx_desc_array, 0, sizeof(struct tx_desc) * E1000_TXDESC);
    memset(tx_pkt_bufs, 0, sizeof(struct tx_pkt) * E1000_TXDESC);
    for (i = 0; i < E1000_TXDESC; i++) {
        tx_desc_array[i].addr = PADDR(tx_pkt_bufs[i].buf);
        tx_desc_array[i].status |= E1000_TXD_STAT_DD;
    }

    /* Transmit initialization */
    // Base address registers
    e1000[E1000_TDBAL] = PADDR(tx_desc_array);
    e1000[E1000_TDBAH] = 0x0;

    // Set transmit descriptor length
    e1000[E1000_TDLEN] = sizeof(struct tx_desc) * E1000_TXDESC;

    // Set Transmit descriptor head and tail (0b means 0x0 not 0xb!)
    e1000[E1000_TDH] = 0x0;
    e1000[E1000_TDT] = 0x0;

    // Set TCTL Transmit control register
    e1000[E1000_TCTL] |= E1000_TCTL_EN;
    e1000[E1000_TCTL] |= E1000_TCTL_PSP;
    e1000[E1000_TCTL] &= ~E1000_TCTL_CT;
    e1000[E1000_TCTL] |= (0x10 << 4);
    e1000[E1000_TCTL] &= ~E1000_TCTL_COLD;
    e1000[E1000_TCTL] |= (0x40 << 12);

    // Transmit IPG register
    e1000[E1000_TIPG] = 0x0;
    e1000[E1000_TIPG] |= (0x6) << 20; // IPGR2
    e1000[E1000_TIPG] |= (0x4) << 10; // IPGR1
    e1000[E1000_TIPG] |= 0xA; // IPGT
}

int e1000_transmit(char *data, int len) {
    if (len > TX_PKT_SIZE) {
        return -E_PKT_OVERFLOW;
    }

    // tdt of current pkt
    uint32_t tdt = e1000[E1000_TDT];
    // ready to send next pkt
    if (tx_desc_array[tdt].status & E1000_TXD_STAT_DD) {
        memmove(tx_pkt_bufs[tdt].buf, data, len);
        tx_desc_array[tdt].length = len;

        tx_desc_array[tdt].status &= ~E1000_TXD_STAT_DD;
        // output 0x9000002a = E1000_TXD_CMD_RS | E1000_TXD_CMD_EOP
        tx_desc_array[tdt].cmd |= E1000_TXD_CMD_RS;
        tx_desc_array[tdt].cmd |= E1000_TXD_CMD_EOP;
 
        e1000[E1000_TDT] = (tdt + 1) % E1000_TXDESC;
    }
    else {
        return -E_TX_FULL;
    }

    return 0;   
}

// if succeed return data and length otherwise return E_RECV_FAIL
int e1000_receive(char *data) {

    uint32_t rdt, len;
    rdt = e1000[E1000_RDT];

    if (rx_desc_array[rdt].status & E1000_RXD_STAT_DD) {
        len = rx_desc_array[rdt].length;

        memmove(data, rx_pkt_bufs[rdt].buf, len);
        // clear state
        rx_desc_array[rdt].status &= ~E1000_RXD_STAT_DD; 
        rx_desc_array[rdt].status &= ~E1000_RXD_STAT_EOP;
        e1000[E1000_RDT] = (rdt + 1) % E1000_RXDESC;
      
        return len;
    }
    
    return -E_RECV_FAIL;
}

