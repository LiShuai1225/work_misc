#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <limits.h>


unsigned long READ_SIZE = 1024;
int TIMES = 20;

unsigned long BUFFER_SIZE=128*1024 ;
int debug = 0;

void usage()
{
    fprintf(stdout,"Usage:file-seek -f filename -n times -s read_size -b buffer_size \n");
    return ;
}


/* simple version of process_size,may overflow 
 * improve it in the future */

int  process_size(char* input,unsigned long* output)
{
    int base = 10;
    char *str = input ;
    char *endptr ;

    unsigned long value = 0 ; 

    value = strtoul(str, &endptr, base);
    if(   (errno == ERANGE && (value == ULONG_MAX)) 
       || (errno != 0 && value == 0)
      )
    {
        return -1 ;
    }

    if(endptr == str)
    {
        return -2 ;
    }

    if(strlen(endptr) >= 2)
    {
        return -3;
    }
    
    switch(*endptr)
    {
    case 'k':
    case 'K':
        *output = value*1024;
        break;
    case 'm':
    case 'M':
        *output = value * 1024 * 1024;
        break;
    case 'g':
    case 'G':
        *output = value * 1024 * 1024 * 1024;
        break ;
    default:
        return -4 ;
    }

    return 0 ;
    
}

int random_read(int fd,off_t offset, void* buffer,size_t buffer_size, size_t count)
{
    int n = 0 ; 
    int ret ; 

    int size = (buffer_size < count)? buffer_size:count;

    off_t current = offset ; 
    while(count > 0 && ((n = read(fd, buffer, size)) != 0))
    {
        if(n == -1)
        {
            if(errno == EINTR)
            {
                continue;
            }
            else
            {
                fprintf(stderr, "failed to read [offset = %ld] [size = %ld] (%s)\n",
                        current, buffer_size, strerror(errno));
                ret = -1;
                break ;
            }
        }

        else if(n == 0) //EOF
        {
            break ;
        }
        else
        {
            current += n;
            count -= n ; 
            size = buffer_size < count? buffer_size:count;
        }
    }

    return ret ;
}

int main(int argc,char* argv[])
{
    int ret ,ret_b, ret_s;
    char filename[1024] ;
    int fd;
    int i ;

    int file_set_flag = 0;
    int long_index = 0;

    off_t offset , f_size ; 
    size_t read_size = 0;
    struct timeval tv ;
    int opt;


    static struct option option_long[] = {
        {"file",required_argument,0,'f'},
        {"read_size",required_argument,0,'s'},
        {"buffer_size",required_argument,0,'b'},
        {"times",required_argument,0,'n'},
        {"debug",required_argument,0,'d'},
    };


    while((opt = getopt_long(argc,argv,"s:n:f:b:d",option_long,&long_index)) != -1)
    {
        switch(opt)
        {
        case 's':
            ret_s  = process_size(optarg,&READ_SIZE);
            break;
        case 'n':
            TIMES  = atoi(optarg);
            break;
        case 'f':
            snprintf(filename,1024,"%s",optarg);
            file_set_flag = 1;
            break;
        case 'b':
            ret_b  = process_size(optarg, &BUFFER_SIZE);
            break;
        case 'd':
            debug = 1;
            break;
        default:
            usage();
            exit(1);
        }
    }

    if(file_set_flag == 0)
    {
        usage();
        fprintf(stderr,"please specifiy the file you want to operate\n");
        return 1;
    }

    if(ret_b || ret_s)
    {
        fprintf(stderr,"buffer_size or read_size is invalid\n");
        return 2;
    }

    fd = open(filename,O_RDONLY);
    if(fd == -1)
    {
        fprintf(stderr,"failed to open %s (%s)\n",filename,strerror(errno));
        return 2;
    }

    struct stat stat_buf ;
    fstat(fd, &stat_buf);

    f_size = stat_buf.st_size ; 


    char* buffer = malloc(BUFFER_SIZE);
    if(buffer == NULL)
    {
        fprintf(stderr,"failed to malloc (%s)\n",strerror(errno));
        ret = 3 ;
        goto out1;
    }

    srandom(getpid());

    for( i= 0 ; i < TIMES ;i++)
    {
        offset = (random()*100 + random()) % f_size;
        lseek(fd, offset, SEEK_SET);

        if(offset + READ_SIZE < f_size)
            read_size = READ_SIZE ;
        else
            read_size = f_size - 1 - offset ;

        if(debug)
        {
            gettimeofday(&tv,NULL);
            fprintf(stderr,"%ld.%06d [FILENAME = %30s][LOOP %d] [offset = %15ld] [READ_SIZE = %15ld] RANDOM READ BEGIN\n",
                    tv.tv_sec,(int)tv.tv_usec, filename, i , offset, read_size);
        }

        random_read(fd,offset,buffer,BUFFER_SIZE,read_size);

        if(debug)
        {
            gettimeofday(&tv,NULL);
            fprintf(stderr,"%ld.%06d [FILENAME = %30s][LOOP %d] [offset = %15ld] [READ_SIZE = %15ld] RANDOM READ END\n",
                    tv.tv_sec,(int)tv.tv_usec, filename, i , offset, read_size);
        }

    }


out2:
    free(buffer);
out1:
    close(fd);
    return ret ;

}

