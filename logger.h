/*
 * logger.h -- definitions for the char module
 *
 */

#ifndef _LOGGER_H_
#define _LOGGER_H_

#include <linux/ioctl.h> /* needed for the _IOW etc stuff used later */

#ifndef LOGGER_MAJOR
#define LOGGER_MAJOR 0   /* dynamic major by default */
#endif

#ifndef LOGGER_NR_DEVS
#define LOGGER_NR_DEVS 4    /* logger0 through logger3 */
#endif

#define LOGGER_ENTRY_MAX_LEN            (4*1024)
#define LOGGER_ENTRY_MAX_PAYLOAD        \
        (LOGGER_ENTRY_MAX_LEN - sizeof(struct logger_entry))
#define BUF_SIZE (16*1024)

struct logger_entry {
    __u16 len;
    __u16 __pad;
    __s32 pid;
    __s32 tid;
    char msg[0];
};

struct logger_dev {
    unsigned char * buffer;
    struct cdev cdev;	  /* Char device structure		*/
    size_t w_off;
    size_t r_off;
    size_t size;

};

#define logger_offset(n)        ((n) & (log->size - 1))


/*
 * The different configurable parameters
 */
extern int logger_major;     /* main.c */
extern int logger_nr_devs;

#endif /* _LOGGER_H_ */
