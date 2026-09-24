#include "kernel.h"

#define VGA ((volatile uint16_t*)0xB8000)
#define W 80
#define H 25
#define COM1 0x3F8

extern void setup_paging(void);

static size_t row,col;
static uint8_t attr=0x07;

static inline void outb(uint16_t p,uint8_t v){__asm__ volatile("outb %0,%1"::"a"(v),"Nd"(p));}
static inline uint8_t inb(uint16_t p){uint8_t v;__asm__ volatile("inb %1,%0":"=a"(v):"Nd"(p));return v;}
static void delay(volatile uint64_t n){while(n--)__asm__ volatile("pause");}
static size_t strlen_local(const char*s){size_t n=0;while(s[n])n++;return n;}
#define strlen strlen_local

static void serial_init(void){
    outb(COM1+1,0);outb(COM1+3,0x80);outb(COM1+0,3);outb(COM1+1,0);
    outb(COM1+3,3);outb(COM1+2,0xC7);outb(COM1+4,3);
}
static void serial_put(char c){while(!(inb(COM1+5)&0x20));outb(COM1,c);}
void putc(char c){
    serial_put(c);
    if(c=='\n'){col=0;if(row<H-1)row++;return;}
    if(c=='\r'){col=0;return;}
    if(c=='\b'){if(col)col--;VGA[row*W+col]=(attr<<8)|' ';return;}
    VGA[row*W+col]=(attr<<8)|(uint8_t)c;
    if(++col>=W){col=0;if(row<H-1)row++;}
}
void puts(const char*s){while(*s)putc(*s++);}
void clear_screen(void){for(size_t i=0;i<W*H;i++)VGA[i]=(attr<<8)|' ';row=col=0;}
void shell_prompt(void){puts("C:\\>");}

static int ieq(const char*a,const char*b){
    while(*a&&*b){
        char x=*a++,y=*b++;
        if(x>='a'&&x<='z')x=(char)(x-'a'+'A');
        if(y>='a'&&y<='z')y=(char)(y-'a'+'A');
        if(x!=y)return 0;
    }
    return *a==*b;
}
static size_t tok(char*s,char**out){
    size_t n=0;
    while(*s==' ')s++;
    while(*s&&n<7){
        out[n++]=s;
        while(*s&&*s!=' ')s++;
        if(*s){*s++=0;while(*s==' ')s++;}
    }
    return n;
}
static uint8_t bcd(uint8_t x){return (uint8_t)((x&0x0F)+((x>>4)*10));}
static uint8_t cmos(uint8_t r){outb(0x70,(uint8_t)(0x80|r));return inb(0x71);}
static void show_datetime(int date){
    uint8_t st=cmos(0x0B),sec=cmos(0),min=cmos(2),hr=cmos(4);
    uint8_t day=cmos(7),mon=cmos(8),yr=cmos(9);
    if(!(st&4)){sec=bcd(sec);min=bcd(min);hr=bcd(hr);day=bcd(day);mon=bcd(mon);yr=bcd(yr);}
    char n[4];
    if(date){
        puts("20");n[0]=(char)('0'+yr/10);n[1]=(char)('0'+yr%10);n[2]=0;puts(n);putc('-');
        n[0]=(char)('0'+mon/10);n[1]=(char)('0'+mon%10);n[2]=0;puts(n);putc('-');
        n[0]=(char)('0'+day/10);n[1]=(char)('0'+day%10);n[2]=0;puts(n);putc('\n');
    }else{
        n[0]=(char)('0'+hr/10);n[1]=(char)('0'+hr%10);n[2]=0;puts(n);putc(':');
        n[0]=(char)('0'+min/10);n[1]=(char)('0'+min%10);n[2]=0;puts(n);putc(':');
        n[0]=(char)('0'+sec/10);n[1]=(char)('0'+sec%10);n[2]=0;puts(n);putc('\n');
    }
}
static void beep(void){
    uint32_t div=1193182u/440u;
    outb(0x43,0xB6);outb(0x42,(uint8_t)div);outb(0x42,(uint8_t)(div>>8));
    uint8_t v=inb(0x61);outb(0x61,(uint8_t)(v|3));delay(600000);outb(0x61,(uint8_t)(v&~3));
}
static void cpuinfo(void){
    uint32_t a,b,c,d;char v[13];
    __asm__ volatile("cpuid":"=a"(a),"=b"(b),"=c"(c),"=d"(d):"a"(0));
    ((uint32_t*)v)[0]=b;((uint32_t*)v)[1]=d;((uint32_t*)v)[2]=c;v[12]=0;
    puts("CPU: ");puts(v);putc('\n');
}
static void print_num(uint32_t v){
    char n[12];int k=0;if(!v)n[k++]='0';
    while(v){n[k++]=(char)('0'+v%10);v/=10;}
    for(int i=k-1;i>=0;i--)putc(n[i]);
}
static void help(void){
    puts("\nReal: DIR TYPE DEL ERASE MKFILE FSINFO FORMAT C: DISK PS RUNPID CLS ECHO VER MEM DATE TIME CPU BEEP COLOR CALC COPY REN TOUCH CHKDSK REBOOT HALT\n");
    puts("Absurd: FART DUPA KURWA CHUJ IDIOT WHY NOTHING DOOM MOON LASER NUKE HACK SUDO BEER DANCE COFFEE MATRIX SPIN PING PANIC RICKROLL\n");
    puts("The disk is real. The command names are questionable.\n");
}

static void command(char*line){
    char*t[8];size_t nt=tok(line,t);if(!nt)return;

    if(ieq(t[0],"HELP")){help();return;}
    if(ieq(t[0],"VER")||ieq(t[0],"ABOUT")){puts("LietY2 2.0 x86_64 — DOS-like shell, LYFS, AHCI/ATA, ring3, INT 80h.\n");return;}
    if(ieq(t[0],"CLS")){clear_screen();return;}
    if(ieq(t[0],"DIR")||ieq(t[0],"LS")){if(!fs_list())puts("DIR: filesystem offline.\n");return;}
    if(ieq(t[0],"TYPE")||ieq(t[0],"CAT")){
        if(nt<2){puts("TYPE: missing filename.\n");return;}
        uint8_t b[8192];uint32_t n=0;
        if(!fs_read_file(t[1],b,sizeof(b)-1,&n)){puts("File not found or disk error.\n");return;}
        b[n]=0;puts((char*)b);if(!n||b[n-1]!='\n')putc('\n');return;
    }
    if(ieq(t[0],"MKFILE")||ieq(t[0],"WRITE")||ieq(t[0],"TOUCH")){
        if(ieq(t[0],"TOUCH")&&nt>=2){puts(fs_write_file(t[1],"",0)?"Touched.\n":"Touch failed.\n");return;}
        if(nt<3){puts("MKFILE NAME TEXT\n");return;}
        if(fs_write_file(t[1],t[2],(uint32_t)strlen(t[2])))puts("File written to real disk.\n");else puts("Write failed.\n");return;
    }
    if(ieq(t[0],"DEL")||ieq(t[0],"ERASE")){
        if(nt<2||!fs_delete(t[1]))puts("Delete failed.\n");else puts("Deleted from real disk.\n");return;
    }
    if(ieq(t[0],"COPY")){
        if(nt<3){puts("COPY SOURCE DEST\n");return;}
        uint8_t b[8192];uint32_t n=0;
        if(!fs_read_file(t[1],b,sizeof(b),&n)||!fs_write_file(t[2],b,n))puts("COPY failed.\n");else puts("COPY complete.\n");
        return;
    }
    if(ieq(t[0],"REN")||ieq(t[0],"RENAME")){
        if(nt<3){puts("REN OLD NEW\n");return;}
        uint8_t b[8192];uint32_t n=0;
        if(!fs_read_file(t[1],b,sizeof(b),&n)||!fs_write_file(t[2],b,n)||!fs_delete(t[1]))puts("REN failed.\n");else puts("Renamed.\n");
        return;
    }
    if(ieq(t[0],"CHKDSK")){
        uint32_t f,u,tot;
        if(!fs_info(&f,&u,&tot)){puts("CHKDSK: disk unavailable.\n");return;}
        puts("CHKDSK: ");print_num(f);puts(" files, ");print_num(u);puts(" blocks used, ");print_num(tot-u);puts(" free.\n");return;
    }
    if(ieq(t[0],"FORMAT")){
        if(nt>=3&&ieq(t[1],"C:")&&ieq(t[2],"YES")){
            puts("FORMAT C: YES — formatting LYFS...\n");
            puts(fs_format()?"Format complete.\n":"Format failed.\n");
        }else puts("Syntax deliberately strict: FORMAT C: YES\n");
        return;
    }
    if(ieq(t[0],"FSINFO")){
        uint32_t f,u,tot;
        if(!fs_info(&f,&u,&tot)){puts("FSINFO: not mounted.\n");return;}
        puts("LYFS v1 — files ");print_num(f);puts(" blocks ");print_num(u);puts("/");print_num(tot);putc('\n');return;
    }
    if(ieq(t[0],"DISK")){puts("Disk backend: ");puts(disk_backend());putc('\n');return;}
    if(ieq(t[0],"PS")){proc_list();return;}
    if(ieq(t[0],"RUNPID")){if(!proc_run_init())puts("RUNPID failed.\n");return;}
    if(ieq(t[0],"DATE")){show_datetime(1);return;}
    if(ieq(t[0],"TIME")){show_datetime(0);return;}
    if(ieq(t[0],"CPU")){cpuinfo();return;}
    if(ieq(t[0],"MEM")){puts("1 GiB identity map active for this milestone; per-process page tables come later.\n");return;}
    if(ieq(t[0],"ECHO")){
        if(nt>1){puts(t[1]);for(size_t i=2;i<nt;i++){putc(' ');puts(t[i]);}}
        putc('\n');return;
    }
    if(ieq(t[0],"COLOR")){
        if(nt<2){puts("COLOR 07 / COLOR 0A / COLOR 4F\n");return;}
        uint8_t hi=0,lo=7;
        char x=t[1][0],y=t[1][1];
        if(x>='0'&&x<='9')hi=(uint8_t)(x-'0');else if(x>='A'&&x<='F')hi=(uint8_t)(x-'A'+10);else if(x>='a'&&x<='f')hi=(uint8_t)(x-'a'+10);
        if(y>='0'&&y<='9')lo=(uint8_t)(y-'0');else if(y>='A'&&y<='F')lo=(uint8_t)(y-'A'+10);else if(y>='a'&&y<='f')lo=(uint8_t)(y-'a'+10);
        attr=(uint8_t)((hi<<4)|lo);puts("Theme changed. Taste not included.\n");return;
    }
    if(ieq(t[0],"BEEP")){beep();puts("BEEP.\n");return;}
    if(ieq(t[0],"CALC")){puts("2+2=4. The calculator is the only responsible subsystem.\n");return;}
    if(ieq(t[0],"FART")){puts("PRRRRT. The filesystem survived.\n");return;}
    if(ieq(t[0],"DUPA")){puts("DUPA.EXE is technically the shell.\n");return;}
    if(ieq(t[0],"KURWA")){puts("KURWA.EXE: tak.\n");return;}
    if(ieq(t[0],"CHUJ")){puts("CHUJ.EXE loaded. Nothing useful happened. As designed.\n");return;}
    if(ieq(t[0],"IDIOT")){puts("Confirmed. You are using a DOS clone written in C.\n");return;}
    if(ieq(t[0],"WHY")){puts("Because reasonable operating systems were already taken.\n");return;}
    if(ieq(t[0],"NOTHING")){puts("Successfully performed absolutely nothing. Exit code 0.\n");return;}
    if(ieq(t[0],"DOOM")){puts("DOOM.EXE: demons requested a newer syscall ABI.\n");return;}
    if(ieq(t[0],"MOON")){puts("MOON: 384400 km away. Driver missing.\n");return;}
    if(ieq(t[0],"LASER")){puts("LASER: aimed at the concept of seriousness.\n");return;}
    if(ieq(t[0],"NUKE")){puts("NUKE: common sense blocked the request.\n");return;}
    if(ieq(t[0],"HACK")){puts("HACK complete: you successfully typed HACK.\n");return;}
    if(ieq(t[0],"SUDO")){puts("sudo: command not found. Kernel is already root and also annoyed.\n");return;}
    if(ieq(t[0],"BEER")){puts("Error 418: LietY2 kernel is not a bartender.\n");return;}
    if(ieq(t[0],"DANCE")){puts("[kernel] o/ \\o/ o/ \\o/\n");return;}
    if(ieq(t[0],"COFFEE")){puts("Coffee subsystem: imaginary caffeine injected successfully.\n");return;}
    if(ieq(t[0],"MATRIX")){for(int i=0;i<8;i++){puts("010101 1 0 1 0 KERNEL 1 0 1 0\n");delay(120000);}return;}
    if(ieq(t[0],"SPIN")){puts("Spinning... ");for(int i=0;i<6;i++){putc('/');delay(80000);putc('\b');putc('-');delay(80000);putc('\b');}puts("done.\n");return;}
    if(ieq(t[0],"PING")){puts("PONG. Network driver remains offensively absent.\n");return;}
    if(ieq(t[0],"PANIC")){puts("PANIC requested manually. Refused because that would be too efficient.\n");return;}
    if(ieq(t[0],"RICKROLL")){puts("Kernel caught the reference and continued booting.\n");return;}
    if(ieq(t[0],"REBOOT")){puts("Rebooting...\n");outb(0x64,0xFE);for(;;)__asm__ volatile("hlt");}
    if(ieq(t[0],"HALT")){puts("System halted. Press reset.\n");for(;;)__asm__ volatile("hlt");}
    puts("Bad command or file name. Also possibly a bad life choice.\n");
}

uint64_t syscall_handle(struct syscall_frame*f){
    switch(f->rax){
    case 1:{
        uint64_t p=f->rdi,n=f->rsi;
        if(p<0x400000||n>0x100000||p+n>0x900000){f->rax=(uint64_t)-14;return 0;}
        for(uint64_t i=0;i<n;i++)putc(((const char*)p)[i]);
        f->rax=n;return 0;
    }
    case 2:f->rax=proc_current_pid();return 0;
    case 3:proc_mark_exit();return 1;
    case 4:{
        uint64_t np=f->rdi,nl=f->rsi,bp=f->rdx,cap=f->r10;
        if(nl==0||nl>31||np<0x400000||np+nl>0x900000||bp<0x400000||cap>0x100000||bp+cap>0x900000){f->rax=(uint64_t)-14;return 0;}
        char name[32];for(uint64_t i=0;i<nl;i++)name[i]=((const char*)np)[i];name[nl]=0;
        uint32_t out=0;if(!fs_read_file(name,(void*)bp,(uint32_t)cap,&out)){f->rax=(uint64_t)-2;return 0;}
        f->rax=out;return 0;
    }
    case 5:{
        uint64_t np=f->rdi,nl=f->rsi,bp=f->rdx,len=f->r10;
        if(nl==0||nl>31||np<0x400000||np+nl>0x900000||bp<0x400000||len>0x100000||bp+len>0x900000){f->rax=(uint64_t)-14;return 0;}
        char name[32];for(uint64_t i=0;i<nl;i++)name[i]=((const char*)np)[i];name[nl]=0;
        if(!fs_write_file(name,(const void*)bp,(uint32_t)len)){f->rax=(uint64_t)-5;return 0;}
        f->rax=len;return 0;
    }
    default:f->rax=(uint64_t)-38;return 0;
    }
}

static char key(void){
    while(!(inb(0x64)&1))__asm__ volatile("pause");
    uint8_t s=inb(0x60);if(s&0x80)return 0;
    if(s==0x1C)return '\n';if(s==0x0E)return '\b';if(s==0x39)return ' ';
    if(s>=0x10&&s<=0x19){static const char r[]="qwertyuiop";return r[s-0x10];}
    if(s>=0x1E&&s<=0x26){static const char r[]="asdfghjkl";return r[s-0x1E];}
    if(s>=0x2C&&s<=0x32){static const char r[]="zxcvbnm";return r[s-0x2C];}
    if(s>=2&&s<=11){static const char r[]="1234567890";return r[s-2];}
    if(s==0x0F)return '\t';if(s==0x0C)return '-';if(s==0x0D)return '=';if(s==0x1A)return '[';if(s==0x1B)return ']';
    if(s==0x27)return ';';if(s==0x28)return '\'';if(s==0x33)return ',';if(s==0x34)return '.';if(s==0x35)return '/';return 0;
}

void kmain(void){
    serial_init();
    puts("EARLY: serial console online.\\n");
    clear_screen();
    puts("EARLY: console memory online.\\n");
    puts("PAGING: switching page tables...\\n");
    setup_paging();
    puts("PAGING: switched.\\n");
    arch_init();
    puts("ARCH: GDT/IDT/TSS online.\\n");
    proc_init();
    puts("\nLietY2 2.0 — The Operating System Nobody Ordered\n");
    puts("REAL kernel / REAL disk / REAL LYFS / REAL ring3 / REAL syscall / ZERO good reasons\n");
    puts("\nDetecting disk... ");if(disk_init())puts(disk_backend());else puts("NONE");putc('\n');
    if(!fs_mount()){
        puts("Filesystem not formatted. Creating LYFS v1...\n");
        if(!fs_format())puts("WARNING: disk format failed. Shell stays alive.\n");else puts("LYFS mounted.\n");
    }else puts("LYFS mounted from disk.\n");
    uint32_t f,u,t;if(fs_info(&f,&u,&t)){puts("FS: ");print_num(f);puts(" files, ");print_num(u);puts(" blocks used.\n");}
    if(proc_run_init()){}else puts("INIT not started.\n");
    proc_list();help();puts("\n");shell_prompt();
    char buf[256];size_t n=0;
    for(;;){
        char c=key();if(!c)continue;
        if(c=='\n'){putc('\n');buf[n]=0;command(buf);n=0;shell_prompt();}
        else if(c=='\b'){if(n){n--;putc('\b');}}
        else if(c>=32&&c<127&&n<sizeof(buf)-1){buf[n++]=c;putc(c);}
    }
}
