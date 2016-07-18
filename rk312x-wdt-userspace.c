#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <strings.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <stdint.h>
#include <time.h>

#define WDT_OFFSET 0x2004c000
#define CRU_OFFSET 0x20000000

#define CRU_CLKGATE7_CON (CRU_OFFSET + 0xec)
#define CRU_CLKGATE7_CON__WDT_DIS (1 << 15)
#define CRU_CLKGATE7_CON__WDT_EN  (0 << 15)
#define CRU_CLKGATE7_CON__WDT_MSK (1 << 31)

#define WDT_CR (WDT_OFFSET + 0x00)
#define WDT_CR__PULSE_LEN_2     (0 << 2)
#define WDT_CR__PULSE_LEN_4     (1 << 2)
#define WDT_CR__PULSE_LEN_8     (2 << 2)
#define WDT_CR__PULSE_LEN_16    (3 << 2)
#define WDT_CR__PULSE_LEN_32    (4 << 2)
#define WDT_CR__PULSE_LEN_64    (5 << 2)
#define WDT_CR__PULSE_LEN_128   (6 << 2)
#define WDT_CR__PULSE_LEN_256   (7 << 2)

#define WDT_CR__GEN_IRQ         (1 << 1)
#define WDT_CR__SKIP_IRQ        (1 << 0)

#define WDT_CR__WDT_DISABLE     (0 << 0)
#define WDT_CR__WDT_ENABLE      (1 << 0)

#define WDT_TORR (WDT_OFFSET + 0x04)
#define WDT_TORR__TO_FFFF     ( 0 << 0)
#define WDT_TORR__TO_1FFFF    ( 1 << 0)
#define WDT_TORR__TO_3FFFF    ( 2 << 0)
#define WDT_TORR__TO_7FFFF    ( 3 << 0)
#define WDT_TORR__TO_FFFFF    ( 4 << 0)
#define WDT_TORR__TO_1FFFFF   ( 5 << 0)
#define WDT_TORR__TO_3FFFFF   ( 6 << 0)
#define WDT_TORR__TO_7FFFFF   ( 7 << 0)
#define WDT_TORR__TO_FFFFFF   ( 8 << 0)
#define WDT_TORR__TO_1FFFFFF  ( 9 << 0)
#define WDT_TORR__TO_3FFFFFF  (10 << 0)
#define WDT_TORR__TO_7FFFFFF  (11 << 0)
#define WDT_TORR__TO_FFFFFFF  (12 << 0)
#define WDT_TORR__TO_1FFFFFFF (13 << 0)
#define WDT_TORR__TO_3FFFFFFF (14 << 0)
#define WDT_TORR__TO_7FFFFFFF (15 << 0)

#define WDT_CCVR (WDT_OFFSET + 0x08)

#define WDT_CRR (WDT_OFFSET + 0x0c)
#define WDT_CRR__VALUE 0x76

static uint32_t *mem_32 = 0;
static int   *prev_mem_range = 0;

extern void sys_exit(int code);
extern int sys_write(int fd, const void *buf, int count);
extern int sys_open(const char *pathname, int flags);
extern int sys_close(int fd);
extern void *sys_mmap(void *addr, size_t length, int prot, int flags,
                      int fd, off_t offset);
extern void *sys_mmap2(void *addr, size_t length, int prot, int flags,
                       int fd, off_t offset);
extern int sys_munmap(void *addr, size_t length);
extern int sys_nanosleep(const struct timespec *req, struct timespec *rem);

static int _strlen(const char *s) {
    int len = 0;
    while (*s++)
        len++;
    return len;
}

static void _puts(const char *s) {
    sys_write(1, s, _strlen(s));
    sys_write(1, "\n", 1);
}

static void _perror(const char *s) {
    sys_write(2, s, _strlen(s));
    sys_write(2, "\n", 1);
}

static void _sleep(int secs) {
    struct timespec sleepsecs = {
        .tv_sec = secs,
        .tv_nsec = 0,
    };
    sys_nanosleep(&sleepsecs, NULL);
}

static int readl(long offset) {
    int result;
    static int mem_fd;

    int *mem_range = (int *)(offset & ~0xFFFF);
    if (mem_range != prev_mem_range) {
        prev_mem_range = mem_range;

        if (mem_32)
            sys_munmap(mem_32, 0xFFFF);
        if (mem_fd)
            sys_close(mem_fd);

        mem_fd = sys_open("/dev/mem", O_RDWR);
        if (mem_fd < 0) {
            _perror("Unable to open /dev/mem");
            mem_fd = 0;
            return -1;
        }

        mem_32 = sys_mmap2(0, 0xffff, PROT_READ | PROT_WRITE, MAP_SHARED, mem_fd, (offset&~0xFFFF) / 4096);
        //mem_32 = mmap(0, 0xffff, PROT_READ | PROT_WRITE, MAP_SHARED, mem_fd, (offset&~0xFFFF));
        if( -1 == (int)mem_32 ) {
            _perror("Unable to mmap file");

            if( -1 == sys_close(mem_fd) )
                _perror("Also couldn't close file");

            mem_fd = 0;
            return -1;
        }
    }

    int scaled_offset = (offset-(offset&~0xFFFF));
    result = mem_32[scaled_offset/sizeof(long)];

    return result;
}

static int writel(long value, long offset) {
    int old_value = readl(offset);
    int scaled_offset = (offset-(offset&~0xFFFF));
    mem_32[scaled_offset/sizeof(long)]  = value;
    return old_value;
}

static void kick_wdt(void) {

    /* Write the magic value to the WDT reset register */
    //devmem2-static 0x2004c00c w 0x76
    writel(WDT_CRR__VALUE, WDT_CRR);
}

static void enable_wdt(void) {

    /* Enable the WDT block */
    //devmem2-static 0x200000ec w 0x80000000
    writel(CRU_CLKGATE7_CON__WDT_MSK | CRU_CLKGATE7_CON__WDT_EN,
                        CRU_CLKGATE7_CON);
    (void)readl(CRU_CLKGATE7_CON);

    /* Set the longest WDT timeout possible (0x7fffffff ticks) */
    //devmem2-static 0x2004c004 w 0xf
    writel(WDT_TORR__TO_7FFFFFFF, WDT_TORR);

    /* Pre-acknowledge the WDT, or else we'll reboot right away */
    kick_wdt();

    /* Enable the WDT */
    //devmem2-static 0x2004c000 w 0xf
    writel(WDT_CR__WDT_ENABLE | WDT_CR__PULSE_LEN_256 | WDT_CR__SKIP_IRQ,
           WDT_CR);

    return;
}

//int main(int argc, char **argv) {
int _start(void) {

    _puts("Enabling WDT...");
    enable_wdt();

    _puts("Done.  Will reset the WDT every 15 seconds.");
    _puts("If you stop this program, the system will reset within 28 seconds!");
    while (1) {
        _sleep(15);
        kick_wdt();
    }

    sys_exit(0);
}
