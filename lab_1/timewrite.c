#include "include/apue.h"
#include <fcntl.h>
#include <sys/times.h>

int main(int argc, char *argv[]){
    int fd, ticks, BUFFSIZE, LoopsCount;
    long long int length;
    char *buff;
    clock_t ClockStart, ClockEnd;
    struct tms tmsStart, tmsEnd;
    double UserTime, SysTime, ClockTime;

    /* check args */
    if (argc != 2 && argc != 3){
        err_quit("agrv eroor: ./timewrite <f1 f2 or ./timewrite f1 sync <f2");
    }
    if (argc == 3 && strcmp(argv[2], "sync") != 0){
        err_quit("./timewrite f1 sync <f2");
    }

    /* open file */

    if (argc == 2){ // async
        if ((fd = open(argv[1], O_RDWR|O_CREAT|O_TRUNC, FILE_MODE)) < 0){
            err_sys("Cannot open");
        }
    }
    else { //sync
        if ((fd = open(argv[1], O_RDWR|O_CREAT|O_TRUNC|O_SYNC, FILE_MODE)) < 0) {
            err_sys("Cannot open");
        }
    }

    /* read file */

    // get the length of file
    if ((length = lseek(STDIN_FILENO, 0, SEEK_END)) == -1){
        err_sys("lseek error");
    }

    // allocate memory for buff
    if ((buff = (char*)malloc(sizeof(char)*length)) == NULL){
        err_sys("malloc error");
    }

    // move to head
    if (lseek(STDIN_FILENO, 0, SEEK_SET) == -1 ){
        err_sys("lseek error");
    }

    // read to buff
    if (read(STDIN_FILENO, buff, length) < 0){
        err_sys("read error");
    }

    // Table head    
    printf("%-8s\t %-8s\t %-8s\t %-8s\t %-8s\t\n", "BUFFSIZE", "User CPU", "System CPU", "Clock time", "Loops Count");

    // CPU CLOCK
    ticks = sysconf(_SC_CLK_TCK);

    // use different buffsize to write
    for (BUFFSIZE = 1024; BUFFSIZE <= 524288; BUFFSIZE *= 2) {
        if (lseek(fd, 0, SEEK_SET) == -1){
            err_sys("lseek error");
        }
        LoopsCount = length / BUFFSIZE;

        ClockStart = times(&tmsStart);
        for (int i = 0; i < LoopsCount;i++){
            if (write(fd, buff + i * BUFFSIZE, BUFFSIZE) != BUFFSIZE){
                printf("%d\n", BUFFSIZE);
                err_sys("write error");
            }   
        }
        // if remain
        int remain = length % BUFFSIZE;
        if (remain){
            if (write(fd, buff + LoopsCount * BUFFSIZE, remain) != remain){
                err_sys("write error");
            }
            LoopsCount++;
        }
        ClockEnd = times(&tmsEnd);

        UserTime = (double)(tmsEnd.tms_utime - tmsStart.tms_utime) / ticks;
        SysTime = (double)(tmsEnd.tms_stime - tmsStart.tms_stime) / ticks;
        ClockTime = (double)(ClockEnd - ClockStart) / ticks;

        printf("%8d\t %8.2f\t %8.2f\t %8.2f\t %8d\n", BUFFSIZE, UserTime, SysTime, ClockTime, LoopsCount);
    }
}