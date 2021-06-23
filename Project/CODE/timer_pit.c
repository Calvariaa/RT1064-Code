#include "encoder.h"
#include "motor.h"
#include "timer_pit.h"
#include "elec.h"

void timer1_pit_entry(void *parameter)
{    
    //�ɼ�����������
//    get_icm20602_gyro_spi();
//    get_icm20602_accdata_spi();
    
    //�ɼ�����������
    encoder_get();
    
    //���Ƶ��ת��
    motor_control();
}


void timer_pit_init(void)
{
    rt_timer_t timer;
    
    //����һ����ʱ�� ��������
    timer = rt_timer_create("timer1", timer1_pit_entry, RT_NULL, 1, RT_TIMER_FLAG_PERIODIC);
    
    //������ʱ��
    if(RT_NULL != timer)
    {
        rt_timer_start(timer);
    }

    
}