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

#define LOGGER_ENTRY_MAX_LEN            (20)
#define LOGGER_ENTRY_MAX_PAYLOAD        \
        (LOGGER_ENTRY_MAX_LEN - sizeof(struct logger_entry))
#define BUF_SIZE (40)

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
    struct mutex lock;  /* mutex protecting buffer */
    size_t w_off;
    size_t head;
    struct list_head readers;
    wait_queue_head_t wq;
    size_t size;

};

struct logger_reader {
    struct logger_dev *log;
    struct list_head list;
    size_t r_off;
};
#define logger_offset(n)        ((n) % log->size)


/*
 * The different configurable parameters
 */
extern int logger_major;     /* main.c */
extern int logger_nr_devs;

#endif /* _LOGGER_H_ */
