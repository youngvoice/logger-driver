#include "log_user.h"
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

    while(ret = read(fd1, buf.buf, LOGGER_ENTRY_MAX_LEN)) {
	if (ret < 0)
	{
	    perror("read failed\n");
	    exit(-1);
	}
	printf("[pid] %d read log: len=%d %s\n", getpid(), ret, buf.entry.msg);
    }

    close(fd1);
    return 0;
}
