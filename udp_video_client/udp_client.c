/*
 *创建日期：2017.5.20
  功能：UDP视频传输客户端

1.打开设备文件。 int fd=open(”/dev/video2″,O_RDWR);
2.取得设备的capability，看看设备具有什么功能，比如是否具有视频输入,或者音频输入输出等。VIDIOC_QUERYCAP,struct v4l2_capability
3.设置视频的制式和帧格式，制式包括PAL，NTSC，帧的格式个包括宽度和高度等。VIDIOC_S_STD,VIDIOC_S_FMT,struct v4l2_std_id,struct v4l2_format
4.向驱动申请帧缓冲，一般不超过5个。struct v4l2_requestbuffers
5.将申请到的帧缓冲映射到用户空间，这样就可以直接操作采集到的帧了，而不必去复制。mmap
6.将申请到的帧缓冲全部入队列，以便存放采集到的数据.VIDIOC_QBUF,struct v4l2_buffer
7.开始视频的采集。VIDIOC_STREAMON
8.出队列以取得已采集数据的帧缓冲，取得原始采集数据。VIDIOC_DQBUF
9.将缓冲重新入队列尾,这样可以循环采集。VIDIOC_QBUF
10.停止视频的采集。VIDIOC_STREAMOFF
11.关闭视频设备。close(fd);


  作者：JohnnyWang
  log： 2017.5.29 实现向服务器发送文件
		2017.6.03 增加视频采集程序



* */

#include <sys/types.h> /* See NOTES */
#include <sys/socket.h>
#include <netinet/in.h>//定义了sockaddr_in结构
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/types.h>
#include <linux/videodev2.h>
#include "v4l2grab.h"
#include <time.h>
#include <sys/time.h>
//#include <opencv2/core/core.hpp>
//#include <opencv2/highgui/highgui.hpp>
//#include <opencv2/imgcodecs.hpp>



#define     portnum         8888//自定义网络端口
#define     BUFFER_SIZE     640*480
#define     FILE_VIDEO 	    "/dev/video0" //摄像头端口号
#define     IMAGEWIDTH      640
#define     IMAGEHEIGHT     480
#define     BLOCKSIZE       38400       //图像拆包的块大小
#define     FRAMESIZE       IMAGEWIDTH*IMAGEHEIGHT*3
#define     PACKSIZE        sizeof(data_pack)
#define     PACK_NUM_SUM    FRAMESIZE/BLOCKSIZE



#define  TRUE	1
#define  FALSE	0

static   int        video_fd;
static   int        sockfd;//定义文件描述符,新的客户机文件描述符
static   struct     sockaddr_in server_addr;//主机地址属性
static   struct     v4l2_capability   cap;
struct 				      v4l2_fmtdesc fmtdesc;
struct 				      v4l2_format fmt,fmtack;
struct 				      v4l2_streamparm setfps;
struct 				      v4l2_requestbuffers req;//申请缓存结构体
struct 			        v4l2_buffer buf;//缓存结构体
enum   				      v4l2_buf_type type;
char 				        *video_data_buffer;//定义像素缓存


/*struct buffer   //图像缓存
{
	void * start;
	unsigned int length;
} * buffers;*/

char * video_buffers;

struct udptransbuf//包格式
{
	char buf[BLOCKSIZE];//存放数据的变量
	char flag;//标志
} data_pack;


void mysleep(int isec,int iusec)
{
    struct timeval timer;
    timer.tv_sec  = isec;
    timer.tv_usec = iusec;
    select(0,NULL,NULL,NULL,&timer);
}

int init_v4l2(void)
{
	int i;
	int ret = 0;

	//opendev打开设备
	if ((video_fd = open(FILE_VIDEO, O_RDWR)) == -1)
	{
		printf("Error opening V4L interface\n");
		return (FALSE);
	}

	//query cap
	if (ioctl(video_fd, VIDIOC_QUERYCAP, &cap) == -1)
	{
		printf("Error opening device %s: unable to query device.\n",FILE_VIDEO);
		return (FALSE);
	}
	else
	{
     	printf("driver:\t\t%s\n",cap.driver);
     	printf("card:\t\t%s\n",cap.card);
     	printf("bus_info:\t%s\n",cap.bus_info);
     	printf("version:\t%d\n",cap.version);
     	printf("capabilities:\t%x\n",cap.capabilities);

     	if ((cap.capabilities & V4L2_CAP_VIDEO_CAPTURE) == V4L2_CAP_VIDEO_CAPTURE)
     	{
			printf("Device %s: supports capture.\n",FILE_VIDEO);
		}

		if ((cap.capabilities & V4L2_CAP_STREAMING) == V4L2_CAP_STREAMING)
		{
			printf("Device %s: supports streaming.\n",FILE_VIDEO);
		}
	}

	//emu all support fmt  设置图像模式
	fmtdesc.index=0;
	fmtdesc.type=V4L2_BUF_TYPE_VIDEO_CAPTURE;
	printf("Support format:\n");
	while(ioctl(video_fd,VIDIOC_ENUM_FMT,&fmtdesc)!=-1)
	{
		printf("\t%d.%s\n",fmtdesc.index+1,fmtdesc.description);
		fmtdesc.index++;
	}

    //set fmt
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_MJPEG;//设置为MJPEG格式
	fmt.fmt.pix.height = IMAGEHEIGHT;
	fmt.fmt.pix.width = IMAGEWIDTH;
	fmt.fmt.pix.field = V4L2_FIELD_INTERLACED;

	if(ioctl(video_fd, VIDIOC_S_FMT, &fmt) == -1)
	{
		printf("Unable to set format\n");
		return FALSE;
	}
	if(ioctl(video_fd, VIDIOC_G_FMT, &fmt) == -1)
	{
		printf("Unable to get format\n");
		return FALSE;
	}
	{
     	printf("fmt.type:\t\t%d\n",fmt.type);
     	printf("pix.pixelformat:\t%c%c%c%c\n",fmt.fmt.pix.pixelformat & 0xFF, (fmt.fmt.pix.pixelformat >> 8) & 0xFF,(fmt.fmt.pix.pixelformat >> 16) & 0xFF, (fmt.fmt.pix.pixelformat >> 24) & 0xFF);
     	printf("pix.height:\t\t%d\n",fmt.fmt.pix.height);
     	printf("pix.width:\t\t%d\n",fmt.fmt.pix.width);
     	printf("pix.field:\t\t%d\n",fmt.fmt.pix.field);
	}
	//set fps设置帧率,numerator/denominator秒一帧
	setfps.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	setfps.parm.capture.timeperframe.numerator = 1;
	setfps.parm.capture.timeperframe.denominator = 30;
    if(ioctl(video_fd, VIDIOC_S_PARM, &setfps)==-1)
    {
        printf("Set fps falied\r\n");
        return FALSE;
    }

	printf("init %s \t[OK]\n",FILE_VIDEO);

	return TRUE;
}

int send_video(void)
{
    int     pack_num = 0,time_count = 1;
    struct  timeval time_start,time_stop;
    double  time_use,time_sum;
	  //IplImage* frame;
	  //cvNamedWindow("window",CV_WINDOW_AUTOSIZE);

    gettimeofday(&time_start,NULL);
    while(1)
    {
        //gettimeofday(&time_start,NULL);
        ioctl (video_fd, VIDIOC_DQBUF, &buf);//帧缓存出队 VIDIOC_QBUF  从设备获取一帧视频数据

        /*CvMat cvmat = cvMat(480,640,CV_8UC3,(void*)buffers);
        frame = cvDecodeImage(&cvmat,1);
        cvShowImage("window",frame);
        cvWaitKey(1);
        cvReleaseImage(&frame);*/


        //发送数据到服务器就在此处进行
        //process_image (buffers[buf.index].start);
        for(pack_num = 0; pack_num < PACK_NUM_SUM; pack_num++)//一个数据包拆成8块，每块BLOCKSIZE大小
        {
            //gettimeofday(&time_start,NULL);
            memcpy(data_pack.buf, video_buffers + pack_num*BLOCKSIZE,BLOCKSIZE);//*dst,*src,len
            if(pack_num == PACK_NUM_SUM -1 )    //标识一帧中最后一个数据包
            {
                data_pack.flag = 2;//标记特殊符号
            }
            else
            {
                data_pack.flag = 1;
            }

            //gettimeofday(&time_stop,NULL);
            //gettimeofday(&time_start,NULL);
            sendto(sockfd,(char*)(&data_pack),sizeof(data_pack),0,(struct sockaddr*)(&server_addr),sizeof(struct sockaddr));

            //gettimeofday(&time_stop,NULL);
            //time_use = 1000000*(time_start.tv_sec-time_stop.tv_sec)+(time_start.tv_usec-time_stop.tv_usec);
            //time_sum = time_sum + time_use/1000000;

            //printf("time spend = %f\r\n",time_use/1000000);
            bzero(&data_pack,sizeof(data_pack));
        //printf("send one image..%d\r\n",len);
        }
        ioctl(video_fd, VIDIOC_QBUF, &buf);//环形队列，帧缓存再次入队 以便重复利用VIDIOC_DQBUF  将视频缓冲区归回给设备
        //printf("send one image..%d\r\n",buf.index);
        //mysleep(1,0);
        //gettimeofday(&time_stop,NULL);
        //time_use = 1000000*(time_start.tv_sec-time_stop.tv_sec)+(time_start.tv_usec-time_stop.tv_usec);
        //time_sum = time_sum + time_use/1000000;
        //time_count ++;
        //if(time_count == 100)
            //break;
    }

    gettimeofday(&time_stop,NULL);

    time_use = 1000000*(time_start.tv_sec-time_stop.tv_sec)+(time_start.tv_usec-time_stop.tv_usec);
    time_use = time_use/1000000;

    printf("collect %d frame , time spend = %f\r\n",time_count,time_use/100);
    return(TRUE);

}

int v4l2_grab(void)
{
	unsigned int n_buffers;

	//request for 4 buffers  申请四个缓存
	req.count= 1 ;//只申请一个缓存，图像会流畅的多
	req.type=V4L2_BUF_TYPE_VIDEO_CAPTURE;
	req.memory=V4L2_MEMORY_MMAP;
	if(ioctl(video_fd,VIDIOC_REQBUFS,&req)==-1)
	{
		printf("request for buffers error\n");
	}


	//mmap for buffers
	video_buffers = (char*)malloc(req.count*sizeof (*video_buffers));
	if (!video_buffers)
	{
		printf ("Out of memory\n");
		return(FALSE);
	}

	//将所有的帧缓存初始化
	for (n_buffers = 0; n_buffers < req.count; n_buffers++)
	{
		buf.type =   V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_MMAP;
		buf.index =  n_buffers;
		//query buffers
		if (ioctl (video_fd, VIDIOC_QUERYBUF, &buf) == -1)
		{
			printf("query buffer error\n");
			return(FALSE);
		}

		//buffers[n_buffers].length = buf.length;
		//map完成内存映射
		video_buffers =(char*) mmap(NULL,buf.length,PROT_READ |PROT_WRITE, MAP_SHARED, video_fd, buf.m.offset);

		if (video_buffers  == MAP_FAILED)
		{
			printf("buffer map error\n");
			return(FALSE);
		}
	}

	//queue
	for (n_buffers = 0; n_buffers < req.count; n_buffers++)
	{
		buf.index = n_buffers;
		ioctl(video_fd, VIDIOC_QBUF, &buf);//将缓存入队
	}
	/*
	  buf.0 <-- buf.1 <-- buf.2 <-- buf.3 <-- buf.4
	*/


	type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	ioctl (video_fd, VIDIOC_STREAMON, &type);//开始流I/O操作，capture or output device

  if(!send_video())	//发送图像
      return FALSE;

}

int close_v4l2(void)
{
     if(video_fd != -1)
     {
         close(video_fd);
         return (TRUE);
     }
     return (FALSE);
}

int init_socket(char* argv)
{
    //int     sockfd;//定义文件描述符,新的客户机文件描述符
    //struct  sockaddr_in server_addr;//主机地址属性
    char    buffer[BUFFER_SIZE]; //发送数据缓存
    int     len=0;

    printf("ip is %s\r\n",argv);

	 //创建套接字
    if((sockfd=socket(AF_INET,SOCK_DGRAM,0)) == -1)//如果创建错误，就返回
    {
        printf("create socket error\r\n");
        exit(1);
    }
    //设置要绑定的地址
    bzero(&server_addr,sizeof(struct sockaddr_in));//将地址结构体先清零
    server_addr.sin_family = AF_INET; //网络域
    server_addr.sin_port   = htons(portnum);//将两个字节的端口转换成网络字节序
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);//将任意的IP地址
    inet_aton(argv,&server_addr.sin_addr);

}

int main(int argc, char **argv)
{


    //FILE * stream;

    //stream = fopen("1.jpg","wb");
	 /*
    if(!stream)
	{
		printf("open error\n");
		return(FALSE);
	}*/

    if(argc !=2)
    {
        printf("Usage: %s server_ip\n",argv[0]);
        exit(-1);
    }


	init_socket(argv[1]);//初始化UDP套接字端口

	if(init_v4l2() == FALSE) //初始化v4l2
	{
     	return(FALSE);
	}

    v4l2_grab();

    //fwrite(buffers[buf.index].start, 1, 640*480, stream);
    //printf("save picture OK\n");
    //fclose(stream);
    printf("传输数据结束.....\r\n");
    //4.关闭连接
    close(sockfd);
    close_v4l2();
    return 0;

}

