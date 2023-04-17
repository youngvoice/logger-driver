#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdint.h>


#define LOGGER_ENTRY_MAX_LEN            (4*1024)
#define LOGGER_ENTRY_MAX_PAYLOAD        \
        (LOGGER_ENTRY_MAX_LEN - sizeof(struct logger_entry))

struct logger_entry {
    uint16_t len;
    uint16_t __pad;
    int32_t pid;
    int32_t tid;
    char msg[0];
};


struct read_buffer {
    union {
        unsigned char buf[LOGGER_ENTRY_MAX_LEN + 1] __attribute__((aligned(4)));
        struct logger_entry entry __attribute__((aligned(4)));
    };

};

/*
int main()
{
    open();
    write();
    read();
    close();
    return 0;

}
*/
int main()
{
    //char buf[BUFSIZE];
    int ret;
    char s[] = "hello world";
    int s_len = strlen(s);
    struct read_buffer buf;
    int fd1 = open("/dev/my_log", O_RDONLY);
    if (fd1 < 0)
    {
	perror("open failed\n");
	exit(-1);
    }
    /*
    ret = write(fd1, s, s_len);
    if (ret < 0)
    {
	perror("write failed\n");
	exit(-1);
    }
    */
    sleep(5);

    ret = read(fd1, buf.buf, LOGGER_ENTRY_MAX_LEN);
    if (ret < 0)
    {
	perror("read failed\n");
	exit(-1);
    }

    printf("read log: len=%d %s\n", ret, buf.entry.msg);
    close(fd1);
    return 0;
}
