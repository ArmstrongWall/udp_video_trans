/*                                                                                                                              
 *创建日期：2017.5.28
  功能：UDP服务器
  作者：JohnnyWang
  log： 2017.5.28 实现接收客户端数据
        2017.6.10 实现连续接受图像
 * */
#include <sys/types.h> /* See NOTES */
#include <sys/socket.h>
#include <netinet/in.h>//使用struct sockaddr_in需包含的头文件
#include <arpa/inet.h>//使用inet_ntoa网络地址转换成字符串函数需包含的头文件 
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <opencv2/core/core.hpp>      
#include <opencv2/highgui/highgui.hpp>  

#define     portnum         8888//自定义网络端口
#define     BLOCKSIZE       38400       //图像拆包的块大小
#define     IMAGE_HIGHT     480
#define     IMAGE_WIDETH    640
#define     FRAMESIZE       IMAGE_WIDETH*IMAGE_HIGHT*3
#define     PACKSIZE        sizeof(data_pack)
#define     PACK_NUM_SUM    FRAMESIZE/BLOCKSIZE


struct udptransbuf//包格式
{  
      char buf[BLOCKSIZE];//存放数据的变量                                       
      char flag;//标志
} data_pack;


int main()
{
    //FILE *  stream;
    int     sockfd;//定义文件描述符,新的客户机文件描述符
    struct  sockaddr_in server_addr;//主机地址属性
    struct  sockaddr_in client_addr;//客户机地址
    int     nbyte;
    int     addrlen,len=0;
    struct  timeval  time_start,time_stop;
    float   time_use,time_sum;
    int     flag=0,pack_num=0;

    //创建套接字
    //socket函数的两个参数一个是确定套接字通信域 
    //AF_INET  ipv4通信域 
    //AF_INET6 ipv6通信域
    //第二个是套接字类型
    //SOCK_DGRAM  长度固定，无连接的不可靠的报文传递 UDP
    //SOCK_RAW    IP协议的数据报接口
    //SOCK_STREAM 有序、可靠、双向字节流  TCP
    if((sockfd=socket(AF_INET,SOCK_DGRAM,0)) == -1)//如果创建错误，就返回
    {
        printf("create socket error\r\n");
        exit(1);
    }

    //设置要绑定的地址
    bzero(&server_addr,sizeof(struct sockaddr_in));//将地址结构体先清零
    server_addr.sin_family = AF_INET;       //网络域
    server_addr.sin_port   = htons(portnum);//将两个字节的端口转换成网络字节序
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);//将任意的IP地址赋予数据段

    //绑定地址
    bind(sockfd,(struct sockaddr *)(&server_addr),sizeof(struct sockaddr));
    //第二个参数一般先使用sockaddr_in（便于定义结构体成员变量），然后在强制转化，能够更方便
    
    
    addrlen = sizeof(struct sockaddr);
    
    printf("start\r\n");

    /*if((stream = fopen("test.jpg","w"))==NULL)
    {
        printf("file open fialed\r\n");
        exit(-1);
    }*/


    char* img;
    int count = 0,time_count = 1;
    img = (char*)malloc(FRAMESIZE);

    IplImage* frame;
    cvNamedWindow("window",CV_WINDOW_AUTOSIZE);

    struct timeval  nowtime;
    
    while(1)
    {

        //gettimeofday(&time_start,NULL);
        //for(pack_num = 0;pack_num < 8; pack_num++)
        nbyte=recvfrom(sockfd , (char*)(&data_pack), sizeof(data_pack) , 0 , (struct sockaddr*)(&client_addr) , &addrlen);
            //recvfrom函数的第三个参数设置为MSG_DONTWAIT即设置为非阻塞，不然就是一直阻塞的函数
        if(nbyte == PACKSIZE)
        {
            count = count + data_pack.flag;
            memcpy( img + pack_num*BLOCKSIZE, data_pack.buf, BLOCKSIZE);	 
            pack_num ++;
            if(pack_num == PACK_NUM_SUM)
            {
                pack_num = 0;
             }

            //printf("%d\r\n",pack_num);

        }

        if(data_pack.flag==2)  //data.flag==2是一帧中的最后一个数据块  
            {   
                if(count== PACK_NUM_SUM + 1)  
                {       
                    gettimeofday(&nowtime,NULL);   
                    printf("now time is %d.%d\r\n",nowtime.tv_sec,nowtime.tv_usec);
                         
                    //printf("recv  %d flag= %d\r\n",nbyte,data_pack.flag);
                    CvMat cvmat = cvMat(480, 640, CV_8UC3, (void*)img);
                    frame = cvDecodeImage(&cvmat, 1);//解码，这一步将数据转换为IplImage格式
                    cvShowImage("window", frame);//显示图像
                    cvWaitKey(400);
                    cvReleaseImage(&frame);//释放图像空间
                    //cvSaveImage("image.jpg", frame, 0);
                    bzero(img,FRAMESIZE);
                    //break;
                }
                else
                {
                    count = 0;//清零
                }
            } 

    }

    printf("time use %f \r\n",time_sum);

   /* 
    frame = cvDecodeImage(&cvmat, 1);
    cvNamedWindow("window",CV_WINDOW_AUTOSIZE);
    cvShowImage("window", frame);
    cvWaitKey(1000);
    cvSaveImage("image.jpg", frame, 0);*/
    

    //stream = fopen("1.jpg","wb");
    //fwrite(img,1,FRAMESIZE,stream);

    free(img); 
    close(sockfd);
    
    //fclose(stream);

    return 0;

}
