#include <stdio.h>
#include <syslog.h>
#include <string.h>

int main(int argc, char *argv[])
{
    char *filePath;
    char *str;
    FILE *file;

    if (argc == 2 && strcmp(argv[1], "--help") == 0)
    {
        printf("help:\n writer /path/to/file string\r\n");
        return 0;
    }
    else if (argc != 3)
    {
        printf("the number of arguments is not valid\r\n");
        printf("use --help to show you how to run\r\n");
        return 1;
    }

    filePath = argv[1];
    str = argv[2];


    openlog(NULL, 0, LOG_USER); // open system logger

    file = fopen(filePath, "w");

    // checking path  of file
    if (!file)
    {
        printf("opening file failed.\r\n");
        syslog(LOG_ERR, "the system can not open file in %s.\r\n", filePath);
        return 1;
    }
    else
    {
        int retEcho = fprintf(file, "%s", str); //write string in file
        //check result of writing
        if (retEcho < 0)
        {
            printf("writing your string failed.\r\n");
            syslog(LOG_ERR, "the system can not write your string \"%s\" in %s.\r\n", str, filePath);
            return 1;
        }
    }

    syslog(LOG_DEBUG, "Writing %s to %s", str, filePath);

    fclose(file);

    return 0;
}
