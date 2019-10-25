#include <cstdio>
#include <sys/ptrace.h>
#include <cerrno>
#include <sys/wait.h>
#include <syscall.h>
#include <malloc.h>
#include <algorithm>
#include <sys/mman.h>
#include <dlfcn.h>
#include <elf.h>
#include <cstring>


#define CPSR_T_MASK     ( 1u << 5 )

const int long_size = sizeof(long);
const char *libc_path = "/system/lib/libc.so";



long get_remote_addr(pid_t target_pid, const char* module_name, void* local_addr);
long getSysCallNo(int pid, struct pt_regs *regs);
void hookSysCallBefore(pid_t pid);
void hookSysCallAfter(pid_t pid);
void modifyString(pid_t pid, long addr, long strlen);
void putdata(pid_t child, long addr,
             char *str, int len);
void getdata(pid_t child, long addr,
             char *str, int len);


int main(int argc, char *argv[])
{
    if(argc != 2) {
        printf("Usage: %s <pid to be traced>\n", argv[0]);
        return 1;
    }

    pid_t pid;
    int status;
    pid = atoi(argv[1]);

    if(0 != ptrace(PTRACE_ATTACH, pid, NULL, NULL))
    {
        printf("Trace process failed:%d.\n", errno);
        return 1;
    }

    ptrace(PTRACE_SYSCALL, pid, NULL, NULL);

    while(1)
    {
        wait(&status);
        hookSysCallBefore(pid);
        ptrace(PTRACE_SYSCALL, pid, NULL, NULL);

        wait(&status);
        hookSysCallAfter(pid);
        ptrace(PTRACE_SYSCALL, pid, NULL, NULL);
    }

    ptrace(PTRACE_DETACH, pid, NULL, NULL);
    return 0;
}


void hookSysCallBefore(pid_t pid)
{
    struct pt_regs regs;
    int sysCallNo = 0;

    ptrace(PTRACE_GETREGS, pid, NULL, &regs);
    sysCallNo = getSysCallNo(pid, &regs);
    printf("Before SysCallNo = %d\n",sysCallNo);

    if(sysCallNo == __NR_write)
    {
        printf("__NR_write: %ld %p %ld\n",regs.ARM_r0,(void*)regs.ARM_r1,regs.ARM_r2);
    }
}

void hookSysCallAfter(pid_t pid)
{
    struct pt_regs regs;
    int sysCallNo = 0;

    ptrace(PTRACE_GETREGS, pid, NULL, &regs);
    sysCallNo = getSysCallNo(pid, &regs);

    printf("After SysCallNo = %d\n",sysCallNo);

    if(sysCallNo == __NR_write)
    {
        printf("__NR_write return: %ld\n",regs.ARM_r0);
    }

    printf("\n");
}
void reverse(char *str)
{   int i, j;
    char temp;
    for(i = 0, j = strlen(str) - 2;
        i <= j; ++i, --j) {
        temp = str[i];
        str[i] = str[j];
        str[j] = temp;
    }
}
long getSysCallNo(int pid, struct pt_regs *regs)
{
    long scno = 0;
    scno = ptrace(PTRACE_PEEKTEXT, pid, (void *)(regs->ARM_pc - 4), NULL);
    if(scno == 0)
        return 0;

    if (scno == 0xef000000) {
        scno = regs->ARM_r7;
    } else {
        if ((scno & 0x0ff00000) != 0x0f900000) {
            return -1;
        }
        scno &= 0x000fffff;
    }
    return scno;
}
void modifyString(pid_t pid, long addr, long strlen)
{
    char* str;
    str = (char *)calloc((strlen+1) * sizeof(char), 1);
    getdata(pid, addr, str, strlen);
    reverse(str);
    putdata(pid, addr, str, strlen);
}
void getdata(pid_t child, long addr,
             char *str, int len)
{   char *laddr;
    int i, j;
    union u {
        long val;
        char chars[long_size];
    }data;
    i = 0;
    j = len / long_size;
    laddr = str;
    while(i < j) {
        data.val = ptrace(PTRACE_PEEKDATA,
                          child, addr + i * 4,
                          NULL);
        memcpy(laddr, data.chars, long_size);
        ++i;
        laddr += long_size;
    }
    j = len % long_size;
    if(j != 0) {
        data.val = ptrace(PTRACE_PEEKDATA,
                          child, addr + i * 4,
                          NULL);
        memcpy(laddr, data.chars, j);
    }
    str[len] = '\0';
}

void putdata(pid_t child, long addr,
             char *str, int len)
{   char *laddr;
    int i, j;
    union u {
        long val;
        char chars[long_size];
    }data;
    i = 0;
    j = len / long_size;
    laddr = str;
    while(i < j) {
        memcpy(data.chars, laddr, long_size);
        ptrace(PTRACE_POKEDATA, child,
               addr + i * 4, data.val);
        ++i;
        laddr += long_size;
    }
    j = len % long_size;
    if(j != 0) {
        memcpy(data.chars, laddr, j);
        ptrace(PTRACE_POKEDATA, child,
               addr + i * 4, data.val);
    }
}
void* get_module_base(pid_t pid, const char* module_name)
{
    FILE *fp;
    long addr = 0;
    char *pch;
    char filename[32];
    char line[1024];
    if (pid == 0) {
        snprintf(filename, sizeof(filename), "/proc/self/maps");
    } else {
        snprintf(filename, sizeof(filename), "/proc/%d/maps", pid);
    }
    fp = fopen(filename, "r");

    if (fp != NULL) {
        while (fgets(line, sizeof(line), fp)) {
            if (strstr(line, module_name)) {
                pch = strtok( line, "-" );
                addr = strtoul( pch, NULL, 16 );
                if (addr == 0x8000)
                    addr = 0;
                break;
            }
        }
        fclose(fp) ;
    }
    return (void *)addr;
}

long get_remote_addr(pid_t target_pid, const char* module_name, void* local_addr)
{
    void* local_handle, *remote_handle;

    local_handle = get_module_base(0, module_name);
    remote_handle = get_module_base(target_pid, module_name);

    printf("module_base: local[%p], remote[%p]\n", local_handle, remote_handle);

    long ret_addr = (long)((uint32_t)local_addr + (uint32_t)remote_handle - (uint32_t)local_handle);

    printf("remote_addr: [%p]\n", (void*) ret_addr);

    return ret_addr;
}
int ptrace_setregs(pid_t pid, struct pt_regs * regs)
{
    if (ptrace(PTRACE_SETREGS, pid, NULL, regs) < 0) {
        perror("ptrace_setregs: Can not set register values");
        return -1;
    }

    return 0;
}
int ptrace_continue(pid_t pid)
{
    if (ptrace(PTRACE_CONT, pid, NULL, 0) < 0) {
        perror("ptrace_cont");
        return -1;
    }

    return 0;
}
int ptrace_call(pid_t pid, long addr, long *params, uint32_t num_params, struct pt_regs* regs)
{
    uint32_t i;
    for (i = 0; i < num_params && i < 4; i ++) {
        regs->uregs[i] = params[i];
    }
    //
    // push remained params onto stack
    //
    if (i < num_params) {
        regs->ARM_sp -= (num_params - i) * sizeof(long) ;
        putdata(pid, (long)regs->ARM_sp, (char*)&params[i], (num_params - i) * sizeof(long));
    }

    regs->ARM_pc = addr;
    if (regs->ARM_pc & 1) {
        /* thumb */
        regs->ARM_pc &= (~1u);
        regs->ARM_cpsr |= CPSR_T_MASK;
    } else {
        /* arm */
        regs->ARM_cpsr &= ~CPSR_T_MASK;
    }

    regs->ARM_lr = 0;

    if (ptrace_setregs(pid, regs) == -1
        || ptrace_continue(pid) == -1) {
        printf("error\n");
        return -1;
    }

    int stat = 0;
    waitpid(pid, &stat, WUNTRACED);
    while (stat != 0xb7f) {
        if (ptrace_continue(pid) == -1) {
            printf("error\n");
            return -1;
        }
        waitpid(pid, &stat, WUNTRACED);
    }

    return 0;
}

void injectSo(pid_t pid,char* so_path, char* function_name,char* parameter)
{
    struct pt_regs old_regs,regs;
    long mmap_addr, dlopen_addr, dlsym_addr, dlclose_addr;

//save old regs

    ptrace(PTRACE_GETREGS, pid, NULL, &old_regs);
    memcpy(&regs, &old_regs, sizeof(regs));

// get remote addres

    printf("getting remote addres:\n");
    mmap_addr = get_remote_addr(pid, libc_path, (void *)mmap);
    dlopen_addr = get_remote_addr( pid, libc_path, (void *)dlopen );
    dlsym_addr = get_remote_addr( pid, libc_path, (void *)dlsym );
    dlclose_addr = get_remote_addr( pid, libc_path, (void *)dlclose );

    printf("mmap_addr=%p dlopen_addr=%p dlsym_addr=%p dlclose_addr=%p\n",
           (void*)mmap_addr,(void*)dlopen_addr,(void*)dlsym_addr,(void*)dlclose_addr);

    long parameters[10];

//mmap

    parameters[0] = 0; //address
    parameters[1] = 0x4000; //size
    parameters[2] = PROT_READ | PROT_WRITE | PROT_EXEC; //WRX
    parameters[3] = MAP_ANONYMOUS | MAP_PRIVATE; //flag
    parameters[4] = 0; //fd
    parameters[5] = 0; //offset

    ptrace_call(pid, mmap_addr, parameters, 6, &regs);

    ptrace(PTRACE_GETREGS, pid, NULL, &regs);
    long map_base = regs.ARM_r0;

    printf("map_base = %p\n", (void*)map_base);

//dlopen

    printf("save so_path = %s to map_base = %p\n", so_path, (void*)map_base);
    putdata(pid, map_base, so_path, strlen(so_path) + 1);

    parameters[0] = map_base;
    parameters[1] = RTLD_NOW| RTLD_GLOBAL;

    ptrace_call(pid, dlopen_addr, parameters, 2, &regs);

    ptrace(PTRACE_GETREGS, pid, NULL, &regs);
    long handle = regs.ARM_r0;

    printf("handle = %p\n",(void*) handle);

//dlsym

    printf("save function_name = %s to map_base = %p\n", function_name, (void*)map_base);
    putdata(pid, map_base, function_name, strlen(function_name) + 1);

    parameters[0] = handle;
    parameters[1] = map_base;

    ptrace_call(pid, dlsym_addr, parameters, 2, &regs);

    ptrace(PTRACE_GETREGS, pid, NULL, &regs);
    long function_ptr = regs.ARM_r0;

    printf("function_ptr = %p\n", (void*)function_ptr);

//function_call

    printf("save parameter = %s to map_base = %p\n", parameter, (void*)map_base);
    putdata(pid, map_base, parameter, strlen(parameter) + 1);

    parameters[0] = map_base;

    ptrace_call(pid, function_ptr, parameters, 1, &regs);

//dlcose

    parameters[0] = handle;

    ptrace_call(pid, dlclose_addr, parameters, 1, &regs);

//restore old regs

    ptrace(PTRACE_SETREGS, pid, NULL, &old_regs);
}