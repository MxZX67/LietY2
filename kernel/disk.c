#include "kernel.h"

#define ATA_DATA 0x1F0
#define ATA_SECCOUNT 0x1F2
#define ATA_LBA0 0x1F3
#define ATA_LBA1 0x1F4
#define ATA_LBA2 0x1F5
#define ATA_DRIVE 0x1F6
#define ATA_STATUS 0x1F7
#define ATA_CMD 0x1F7

#define PCI_ADDR 0xCF8
#define PCI_DATA 0xCFC

#define AHCI_GHC 0x04
#define AHCI_PI  0x0C
#define AHCI_PORT_BASE(n) (0x100 + (n) * 0x80)
#define P_CLB 0x00
#define P_FB  0x08
#define P_CMD 0x18
#define P_TFD 0x20
#define P_SIG 0x24
#define P_SSTS 0x28
#define P_SERR 0x30
#define P_CI 0x38

#define CMD_ST (1u << 0)
#define CMD_FRE (1u << 4)
#define CMD_CR (1u << 15)
#define CMD_FR (1u << 14)

struct ahci_cmd_header {
    uint16_t flags;
    uint16_t prdtl;
    uint32_t prdbc;
    uint32_t ctba;
    uint32_t ctbau;
    uint32_t reserved[4];
} __attribute__((packed));

struct ahci_prdt {
    uint32_t dba;
    uint32_t dbau;
    uint32_t reserved;
    uint32_t dbc_i;
} __attribute__((packed));

struct ahci_cmd_table {
    uint8_t cfis[64];
    uint8_t acmd[16];
    uint8_t reserved[48];
    struct ahci_prdt prdt[1];
} __attribute__((packed,aligned(128)));

static uintptr_t ahci_base;
static uint32_t ahci_port;
static int backend;
static struct ahci_cmd_header ahci_cl[32] __attribute__((aligned(1024)));
static uint8_t ahci_fis[256] __attribute__((aligned(256)));
static struct ahci_cmd_table ahci_ct[256] __attribute__((aligned(128)));

static inline void outb(uint16_t p,uint8_t v){__asm__ volatile("outb %0,%1"::"a"(v),"Nd"(p));}
static inline uint8_t inb(uint16_t p){uint8_t v;__asm__ volatile("inb %1,%0":"=a"(v):"Nd"(p));return v;}
static inline void outw(uint16_t p,uint16_t v){__asm__ volatile("outw %0,%1"::"a"(v),"Nd"(p));}
static inline uint16_t inw(uint16_t p){uint16_t v;__asm__ volatile("inw %1,%0":"=a"(v):"Nd"(p));return v;}
static inline void outl(uint16_t p,uint32_t v){__asm__ volatile("outl %0,%1"::"a"(v),"Nd"(p));}
static inline uint32_t inl(uint16_t p){uint32_t v;__asm__ volatile("inl %1,%0":"=a"(v):"Nd"(p));return v;}

static uint32_t pci_read32(uint8_t bus,uint8_t dev,uint8_t fn,uint8_t off){
    uint32_t a=0x80000000u|((uint32_t)bus<<16)|((uint32_t)dev<<11)|((uint32_t)fn<<8)|(off&0xFC);
    outl(PCI_ADDR,a); return inl(PCI_DATA);
}
static void pci_write32(uint8_t bus,uint8_t dev,uint8_t fn,uint8_t off,uint32_t v){
    uint32_t a=0x80000000u|((uint32_t)bus<<16)|((uint32_t)dev<<11)|((uint32_t)fn<<8)|(off&0xFC);
    outl(PCI_ADDR,a); outl(PCI_DATA,v);
}
static int ahci_find(void){
    for(uint32_t bus=0;bus<256;bus++) for(uint32_t dev=0;dev<32;dev++) for(uint32_t fn=0;fn<8;fn++){
        uint32_t id=pci_read32((uint8_t)bus,(uint8_t)dev,(uint8_t)fn,0);
        if(id==0xFFFFFFFFu) continue;
        uint32_t cc=pci_read32((uint8_t)bus,(uint8_t)dev,(uint8_t)fn,8);
        uint8_t class=(uint8_t)(cc>>24),sub=(uint8_t)(cc>>16),prog=(uint8_t)(cc>>8);
        if(class!=0x01||sub!=0x06||prog!=0x01) continue;

        uint32_t bar5=pci_read32((uint8_t)bus,(uint8_t)dev,(uint8_t)fn,0x24);
        if((bar5&1)||bar5==0) continue;
        uint64_t bar=(uint64_t)(bar5&~0xFu);
        if((bar5&0x6)==0x4) bar|=((uint64_t)pci_read32((uint8_t)bus,(uint8_t)dev,(uint8_t)fn,0x28)<<32);
        if(!bar) continue;

        uint32_t cmd=pci_read32((uint8_t)bus,(uint8_t)dev,(uint8_t)fn,0x04);
        if(!(cmd&0x2)||!(cmd&0x4)) pci_write32((uint8_t)bus,(uint8_t)dev,(uint8_t)fn,0x04,cmd|0x6);

        ahci_base=(uintptr_t)bar;
        volatile uint32_t *ghc=(volatile uint32_t*)(ahci_base+AHCI_GHC);
        *ghc|=(1u<<31);
        uint32_t pi=*(volatile uint32_t*)(ahci_base+AHCI_PI);

        for(uint32_t p=0;p<32;p++) if(pi&(1u<<p)){
            volatile uint32_t *pr=(volatile uint32_t*)(ahci_base+AHCI_PORT_BASE(p));
            uint32_t ssts=pr[P_SSTS/4];
            uint32_t sig=pr[P_SIG/4];
            if((ssts&0xF)==3&&sig==0x00000101u){ahci_port=p;return 1;}
        }
    }
    return 0;
}

static int ahci_start(void){
    volatile uint32_t *pr=(volatile uint32_t*)(ahci_base+AHCI_PORT_BASE(ahci_port));
    uint32_t cmd=pr[P_CMD/4];
    cmd&=~(CMD_ST|CMD_FRE);
    pr[P_CMD/4]=cmd;
    for(uint32_t i=0;i<100000&&(pr[P_CMD/4]&(CMD_CR|CMD_FR));i++)__asm__ volatile("pause");

    for(size_t i=0;i<sizeof(ahci_cl);i++)((uint8_t*)ahci_cl)[i]=0;
    for(size_t i=0;i<sizeof(ahci_fis);i++)ahci_fis[i]=0;
    for(size_t i=0;i<sizeof(ahci_ct);i++)((uint8_t*)ahci_ct)[i]=0;

    pr[P_CLB/4]=(uint32_t)(uintptr_t)ahci_cl;
    pr[(P_CLB+4)/4]=(uint32_t)((uintptr_t)ahci_cl>>32);
    pr[P_FB/4]=(uint32_t)(uintptr_t)ahci_fis;
    pr[(P_FB+4)/4]=(uint32_t)((uintptr_t)ahci_fis>>32);
    pr[P_SERR/4]=0xFFFFFFFFu;
    pr[P_CMD/4]=cmd|CMD_FRE|CMD_ST;
    return 1;
}

static int ahci_io(uint32_t lba,void *buf,int write){
    volatile uint32_t *pr=(volatile uint32_t*)(ahci_base+AHCI_PORT_BASE(ahci_port));
    if(pr[P_TFD/4]&0x88) return 0;

    struct ahci_cmd_table *ct=&ahci_ct[0];
    for(size_t i=0;i<sizeof(*ct);i++)((uint8_t*)ct)[i]=0;
    ct->prdt[0].dba=(uint32_t)(uintptr_t)buf;
    ct->prdt[0].dbau=(uint32_t)((uintptr_t)buf>>32);
    ct->prdt[0].dbc_i=511;

    struct ahci_cmd_header *ch=&ahci_cl[0];
    for(size_t i=0;i<sizeof(*ch);i++)((uint8_t*)ch)[i]=0;
    ch->flags=(uint16_t)(5|(write?(1u<<6):0));
    ch->prdtl=1;
    ch->ctba=(uint32_t)(uintptr_t)ct;
    ch->ctbau=(uint32_t)((uintptr_t)ct>>32);

    uint8_t *fis=ct->cfis;
    fis[0]=0x27; fis[1]=0x80; fis[2]=(uint8_t)(write?0x35:0x25); fis[7]=0x40;
    fis[4]=(uint8_t)lba; fis[5]=(uint8_t)(lba>>8); fis[6]=(uint8_t)(lba>>16);
    fis[8]=(uint8_t)(lba>>24); fis[12]=1; fis[13]=0;

    pr[P_CI/4]=1;
    for(uint32_t spin=0;spin<10000000;spin++){
        uint32_t is=pr[0x10/4];
        if(is&(1u<<30)){pr[0x10/4]=is;return 0;}
        if(!(pr[P_CI/4]&1))return 1;
        if(pr[P_TFD/4]&0x88)return 0;
        __asm__ volatile("pause");
    }
    return 0;
}

static int ata_wait(uint8_t mask,uint8_t val,uint32_t loops){
    while(loops--){uint8_t s=inb(ATA_STATUS);if((s&mask)==val)return 1;__asm__ volatile("pause");}
    return 0;
}
static int ata_prepare(uint32_t lba){
    if(inb(ATA_STATUS)==0xFF)return 0;
    if(!ata_wait(0x80,0,100000))return 0;
    outb(ATA_DRIVE,0xE0|((lba>>24)&0x0F));
    outb(ATA_SECCOUNT,1);outb(ATA_LBA0,(uint8_t)lba);outb(ATA_LBA1,(uint8_t)(lba>>8));outb(ATA_LBA2,(uint8_t)(lba>>16));
    return 1;
}
static int ata_read_sector(uint32_t lba,void *buf){
    if(!ata_prepare(lba))return 0;
    outb(ATA_CMD,0x20);
    if(!ata_wait(0x88,0x08,1000000))return 0;
    uint16_t *d=(uint16_t*)buf;for(int i=0;i<256;i++)d[i]=inw(ATA_DATA);
    return 1;
}
static int ata_write_sector(uint32_t lba,const void *buf){
    if(!ata_prepare(lba))return 0;
    outb(ATA_CMD,0x30);
    if(!ata_wait(0x88,0x08,1000000))return 0;
    const uint16_t *d=(const uint16_t*)buf;for(int i=0;i<256;i++)outw(ATA_DATA,d[i]);
    (void)inb(ATA_STATUS);
    return ata_wait(0x80,0,1000000);
}

int disk_init(void){
    if(backend)return 1;
    if(ahci_find()&&ahci_start()){backend=1;return 1;}
    outb(ATA_DRIVE,0xA0);outb(ATA_SECCOUNT,0);outb(ATA_LBA0,0);outb(ATA_LBA1,0);outb(ATA_LBA2,0);outb(ATA_CMD,0xEC);
    uint8_t s=inb(ATA_STATUS);
    if(s!=0xFF&&s!=0){backend=2;return 1;}
    return 0;
}
int disk_read(uint32_t lba,void *buf){
    if(!disk_init())return 0;
    if(backend==1)return ahci_io(lba,buf,0);
    return backend==2?ata_read_sector(lba,buf):0;
}
int disk_write(uint32_t lba,const void *buf){
    if(!disk_init())return 0;
    if(backend==1)return ahci_io(lba,(void*)buf,1);
    return backend==2?ata_write_sector(lba,buf):0;
}
const char *disk_backend(void){return backend==1?"AHCI":backend==2?"ATA PIO":"NONE";}
