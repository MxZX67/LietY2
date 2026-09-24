#include "kernel.h"

#define FS_MAGIC 0x4C594653u
#define FS_VERSION 1u
#define FS_SECTOR_SIZE 512u
#define FS_BLOCK_SECTORS 8u
#define FS_BLOCK_SIZE 4096u
#define FS_INODES 64u
#define FS_INODE_SECTORS 16u
#define FS_INODE_START 2u
#define FS_BITMAP_SECTOR (FS_INODE_START+FS_INODE_SECTORS)
#define FS_BITMAP_SECTORS 4u
#define FS_DATA_SECTOR (FS_BITMAP_SECTOR+FS_BITMAP_SECTORS)
#define FS_DISK_SECTORS 131072u
#define FS_BLOCKS ((FS_DISK_SECTORS-FS_DATA_SECTOR)/FS_BLOCK_SECTORS)

struct superblock {
    uint32_t magic,version,sector_size,block_sectors,disk_sectors;
    uint32_t inode_count,inode_start,inode_sectors,bitmap_sector,bitmap_sectors,data_sector,block_count;
    uint8_t reserved[512-48];
} __attribute__((packed));

struct inode {
    char name[32];
    uint32_t size,start,blocks;
    uint8_t used,dir;
    uint16_t flags;
    uint8_t reserved[80];
} __attribute__((packed));

static struct superblock sb;
static uint8_t bitmap[FS_BITMAP_SECTORS*FS_SECTOR_SIZE];
static uint8_t sector_buf[512] __attribute__((aligned(16)));
static uint8_t block_buf[FS_BLOCK_SIZE] __attribute__((aligned(16)));
static int mounted;

static int name_eq(const char*a,const char*b){
    for(size_t i=0;i<32;i++){
        char x=a[i],y=b[i];
        if(x>='a'&&x<='z')x=(char)(x-'a'+'A');
        if(y>='a'&&y<='z')y=(char)(y-'a'+'A');
        if(x!=y)return 0;
        if(!x&&!y)return 1;
    }
    return 1;
}
static void name_copy(char*d,const char*s){
    size_t i=0;
    for(;i<31&&s[i];i++)d[i]=s[i];
    d[i]=0;
    while(++i<32)d[i]=0;
}
static uint32_t bit_get(uint32_t b){return (bitmap[b>>3]>>(b&7))&1u;}
static void bit_set(uint32_t b,uint32_t v){if(v)bitmap[b>>3]|=(uint8_t)(1u<<(b&7));else bitmap[b>>3]&=(uint8_t)~(1u<<(b&7));}
static int load_bitmap(void){for(uint32_t i=0;i<FS_BITMAP_SECTORS;i++)if(!disk_read(FS_BITMAP_SECTOR+i,bitmap+i*512))return 0;return 1;}
static int save_bitmap(void){for(uint32_t i=0;i<FS_BITMAP_SECTORS;i++)if(!disk_write(FS_BITMAP_SECTOR+i,bitmap+i*512))return 0;return 1;}
static int inode_io(uint32_t idx,struct inode*in,int wr){
    if(idx>=FS_INODES)return 0;
    uint32_t off=idx*sizeof(struct inode),sec=FS_INODE_START+off/512,pos=off%512;
    if(!disk_read(sec,sector_buf))return 0;
    if(wr){for(size_t i=0;i<sizeof(*in);i++)sector_buf[pos+i]=((uint8_t*)in)[i];return disk_write(sec,sector_buf);}
    for(size_t i=0;i<sizeof(*in);i++)((uint8_t*)in)[i]=sector_buf[pos+i];
    return 1;
}
static int inode_find(const char*name,struct inode*out,uint32_t*outidx){
    for(uint32_t i=0;i<FS_INODES;i++){
        struct inode in;if(!inode_io(i,&in,0))return 0;
        if(in.used&&name_eq(in.name,name)){if(out)*out=in;if(outidx)*outidx=i;return 1;}
    }
    return 0;
}
static int alloc_run(uint32_t need,uint32_t*out){
    if(!need){*out=0;return 1;}
    for(uint32_t s=0;s+need<=sb.block_count;s++){
        uint32_t ok=1;
        for(uint32_t j=0;j<need;j++)if(bit_get(s+j)){ok=0;break;}
        if(ok){for(uint32_t j=0;j<need;j++)bit_set(s+j,1);*out=s;return save_bitmap();}
    }
    return 0;
}

int fs_mount(void){
    mounted=0;
    if(!disk_read(1,&sb))return 0;
    if(sb.magic!=FS_MAGIC||sb.version!=FS_VERSION||sb.sector_size!=512||sb.block_sectors!=8||
       sb.inode_count!=FS_INODES||sb.inode_start!=FS_INODE_START||sb.inode_sectors!=FS_INODE_SECTORS||
       sb.bitmap_sector!=FS_BITMAP_SECTOR||sb.bitmap_sectors!=FS_BITMAP_SECTORS||
       sb.data_sector!=FS_DATA_SECTOR)return 0;
    if(sb.disk_sectors!=FS_DISK_SECTORS||sb.block_count>FS_BLOCKS)return 0;
    if(!load_bitmap())return 0;
    mounted=1;return 1;
}

int fs_format(void){
    if(!disk_init())return 0;
    sb.magic=FS_MAGIC;sb.version=FS_VERSION;sb.sector_size=512;sb.block_sectors=8;sb.disk_sectors=FS_DISK_SECTORS;
    sb.inode_count=FS_INODES;sb.inode_start=FS_INODE_START;sb.inode_sectors=FS_INODE_SECTORS;
    sb.bitmap_sector=FS_BITMAP_SECTOR;sb.bitmap_sectors=FS_BITMAP_SECTORS;sb.data_sector=FS_DATA_SECTOR;sb.block_count=FS_BLOCKS;
    for(size_t i=0;i<sizeof(sb.reserved);i++)sb.reserved[i]=0;
    for(size_t i=0;i<sizeof(bitmap);i++)bitmap[i]=0;
    for(uint32_t s=0;s<FS_INODE_SECTORS;s++){
        for(size_t i=0;i<512;i++)sector_buf[i]=0;
        if(!disk_write(FS_INODE_START+s,sector_buf))return 0;
    }
    if(!save_bitmap()||!disk_write(1,&sb))return 0;
    mounted=1;
    const char *welcome="Welcome to LietY2. Your disk has been professionally confused.\n";
    const char *readme="LYFS v1: real sectors, real inodes, absolutely no sense.\nTry DIR, TYPE WELCOME.TXT, FSINFO, PS and RUNPID.\n";
    return fs_write_file("WELCOME.TXT",welcome,(uint32_t)(sizeof("Welcome to LietY2. Your disk has been professionally confused.\n")-1)) &&
           fs_write_file("README.TXT",readme,(uint32_t)(sizeof("LYFS v1: real sectors, real inodes, absolutely no sense.\nTry DIR, TYPE WELCOME.TXT, FSINFO, PS and RUNPID.\n")-1));
}

int fs_info(uint32_t*files,uint32_t*used,uint32_t*total){
    if(!mounted)return 0;
    uint32_t f=0,u=0;
    for(uint32_t i=0;i<FS_INODES;i++){struct inode in;if(!inode_io(i,&in,0))return 0;if(in.used){f++;u+=in.blocks;}}
    *files=f;*used=u;*total=sb.block_count;return 1;
}
int fs_list(void){
    if(!mounted)return 0;
    puts("\n Volume label: CHAOS\n\n");
    for(uint32_t i=0;i<FS_INODES;i++){
        struct inode in;if(!inode_io(i,&in,0))return 0;
        if(!in.used)continue;
        puts(in.name);puts("  ");
        char n[12];uint32_t v=in.size;int k=0;
        if(!v)n[k++]='0';
        while(v){n[k++]=(char)('0'+v%10);v/=10;}
        for(int j=k-1;j>=0;j--)putc(n[j]);
        puts(" bytes\n");
    }
    puts("\n");return 1;
}
int fs_read_file(const char*name,void*buf,uint32_t cap,uint32_t*out_len){
    if(!mounted)return 0;
    struct inode in;if(!inode_find(name,&in,0))return 0;
    uint32_t left=in.size,done=0;
    for(uint32_t b=0;b<in.blocks&&left;b++){
        for(uint32_t s=0;s<8&&left;s++){
            if(!disk_read(FS_DATA_SECTOR+(in.start+b)*8+s,sector_buf))return 0;
            uint32_t n=left<512?left:512;
            if(done+n>cap)n=cap-done;
            for(uint32_t i=0;i<n;i++)((uint8_t*)buf)[done+i]=sector_buf[i];
            done+=n;left-=n;
            if(done>=cap){left=0;break;}
        }
    }
    if(out_len)*out_len=done;
    return 1;
}
int fs_write_file(const char*name,const void*data,uint32_t len){
    if(!mounted||len>FS_BLOCKS*FS_BLOCK_SIZE)return 0;
    struct inode old;uint32_t oldidx;
    if(inode_find(name,&old,&oldidx)&&!fs_delete(name))return 0;

    struct inode in;
    for(size_t i=0;i<sizeof(in);i++)((uint8_t*)&in)[i]=0;
    name_copy(in.name,name);in.size=len;in.used=1;in.blocks=(len+FS_BLOCK_SIZE-1)/FS_BLOCK_SIZE;

    uint32_t idx=FS_INODES;
    for(uint32_t i=0;i<FS_INODES;i++){struct inode x;if(!inode_io(i,&x,0))return 0;if(!x.used){idx=i;break;}}
    if(idx==FS_INODES)return 0;

    uint32_t start=0;
    if(!alloc_run(in.blocks,&start))return 0;
    in.start=start;

    uint32_t left=len,off=0;
    for(uint32_t b=0;b<in.blocks;b++){
        for(size_t i=0;i<FS_BLOCK_SIZE;i++)block_buf[i]=0;
        uint32_t n=left<FS_BLOCK_SIZE?left:FS_BLOCK_SIZE;
        for(uint32_t i=0;i<n;i++)block_buf[i]=((const uint8_t*)data)[off+i];
        for(uint32_t s=0;s<8;s++){
            if(!disk_write(FS_DATA_SECTOR+(in.start+b)*8+s,block_buf+s*512)){
                for(uint32_t z=0;z<in.blocks;z++)bit_set(in.start+z,0);
                save_bitmap();return 0;
            }
        }
        off+=n;left-=n;
    }
    if(!inode_io(idx,&in,1)){
        for(uint32_t z=0;z<in.blocks;z++)bit_set(in.start+z,0);
        save_bitmap();return 0;
    }
    return 1;
}
int fs_delete(const char*name){
    if(!mounted)return 0;
    struct inode in;uint32_t idx;
    if(!inode_find(name,&in,&idx))return 0;
    for(uint32_t b=0;b<in.blocks;b++)bit_set(in.start+b,0);
    if(!save_bitmap())return 0;
    for(size_t i=0;i<sizeof(in);i++)((uint8_t*)&in)[i]=0;
    return inode_io(idx,&in,1);
}
