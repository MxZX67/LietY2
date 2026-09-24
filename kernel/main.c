#include <stdint.h>
#include <stddef.h>

#define VGA ((volatile uint16_t*)0xB8000)
#define W 80
#define H 25
#define COM1 0x3F8

static size_t row, col;
static uint8_t attr=0x07;

static inline void outb(uint16_t p,uint8_t v){__asm__ volatile("outb %0,%1"::"a"(v),"Nd"(p));}
static inline uint8_t inb(uint16_t p){uint8_t v;__asm__ volatile("inb %1,%0":"=a"(v):"Nd"(p));return v;}
static void delay(volatile uint64_t n){while(n--)__asm__ volatile("pause");}

static void serial_init(void){
 outb(COM1+1,0);outb(COM1+3,0x80);outb(COM1+0,3);outb(COM1+1,0);
 outb(COM1+3,3);outb(COM1+2,0xC7);outb(COM1+4,3);
}
static void serial_put(char c){while(!(inb(COM1+5)&0x20));outb(COM1,c);}
static void putc(char c){
 serial_put(c);
 if(c=='\n'){col=0;if(row<H-1)row++;return;}
 if(c=='\r'){col=0;return;}
 if(c=='\b'){if(col)col--;VGA[row*W+col]=(attr<<8)|' ';return;}
 VGA[row*W+col]=(attr<<8)|(uint8_t)c;
 if(++col>=W){col=0;if(row<H-1)row++;}
}
static void puts(const char*s){while(*s)putc(*s++);}
static void clear(void){for(size_t i=0;i<W*H;i++)VGA[i]=(attr<<8)|' ';row=col=0;}
static int eq(const char*a,const char*b){while(*a&&*b&&*a==*b){a++;b++;}return *a==*b;}

static char key(void){
 while(!(inb(0x64)&1))__asm__ volatile("pause");
 uint8_t s=inb(0x60); if(s&0x80)return 0;
 if(s==0x1C)return '\n'; if(s==0x0E)return '\b'; if(s==0x39)return ' ';
 if(s>=0x10&&s<=0x19){static const char r[]="qwertyuiop";return r[s-0x10];}
 if(s>=0x1E&&s<=0x26){static const char r[]="asdfghjkl";return r[s-0x1E];}
 if(s>=0x2C&&s<=0x32){static const char r[]="zxcvbnm";return r[s-0x2C];}
 if(s>=2&&s<=11){static const char r[]="1234567890";return r[s-2];}
 if(s==0x0F)return '\t';if(s==0x0C)return '-';if(s==0x0D)return '=';
 if(s==0x1A)return '[';if(s==0x1B)return ']';if(s==0x27)return ';';
 if(s==0x28)return '\'';if(s==0x33)return ',';if(s==0x34)return '.';
 if(s==0x35)return '/';return 0;
}

static void help(void){
 puts("\nCommands:\n");
 puts("DIR TYPE DEL ERASE ECHO VER CLS HELP MEM DATE TIME\n");
 puts("FORMAT C: YES REBOOT HALT BEEP MATRIX SPIN IDIOT SUDO\n");
 puts("DRINK BEER MOON LASER DOOM FART NUKE HACK NOTHING WHY\n");
 puts("DANCE COFFEE MAKEFILE PING KILLSELF ABOUT SYS\n");
 puts("Some commands are useful. Most are evidence.\n");
}
static void command(char*s){
 while(*s==' ')s++;
 if(!*s)return;
 if(eq(s,"help")){help();return;}
 if(eq(s,"ver")||eq(s,"about")||eq(s,"sys")){puts("LietY2 0.1 x86_64 - DOS aesthetics, real kernel.\n");return;}
 if(eq(s,"cls")){clear();return;}
 if(eq(s,"dir")){puts(" Volume in drive C is CHAOS\n\n <DIR> .\n <DIR> ..\n 42 README.TXT\n 1337 IMPORTANT.TXT\n\n2 file(s), 1379 bytes\n");return;}
 if(eq(s,"type")){puts("TYPE needs a filename. Try TYPE README.TXT\n");return;}
 if(eq(s,"del")||eq(s,"erase")){puts("File deleted. Reality unchanged.\n");return;}
 if(eq(s,"echo")){puts(s+4);putc('\n');return;}
 if(eq(s,"mem")){puts("65536K conventional memory. 0K useful paperwork.\n");return;}
 if(eq(s,"date")){puts("Date: somewhere between boot and regret.\n");return;}
 if(eq(s,"time")){puts("Time: approximately now.\n");return;}
 if(eq(s,"format c: yes")){puts("FORMAT C: acknowledged. Disk driver is next milestone.\n");return;}
 if(eq(s,"beep")){outb(0x61,inb(0x61)|3);delay(500000);outb(0x61,inb(0x61)&~3);puts("BEEP.\n");return;}
 if(eq(s,"matrix")){for(int i=0;i<12;i++){puts("010101 KERNEL 101010\n");delay(250000);}return;}
 if(eq(s,"spin")){puts("Spinning absolutely nothing...");for(int i=0;i<8;i++){putc('.');delay(150000);}puts(" done.\n");return;}
 if(eq(s,"idiot")){puts("Confirmed. You installed an OS with a FART command.\n");return;}
 if(eq(s,"sudo")){puts("sudo: nope. This kernel is the boss.\n");return;}
 if(eq(s,"drink")||eq(s,"beer")){puts("Error 418: kernel is not a bartender.\n");return;}
 if(eq(s,"moon")){puts("MOON: 384400 km away. Driver not found.\n");return;}
 if(eq(s,"laser")){puts("LASER armed. Target: seriousness.\n");return;}
 if(eq(s,"doom")){puts("DOOM cannot start: demons requested a newer ABI.\n");return;}
 if(eq(s,"fart")){puts("PRRRT. System integrity: somehow improved.\n");return;}
 if(eq(s,"nuke")){puts("NUKE: denied by common sense.\n");return;}
 if(eq(s,"hack")){puts("HACK: typing commands is not hacking, genius.\n");return;}
 if(eq(s,"nothing")){puts("Successfully did absolutely nothing.\n");return;}
 if(eq(s,"why")){puts("Because someone thought this was a good idea.\n");return;}
 if(eq(s,"dance")){puts("<kernel> o/  \\o/  \\o/\n");return;}
 if(eq(s,"coffee")){puts("Coffee subsystem online. Human subsystem questionable.\n");return;}
 if(eq(s,"makefile")){puts("Makefile: make it work, then make it weird.\n");return;}
 if(eq(s,"ping")){puts("PONG. No network card required.\n");return;}
 if(eq(s,"killself")){puts("Nice try. Kernel refuses to self-delete.\n");return;}
 if(eq(s,"reboot")){puts("Rebooting...\n");outb(0x64,0xFE);for(;;)__asm__ volatile("hlt");}
 if(eq(s,"halt")){puts("System halted. Press reset.\n");for(;;)__asm__ volatile("hlt");}
 puts("Bad command or file name. Also possibly a bad life choice.\n");
}

void kmain(void){
 serial_init();clear();
 puts("\nLietY2 0.1 - The Operating System Nobody Ordered\n");
 puts("Real x86_64 kernel. DOS-ish shell. Completely unnecessary features.\n");
 help();puts("\nC:\\>");
 char buf[128];size_t n=0;
 for(;;){
  char c=key();if(!c)continue;
  if(c=='\n'){putc('\n');buf[n]=0;command(buf);n=0;puts("C:\\>");}
  else if(c=='\b'){if(n){n--;putc('\b');}}
  else if(c>=32&&c<127&&n<sizeof(buf)-1){buf[n++]=c;putc(c);}
 }
}
