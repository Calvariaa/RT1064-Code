/*********************************************************************************************************************
 * COPYRIGHT NOTICE
 * Copyright (c) 2019,��ɿƼ�
 * All rights reserved.
 * ��������QQȺ��һȺ��179029047(����)  ��Ⱥ��244861897
 *
 * �����������ݰ�Ȩ������ɿƼ����У�δ��������������ҵ��;��
 * ��ӭ��λʹ�ò������������޸�����ʱ���뱣����ɿƼ��İ�Ȩ������
 *
 * @file       		main
 * @company	   		�ɶ���ɿƼ����޹�˾
 * @author     		��ɿƼ�(QQ3184284598)
 * @version    		�鿴doc��version�ļ� �汾˵��
 * @Software 		IAR 8.3 or MDK 5.28
 * @Target core		NXP RT1064DVL6A
 * @Taobao   		https://seekfree.taobao.com/
 * @date       		2019-04-30
 ********************************************************************************************************************/


//�����Ƽ�IO�鿴Projecct�ļ����µ�TXT�ı�


//���µĹ��̻��߹����ƶ���λ�����ִ�����²���
//��һ�� �ر��������д򿪵��ļ�
//�ڶ��� project  clean  �ȴ��·�����������

//���ش���ǰ������Լ�ʹ�õ��������ڹ���������������Ϊ�Լ���ʹ�õ�

#include "headfile.h"

#include "timer_pit.h"
#include "encoder.h"
#include "buzzer.h"
#include "button.h"
#include "motor.h"
#include "elec.h"
#include "openart_mini.h"
#include "smotor.h"

#include "debugger.h"
#include "imgproc.h"

#include <stdio.h>

rt_sem_t camera_sem;

debugger_image_t img0 = CREATE_DEBUGGER_IMAGE("raw", MT9V03X_CSI_W, MT9V03X_CSI_H, NULL);
image_t img_raw = DEF_IMAGE(NULL, MT9V03X_CSI_W, MT9V03X_CSI_H);

int thres_value = 160;
debugger_param_t p0 = CREATE_DEBUGGER_PARAM("thres", 0, 255, 160, &thres_value);

int delta_value = 5;
debugger_param_t p1 = CREATE_DEBUGGER_PARAM("delta", 0, 255, 5, &delta_value);

int kp100 = 2500;
debugger_param_t p2 = CREATE_DEBUGGER_PARAM("kp100", -10000, 10000, 2500, &kp100);

int car_width = 50;
debugger_param_t p3 = CREATE_DEBUGGER_PARAM("car_width", 0, 250, 50, &car_width);

int begin_y = 225;
debugger_param_t p4 = CREATE_DEBUGGER_PARAM("begin_y", 0, MT9V03X_CSI_H, 225, &begin_y);

int pixel_per_meter = 102;
debugger_param_t p5 = CREATE_DEBUGGER_PARAM("pixel_per_meter", 0, MT9V03X_CSI_H, 102, &pixel_per_meter);

bool show_bin = false;
debugger_option_t opt0 = CREATE_DEBUGGER_OPTION("show_bin", false, &show_bin);

bool show_line = false;
debugger_option_t opt1 = CREATE_DEBUGGER_OPTION("show_line", false, &show_line);

bool show_approx = false;
debugger_option_t opt2 = CREATE_DEBUGGER_OPTION("show_approx", false, &show_approx);

AT_DTCM_SECTION_ALIGN(int pts1[MT9V03X_CSI_H][2], 8);
AT_DTCM_SECTION_ALIGN(int pts2[MT9V03X_CSI_H][2], 8);
AT_DTCM_SECTION_ALIGN(float pts1_inv[MT9V03X_CSI_H][2], 8);
AT_DTCM_SECTION_ALIGN(float pts2_inv[MT9V03X_CSI_H][2], 8);
AT_DTCM_SECTION_ALIGN(float line1[MT9V03X_CSI_H][2], 8);
AT_DTCM_SECTION_ALIGN(float line2[MT9V03X_CSI_H][2], 8);
AT_DTCM_SECTION_ALIGN(float pts_road[MT9V03X_CSI_H][2], 8);

#define DEBUG_PIN   D4

void cross_dot(const float x[3], const float y[3], float out[3]){
    out[0] = x[1]*y[2]-x[2]*y[1];
    out[1] = x[2]*y[0]-x[0]*y[2];
    out[2] = x[0]*y[1]-x[1]*y[0];
}

float K[3][3] = {
    {1.241016008672135e+02,                     0, 1.864993169331173e+02},
    {                    0, 1.241477111009435e+02, 1.174203927052049e+02},
    {                    0,                     0,                     1},
};

float D[5] = {0.141323308507051,-0.139460262349753,-0.002478722783453,9.106538935854130e-04,0.027173897921792};

extern int clip(int x, int low, int up);
extern float mapx[240][376];
extern float mapy[240][376];
extern pid_param_t servo_pid;

#define ENCODER_PER_METER   (5800.)

#define ROAD_HALF_WIDTH  (0.225)

void track_leftline(float line[][2], int num, float tracked[][2]){
    for(int i=2; i<num-2; i++){
        float dx = line[i+2][0] - line[i-2][0];
        float dy = line[i+2][1] - line[i-2][1];
        float dn = sqrt(dx*dx+dy*dy);
        dx /= dn;
        dy /= dn;
        tracked[i][0] = line[i][0] - dy * pixel_per_meter * ROAD_HALF_WIDTH;
        tracked[i][1] = line[i][1] + dx * pixel_per_meter * ROAD_HALF_WIDTH;
    }
}

void track_rightline(float line[][2], int num, float tracked[][2]){
    for(int i=2; i<num-2; i++){
        float dx = line[i+2][0] - line[i-2][0];
        float dy = line[i+2][1] - line[i-2][1];
        float dn = sqrt(dx*dx+dy*dy);
        dx /= dn;
        dy /= dn;
        tracked[i][0] = line[i][0] + dy * pixel_per_meter * ROAD_HALF_WIDTH;
        tracked[i][1] = line[i][1] - dx * pixel_per_meter * ROAD_HALF_WIDTH;
    }
}

enum {
    NORMAL, TURN, CROSS, YROAD
} road_type1 = NORMAL, road_type2 = NORMAL;
float da1 = 0, da2 = 0;
float error, dx, dy;

enum {
    TRACK_LEFT, TRACK_RIGHT
} track_type = TRACK_RIGHT;

uint32_t circle_time = 0;
int64_t circle_encoder = 1ll<<63ll;

enum {
    CIRCLE_LEFT_BEGIN, CIRCLE_LEFT_RUNNING, CIRCLE_LEFT_END,
    CIRCLE_RIGHT_BEGIN, CIRCLE_RIGHT_RUNNING, CIRCLE_RIGHT_END,
    CIRCLE_NONE
} circle_type = CIRCLE_NONE;

int anchor_num = 80, num1, num2;

int main(void)
{
	camera_sem = rt_sem_create("camera", 0, RT_IPC_FLAG_FIFO);

    mt9v03x_csi_init();
    icm20602_init_spi();
    
    encoder_init();
//    buzzer_init();
//    button_init();
    motor_init();
//    elec_init();
//    openart_mini();
    smotor_init();
    timer_pit_init();
    seekfree_wireless_init();
    
    pit_init();
    pit_start(PIT_CH3);
    
    // 
    gpio_init(DEBUG_PIN, GPI, 0, GPIO_PIN_CONFIG);
    
    debugger_init();
    debugger_register_image(&img0);
    debugger_register_param(&p0);
    debugger_register_param(&p1);
    debugger_register_param(&p2);
    debugger_register_param(&p3);
    debugger_register_param(&p4);
    debugger_register_param(&p5);
    debugger_register_option(&opt0);
    debugger_register_option(&opt1);
    debugger_register_option(&opt2);
    
    uint32_t t1, t2;
    
    EnableGlobalIRQ(0);
    while (1)
    {		//�ȴ�����ͷ�ɼ����
        rt_sem_take(camera_sem, RT_WAITING_FOREVER);
        img_raw.data = mt9v03x_csi_image[0];
        img0.buffer = mt9v03x_csi_image[0];
        //��ʼ��������ͷͼ��
        t1 = pit_get_us(PIT_CH3);
        if(show_bin) {
            threshold(&img_raw, &img_raw, thres_value);
            int pt0[2] = {img_raw.width/2-car_width, begin_y};
            int pt1[2] = {0, begin_y};
            draw_line(&img_raw, pt0, pt1, 0);
            pt0[0] = img_raw.width/2+car_width;
            pt1[0] = img_raw.width-1;
            draw_line(&img_raw, pt0, pt1, 0);
        } else {
            //thres_value = getOSTUThreshold(&img_raw, 100, 200);
            
            int x1=img_raw.width/2-car_width, y1=begin_y;
            num1=sizeof(pts1)/sizeof(pts1[0]);
            for(;x1>0; x1--) if(AT_IMAGE(&img_raw, x1, y1) < thres_value 
                             || ((int)AT_IMAGE(&img_raw, x1, y1) - (int)AT_IMAGE(&img_raw, x1-1, y1)) > delta_value * 2) break;
            if(AT_IMAGE(&img_raw, x1+1, y1) >= thres_value) findline_lefthand_with_thres(&img_raw, thres_value, delta_value, x1+1, y1, pts1, &num1);
            else num1 = 0;
            
            int x2=img_raw.width/2+car_width, y2=begin_y;
            num2=sizeof(pts2)/sizeof(pts2[0]);
            for(;x2<img_raw.width-1; x2++) if(AT_IMAGE(&img_raw, x2, y2) < thres_value
                             || ((int)AT_IMAGE(&img_raw, x2, y2) - (int)AT_IMAGE(&img_raw, x2+1, y2)) > delta_value * 2) break;
            if(AT_IMAGE(&img_raw, x2-1, y2) >= thres_value) findline_righthand_with_thres(&img_raw, thres_value, delta_value, x2-1, y2, pts2, &num2);
            else num2 = 0;
            
            for(int i=0; i<num1; i++) {
                pts1_inv[i][0] = mapx[pts1[i][1]][pts1[i][0]];
                pts1_inv[i][1] = mapy[pts1[i][1]][pts1[i][0]];
            }
            for(int i=0; i<num2; i++) {
                pts2_inv[i][0] = mapx[pts2[i][1]][pts2[i][0]];
                pts2_inv[i][1] = mapy[pts2[i][1]][pts2[i][0]];
            }

            int line1_num=sizeof(line1)/sizeof(line1[0]);
            int line2_num=sizeof(line2)/sizeof(line2[0]);
            if(num1 > 10) approx_lines_f(pts1_inv, num1, 5, line1, &line1_num);
            else line1_num = 0;
            if(num2 > 10) approx_lines_f(pts2_inv, num2, 5, line2, &line2_num);
            else line2_num = 0;
            
            float line1_dx1, line1_dy1, line1_len1, line1_dx2, line1_dy2, line1_len2;
            int line1_i = 0;
            for(int i=0; i<num1-1; i++){
                float dx = line1[i][0]-line1[i+1][0];
                float dy = line1[i][1]-line1[i+1][1];
                float len = sqrt(dx*dx+dy*dy);
                if(len / pixel_per_meter < 0.05) continue;
                if(line1_i == 0){
                    line1_dx1 = dx;
                    line1_dy1 = dy;
                    line1_len1 = len;
                    line1_i = 1;
                }else{
                    line1_dx2 = dx;
                    line1_dy2 = dy;
                    line1_len2 = len;
                    break;
                }
            }
            
            float line2_dx1, line2_dy1, line2_len1, line2_dx2, line2_dy2, line2_len2;
            int line2_i = 0;
            for(int i=0; i<num2-1; i++){
                float dx = line2[i][0]-line2[i+1][0];
                float dy = line2[i][1]-line2[i+1][1];
                float len = sqrt(dx*dx+dy*dy);
                if(len / pixel_per_meter < 0.04) continue;
                if(line2_i == 0){
                    line2_dx1 = dx;
                    line2_dy1 = dy;
                    line2_len1 = len;
                    line2_i = 1;
                }else{
                    line2_dx2 = dx;
                    line2_dy2 = dy;
                    line2_len2 = len;
                    break;
                }
            }
            
            da1 = 0;
            da2 = 0;
            if(line1_i == 1){
                da1 = acos((line1_dx1 * line1_dx2 + line1_dy1 * line1_dy2) / line1_len1 / line1_len2) * 180. / 3.1415;
            }
            if(line2_i == 1){
                da2 = acos((line2_dx1 * line2_dx2 + line2_dy1 * line2_dy2) / line2_len1 / line2_len2) * 180. / 3.1415;
            }
            
            if(line1_len1 / pixel_per_meter > 0.5) road_type1 = NORMAL;
            else if(10 < da1 && da1 < 45) road_type1 = TURN;
            else if(50 < da1 && da1 < 70) road_type1 = YROAD;
            else if(75 < da1 && da1 < 120) road_type1 = CROSS;
            else road_type1 = NORMAL;
            
            if(line2_len1 / pixel_per_meter > 0.5) road_type2 = NORMAL;
            else if(10 < da2 && da2 < 45) road_type2 = TURN;
            else if(50 < da2 && da2 < 70) road_type2 = YROAD;
            else if(75 < da2 && da2 < 120) road_type2 = CROSS;
            else road_type2 = NORMAL;
            
            uint64_t current_encoder = (motor_l.total_encoder + motor_r.total_encoder) / 2;
            
            if(road_type1 == YROAD && road_type2 == YROAD){
                // y-road
                // TODO: stop and wait openart
                
                // just keep turn right
                track_type = TRACK_RIGHT;
            } else if(road_type1 == CROSS && road_type2 == NORMAL){
                // left circle
                if(current_encoder - circle_encoder > ENCODER_PER_METER * 4){
                    circle_encoder = current_encoder;
                    circle_type = CIRCLE_LEFT_BEGIN;
                }
            } else if(road_type1 == NORMAL && road_type2 == CROSS){
                // right circle
                if(current_encoder - circle_encoder > ENCODER_PER_METER * 4){
                    circle_encoder = current_encoder;
                    circle_type = CIRCLE_RIGHT_BEGIN;
                }
            } else if(road_type1 == CROSS && road_type2 == CROSS){
                // cross
                
            }
            
            if(circle_type != CIRCLE_NONE){
                // in circle
                if(circle_type == CIRCLE_LEFT_BEGIN){
                    track_type = TRACK_RIGHT;
                    if(current_encoder - circle_encoder >= ENCODER_PER_METER * 1){
                        circle_type = CIRCLE_LEFT_RUNNING;
                    }
                    anchor_num = 120;
                } else if(circle_type == CIRCLE_LEFT_RUNNING){
                    track_type = TRACK_LEFT;
                    if(current_encoder - circle_encoder >= ENCODER_PER_METER * (1 + 3.14 * 8 / 9)){
                        circle_type = CIRCLE_LEFT_END;
                    }
                    anchor_num = 60;
                } else if(circle_type == CIRCLE_LEFT_END){
                    track_type = TRACK_RIGHT;
                    if(current_encoder - circle_encoder >= ENCODER_PER_METER * (1 + 3.14 * 8 / 9 + 1)){
                        circle_type = CIRCLE_NONE;
                    }
                    anchor_num = 120;
                } else if(circle_type == CIRCLE_RIGHT_BEGIN){
                    track_type = TRACK_LEFT;
                    if(current_encoder - circle_encoder >= ENCODER_PER_METER * 1){
                        circle_type = CIRCLE_RIGHT_RUNNING;
                    }
                    anchor_num = 120;
                } else if(circle_type == CIRCLE_RIGHT_RUNNING){
                    track_type = TRACK_RIGHT;
                    if(current_encoder - circle_encoder >= ENCODER_PER_METER * (1 + 3.14 * 7 / 8)){
                        circle_type = CIRCLE_RIGHT_END;
                    }
                    anchor_num = 60;
                } else if(circle_type == CIRCLE_RIGHT_END){
                    track_type = TRACK_LEFT;
                    if(current_encoder - circle_encoder >= ENCODER_PER_METER * (1 + 3.14 * 7 / 8 + 1)){
                        circle_type = CIRCLE_NONE;
                    }
                    anchor_num = 120;
                }
            } else {
                if(line1_dx2 < 0 && line2_dx2 < 0) track_type = TRACK_LEFT;
                else if(line1_dx2 > 0 && line2_dx2 > 0) track_type = TRACK_RIGHT;
                else track_type = TRACK_RIGHT;
                anchor_num = 120;
            }
            
            int track_num;
            if(track_type == TRACK_LEFT){
                track_leftline(pts1_inv, num1, pts_road);
                track_num = num1;
            }else{
                track_rightline(pts2_inv, num2, pts_road);
                track_num = num2;
            }
            
                       
            //����ͼ��������ģ������֮���λ��ƫ��
            int idx = track_num < anchor_num + 3 ? track_num - 3 : anchor_num;
            dx = (pts_road[idx][0] - img_raw.width / 2.) / pixel_per_meter;
            dy = (img_raw.height - pts_road[idx][1]) / pixel_per_meter;
            error = -atan2f(dx, dy);
            
            //����ƫ�����PD����
            servo_pid.kp = kp100 / 100.;
            float angle = pid_solve(&servo_pid, error);
            angle = MINMAX(angle, -13, 13);
            
            //PD����֮���ֵ����Ѱ������Ŀ���
            smotor1_control(servo_duty(SMOTOR1_CENTER + angle));
            
            // draw
            
            if(show_line){
                clear_image(&img_raw);
                for(int i=0; i<num1; i++){
                    AT_IMAGE(&img_raw, clip(pts1_inv[i][0], 0, img_raw.width-1), clip(pts1_inv[i][1], 0, img_raw.height-1)) = 255;
                }
                for(int i=0; i<num2; i++){
                    AT_IMAGE(&img_raw, clip(pts2_inv[i][0], 0, img_raw.width-1), clip(pts2_inv[i][1], 0, img_raw.height-1)) = 255;
                }
                for(int i=2; i<track_num-2; i++){
                    AT_IMAGE(&img_raw, clip(pts_road[i][0], 0, img_raw.width-1), clip(pts_road[i][1], 0, img_raw.height-1)) = 255;
                }
                for(int y=0; y<img_raw.height; y++){
                    AT_IMAGE(&img_raw, img_raw.width/2, y) = 255;
                }
                
                for(int i=-3; i<=3; i++){
                    AT_IMAGE(&img_raw, (int)pts_road[idx][0]+i, (int)pts_road[idx][1]) = 255;
                }
                for(int i=-3; i<=3; i++){
                    AT_IMAGE(&img_raw, (int)pts_road[idx][0], (int)pts_road[idx][1]+1) = 255;
                }
            }else if(show_approx){
                clear_image(&img_raw);
                int pt0[2], pt1[2];
                for(int i=1; i<line1_num; i++){
                    pt0[0] = line1[i-1][0];
                    pt0[1] = line1[i-1][1];
                    pt1[0] = line1[i][0];
                    pt1[1] = line1[i][1];
                    draw_line(&img_raw, pt0, pt1, 255);
                }
                for(int i=1; i<line2_num; i++){
                    pt0[0] = line2[i-1][0];
                    pt0[1] = line2[i-1][1];
                    pt1[0] = line2[i][0];
                    pt1[1] = line2[i][1];
                    draw_line(&img_raw, pt0, pt1, 255);
                }
            }
        }
        
        
        // print debug information
        uint32_t tmp = pit_get_us(PIT_CH3);
        static uint8_t buffer[64];
        int len = snprintf((char*)buffer, sizeof(buffer), "process=%dus, period=%dus, road1=%d, road2=%d\r\n", 
                            tmp-t1, tmp-t2, road_type1, road_type2);
        t2 = tmp;
        
        if(gpio_get(DEBUG_PIN)) debugger_worker();
        else usb_cdc_send_buff(buffer, len);
    }
}

  



