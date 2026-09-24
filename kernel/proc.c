#include "kernel.h"

extern unsigned char __user_start[];
extern unsigned char __user_end[];

enum { P_UNUSED=0,P_RUN=1,P_ZOMB=2 };
struct proc { uint32_t pid; uint8_t state; char name[16]; };
static struct proc table[16];
static uint32_t next_pid=1,current_pid=0;

void proc_init(void){
    for(size_t i=0;i<16;i++){table[i].pid=0;table[i].state=P_UNUSED;table[i].name[0]=0;}
    table[0].pid=0;table[0].state=P_RUN;
    table[0].name[0]='k';table[0].name[1]='e';table[0].name[2]='r';table[0].name[3]='n';table[0].name[4]='e';table[0].name[5]='l';table[0].name[6]=0;
    next_pid=1;current_pid=0;
}
uint32_t proc_current_pid(void){return current_pid;}
void proc_mark_exit(void){for(size_t i=0;i<16;i++)if(table[i].pid==current_pid){table[i].state=P_ZOMB;break;}}
void proc_list(void){
    puts("\nPID STATE NAME\n");
    for(size_t i=0;i<16;i++)if(table[i].state!=P_UNUSED){
        puts("#");char n[12];uint32_t v=table[i].pid;int k=0;
        if(!v)n[k++]='0';while(v){n[k++]=(char)('0'+v%10);v/=10;}
        for(int j=k-1;j>=0;j--)putc(n[j]);
        puts("  ");puts(table[i].state==P_RUN?"RUN   ":"ZOMB  ");puts(table[i].name);putc('\n');
    }
}
int proc_run_init(void){
    size_t slot=16;
    for(size_t i=1;i<16;i++)if(table[i].state==P_UNUSED){slot=i;break;}
    if(slot==16)return 0;

    uint64_t entry=(uint64_t)__user_start;
    uint64_t size=(uint64_t)(__user_end-__user_start);
    if(entry!=0x400000||size<32||size>0x100000)return 0;

    table[slot].pid=next_pid++;table[slot].state=P_RUN;
    table[slot].name[0]='I';table[slot].name[1]='N';table[slot].name[2]='I';table[slot].name[3]='T';table[slot].name[4]=0;
    current_pid=table[slot].pid;
    puts("\nStarting INIT in ring 3...\n");
    enter_user(entry,0x804000);
    table[slot].state=P_ZOMB;
    current_pid=0;
    puts("INIT returned to kernel.\n");
    return 1;
}
