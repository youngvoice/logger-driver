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
int main(int argc, char *argv[])
{
    int ret;
    int m_len = strlen(argv[1]);
    if (argc < 2) {
	perror("Usage: ./program message\n");
	exit(0);
    }
    // if you write the message large than LOGGER_ENTRY_MAX_PAYLOAD, it will be shorted by driver
    /*
     *
    if (m_len > LOGGER_ENTRY_MAX_LEN)
    {
	perror("the message is too long\n");
	exit(0);
    }
    */
    int fd1 = open("/dev/my_log", O_WRONLY);
    if (fd1 < 0)
    {
	perror("open failed\n");
	exit(-1);
    }
    ret = write(fd1, argv[1], m_len);
    if (ret < 0)
    {
	perror("write failed\n");
	exit(-1);
    }

    close(fd1);
    return 0;
}
