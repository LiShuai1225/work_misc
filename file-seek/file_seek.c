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



int READ_SIZE = 1024;
int TIMES = 20;

int BUFFER_SIZE=32*1024 ;

void usage()
{
    fprintf(stdout,"Usage:file-seek -f filename -n times -s read_size\n");
    return ;
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
    int ret ;
    char filename[1024] ;
    int fd;
    int i ;

    int file_set_flag = 0;
    int long_index = 0;
    
    off_t offset , f_size ; 
    int read_size = 0;

    int opt;
    static struct option option_long[] = {
        {"read_size",required_argument,0,'s'},
        {"times",required_argument,0,'n'},
        {"file",required_argument,0,'f'},
    };


    while((opt = getopt_long(argc,argv,"s:n:f:",option_long,&long_index)) != -1)
    {
        switch(opt)
        {
        case 's':
            READ_SIZE = atoi(optarg);
            break;
        case 'n':
            TIMES  = atoi(optarg);
            break;
        case 'f':
            snprintf(filename,1024,"%s",optarg);
            file_set_flag = 1;
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


    for( i= 0 ; i < TIMES ;i++)
    {
        offset = random() % f_size;
        lseek(fd, offset, SEEK_SET);

        if(offset + READ_SIZE < f_size)
            read_size = READ_SIZE ;
        else
            read_size = f_size - 1 - offset ;

        random_read(fd,offset,buffer,BUFFER_SIZE,read_size);
    }


out2:
    free(buffer);
out1:
    close(fd);
    return ret ;

}

