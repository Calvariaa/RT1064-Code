#include "headfile.h"
#include "encoder.h"
#include "display.h"
#include "imgproc.h"
#include "attitude_solution.h"


extern int clip(int x, int low, int up);

void display_entry(void *parameter)
{
    ips200_clear(BLACK);
    while(1)
    {
        //ips200_displayimage032_zoom(mt9v03x_csi_image[0], MT9V03X_CSI_W, MT9V03X_CSI_H, MT9V03X_CSI_W/2, MT9V03X_CSI_H/2);//��С��ʾ�������ͬ����ʾһЩ����
    }
    
}






void display_init(void)
{
    rt_thread_t tid;
    
    //��ʼ����Ļ
    ips200_init();
    
    //������ʾ�߳� ���ȼ�����Ϊ31
    tid = rt_thread_create("display", display_entry, RT_NULL, 1024, 31, 30);
    
    //������ʾ�߳�
    if(RT_NULL != tid)
    {
        rt_thread_startup(tid);
    }
}