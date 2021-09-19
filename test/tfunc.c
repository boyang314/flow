#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/mman.h>
#include <syscall.h>
#include <dlfcn.h>
#include <fcntl.h>

#define memfd_create(name) syscall(SYS_memfd_create, name, 0)

int i = 0;

__attribute__((noinline, section("mysec"))) void test_func (void)
{
    i++;
}

__attribute__((noinline, section("mysed"))) int test_f2 (int i)
{
    i++;
    //printf("%d\n", i); //-fPIC, -fPIE do not help
    return i;
}

int main (void)
{
    extern unsigned char __start_mysec[];
    extern unsigned char __stop_mysec[];
    size_t sz = __stop_mysec - __start_mysec;

    extern unsigned char __start_mysed[];
    extern unsigned char __stop_mysed[];
    size_t sz2 = __stop_mysed - __start_mysed;

    printf ("Func len: %lu\n", sz);
    test_func ();
    printf("i:%d\n", i);

    int fd = memfd_create("");
    write(fd, (char*)test_func, sz);
    char *addr = (char*)mmap(NULL, 4096, PROT_EXEC, MAP_PRIVATE, fd, 0);
    int rc = memcmp((char*)test_func, addr, sz);
    printf("rc:%d\n", rc);
    //depend on global i, not posistion indepent
    //((void (*)())addr)();
    test_func ();
    printf("i:%d\n", i);

    //many CALL or JUMP machine instructions are relative to the program counter
    //(so if you "move" them they will jump to an erroneous location).
    //
    //you will not be able to call any other functions from the one you're copying (f1),
    //because relative addresses are hardcoded into the binary code of the function,
    //and moving it into a different location it the memory can make those relative addresses turn bad.
    //
    int fd2 = memfd_create("f");
    write(fd2, (char*)test_f2, sz2);
    char *addr2 = (char*)mmap(NULL, 4096, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE, fd2, 0);
    int rc2 = memcmp((char*)test_f2, addr2, sz2);
    printf("rc2:%d\n", rc2);
    //test_f2(5);
    int ret = ((int (*)(int))addr2)(5);
    printf("ret:%d\n", ret);

    //copy so to memfd
    void *handle = dlopen("libtf3.so", RTLD_NOW | RTLD_LOCAL);
    if (!handle) { printf("%s\n", dlerror()); }
    int (*foo)(int);
    foo = (int (*)(int))dlsym(handle, "test_f3");

    if (foo) foo(7);
    else { printf("failed dlsym\n"); }

    int fd3 = open("libpopen.so", O_RDONLY);
    char tbuf[8192];
    int len = read(fd3, tbuf, sizeof(tbuf));
    printf("read len:%d\n", len);
    
    int fd4 = memfd_create("g");
    write(fd4, tbuf, len);
    char libname[64];
    sprintf(libname, "/proc/%d/fd/%d", getpid(), fd4);
    printf("libname:%s\n", libname);

    void *handle2 = dlopen(libname, RTLD_NOW | RTLD_LOCAL);
    if (!handle2) { printf("%s\n", dlerror()); }
    void (*foo2)(const char*, int);
    foo2 = (void (*)(const char*, int))dlsym(handle2, "reqResp");
    if (foo2) foo2("ls", STDOUT_FILENO);
    else { printf("failed dlsym\n"); }

    return 0;
}
