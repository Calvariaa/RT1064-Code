#include "encoder.h"
#include "motor.h"
#include "timer_pit.h"
#include "elec.h"


void timer1_pit_entry(void *parameter)
{    
    //�ɼ�����������
    ICM_getEulerianAngles();
    
    //�ɼ�����������
    encoder_get();
    
    //���Ƶ��ת��
    motor_control();
    
    
    wireless_show();
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