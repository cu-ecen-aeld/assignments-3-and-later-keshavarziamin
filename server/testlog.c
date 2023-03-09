#include <syslog.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main()
{
    setlogmask(LOG_UPTO(LOG_NOTICE));
    openlog("exampleprog ", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);
    printf("Program started by User %d\r\n", getuid());
    syslog(LOG_NOTICE, "Program started by User %d", getuid());
    syslog(LOG_NOTICE, "A tree falls in a forest");
    closelog();
}
