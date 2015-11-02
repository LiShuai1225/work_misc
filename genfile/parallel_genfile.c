#define _GNU_SOURCE

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pthread.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <getopt.h>

static int  TOP_DIR = -1;
static int  MID_DIR = -1;
static int BOTTOM_DIR = -1;
static int FILE_NUM = -1;

void printf_usage()
{
    fprintf(stderr,"Usage:genfile -t top_dir_num -m mid_dir_num -b bottom_dir_num -f file_num -s start_folder -e end_folder\n");
    return; 
}

void * worker(void* index)
{
    int dest = (int) index;
    char base_folder[256] = {0};
    int i,j;

    char err_msg[1024];
    if(dest > TOP_DIR*MID_DIR)
    {
        printf("too many thread for work for this \n");
        return NULL;
    }

    int top = dest / MID_DIR;
    int mid = dest % MID_DIR;

    getcwd(base_folder,256);

    char base[256] = {0};
    snprintf(base,256,"%s/%d/%d",base_folder,top,mid);
    printf("%d %d begin ! I will take charge of folder %s\n",(int)time(NULL),dest,base);

    int ret ; 
    int calc = 0;
    int open_failed = 0;
    int trunc_failed = 0;
    for( i = 0 ; i <BOTTOM_DIR;i++)
    {
        for(j = 0 ; j < FILE_NUM;j++)
        {
            char filename[256] = {0};
            snprintf(filename,256,"%s/%d/%d",base,i,j);
            int fd = open(filename,O_RDWR|O_CREAT|O_EXCL, S_IRWXU|S_IRGRP|S_IWGRP|S_IROTH);
            if(fd < 0)
            {
                if(errno != EEXIST)
                {
                    fprintf(stderr,"failed to create %s (%s)\n",filename,strerror_r(errno,err_msg,1024));
                    open_failed++;
                }
                continue;
            }

            ret  = ftruncate(fd,1024);
            if(ret != 0)
            {
                fprintf(stderr,"failed to truncate  %s to 1KB (%s)\n",filename,strerror_r(errno,err_msg,1024));
                close(fd);
                trunc_failed++;
                continue;
            }
            close(fd);
            calc++;
        }
    }
    fprintf(stdout,"%d %d Done ! I create %d files in %s,open failed %d, truncate failed %d\n",
            (int)time(NULL),dest,calc,base,open_failed,trunc_failed);
    return NULL;
}



int main(int argc , char* argv[])
{
    int i;
    int long_index = 0;
    int opt;
    int begin = -1;
    int end = -1;
    int ret ; 


    static struct option long_option[] = {
        {"top_dir_num",required_argument,0,'t'},
        {"mid_dir_num",required_argument,0,'m'},
        {"bottom_dir_num",required_argument,0,'b'},
        {"file_num",required_argument,0,'f'},
        {"start_folder",required_argument,0,'s'},
        {"end_folder",required_argument,0,'e'},
    };


    while((opt = getopt_long(argc, argv, "t:m:b:f:s:e:", long_option, &long_index)) != -1)
    {
        switch(opt)
        {
        case 't':
            TOP_DIR=atoi(optarg);
            break;
        case 'm':
            MID_DIR = atoi(optarg);
            break;
        case 'b':
            BOTTOM_DIR = atoi(optarg);
            break;
        case 'f':
            FILE_NUM = atoi(optarg);
            break;
        case 's':
            begin = atoi(optarg);
            break;
        case 'e':
            end = atoi(optarg);
            break;
        default:
            printf_usage();
            exit(1);
        }
    }

    if(TOP_DIR == -1
       || MID_DIR == -1
       || BOTTOM_DIR == -1
       || FILE_NUM == -1
       || begin == -1
       || end == -1)
    {
        printf_usage();
        exit(1);
    }

    if(begin >= (TOP_DIR*MID_DIR) || end > (TOP_DIR*MID_DIR)) 
    {
        fprintf(stderr,"start_folder or end_folder should not bigger than TOP_DIR*MID_DIR\n");
        exit(2);
    }

    int nr_thr = end - begin;
    if(nr_thr < 0 )
    {
        fprintf(stderr,"NR_END must bigger than NR_BEGIN\n");
        exit(2);
    }
    pthread_t* tid = (pthread_t*)malloc(nr_thr*sizeof(pthread_t));
    if(tid == NULL)
    {
        printf("failed to malloc space for tid\n");
        exit(1);
    }

    for(i = begin ; i < end;i++)
    {
        int index = i - begin;
        ret = pthread_create(&tid[index],NULL,worker,(void*) i );
        if(ret != 0 )
        {
            fprintf(stderr,"failed to create thread %d\n",index);
            break;
        }
    }


    for(--i;i >= begin;i--)
    {     
        pthread_join(tid[i-begin],NULL);
    }

    free(tid);
    return 0;
}
