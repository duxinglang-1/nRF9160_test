#ifndef __IMG_H__
#define __IMG_H__

/*Image2Lcd�����ͼ��������֯��ʽΪ��ͼ��ͷ����-��ɫ������-ͼ�����ݡ�
����ɫ/4��/16��/256ɫ����ͼ������ͷ���£�
typedef struct _HEADGRAY
{
   unsigned char scan;
   unsigned char gray;
   unsigned short w;
   unsigned short h;
}HEADGRAY;

scan: ɨ��ģʽ
Bit7: 0:��������ɨ�裬1:��������ɨ�衣 
Bit6: 0:�Զ�����ɨ�裬1:�Ե�����ɨ�衣 
Bit5: 0:�ֽ����������ݴӸ�λ����λ���У�1:�ֽ����������ݴӵ�λ����λ���С� 
Bit4: 0:WORD���͸ߵ�λ�ֽ�˳����PC��ͬ��1:WORD���͸ߵ�λ�ֽ�˳����PC�෴�� 
Bit3~2: ������ 
Bit1~0: [00]ˮƽɨ�裬[01]��ֱɨ�裬[10]����ˮƽ,�ֽڴ�ֱ��[11]���ݴ�ֱ,�ֽ�ˮƽ�� 

gray: �Ҷ�ֵ 
   �Ҷ�ֵ��1:��ɫ��2:�Ļң�4:ʮ���ң�8:256ɫ��12:4096ɫ��16:16λ��ɫ��24:24λ��ɫ��32:32λ��ɫ��

w: ͼ��Ŀ�ȡ�

h: ͼ��ĸ߶ȡ�

��4096ɫ/16λ���ɫ/18λ���ɫ/24λ���ɫ/32λ���ɫ����ͼ������ͷ���£�
typedef struct _HEADCOLOR
{
   unsigned char scan;
   unsigned char gray;
   unsigned short w;
   unsigned short h;
   unsigned char is565;
   unsigned char rgb;
}HEADCOLOR; 

scan��gray��w��h��HEADGRAY�ṹ�е�ͬ����Ա����������ͬ��

is565: ��4096ɫģʽ��Ϊ0��ʾʹ��[16bits(WORD)]��ʽ����ʱͼ��������ÿ��WORD��ʾһ�����أ�Ϊ1��ʾʹ��[12bits(�����ֽ���)]��ʽ����ʱ�������е�ÿ12Bits����һ�����ء�
��16λ��ɫģʽ��Ϊ0��ʾR G B��ɫ������ռ�õ�λ����Ϊ5Bits��Ϊ1��ʾR G B��ɫ������ռ�õ�λ���ֱ�Ϊ5Bits,6Bits,5Bits��
��18λ��ɫģʽ��Ϊ0��ʾ"6Bits in Low Byte"��Ϊ1��ʾ"6Bits in High Byte"��
��24λ��ɫ��32λ��ɫģʽ��is565��Ч��

rgb: ����R G B��ɫ����������˳��rgb��ÿ2Bits��ʾһ����ɫ������[00]��ʾ�հף�[01]��ʾRed��[10]��ʾGreen��[11]��ʾBlue��

��256ɫ���ĵ�ɫ�����ݽṹ���£�
typedef struct _PALENTRY
{
   unsigned char red;
   unsigned char green;
   unsigned char blue;
}PALENTRY;

typedef struct _PALETTE
{
   unsigned short palnum;
   PALENTRY palentry[palnum];
}PALETTE;

����256ɫģʽ�´��ڵ�ɫ�����ݽṹ,��ɫ�����ݽṹ���������ݽṹHEADGRAY֮��
*/

#if 0
//battery(2)
extern unsigned char IMG_BAT_RECT_RED[968];
extern unsigned char IMG_BAT_RECT_WHITE[968];
//back light(6)
extern unsigned char IMG_BKL_DEC_ICON[12878];
extern unsigned char IMG_BKL_INC_ICON[12878];
extern unsigned char IMG_BKL_LEVEL_1[35648];
extern unsigned char IMG_BKL_LEVEL_2[35648];
extern unsigned char IMG_BKL_LEVEL_3[35648];
extern unsigned char IMG_BKL_LEVEL_4[35648];
//blood pressure(7)
extern unsigned char IMG_BP_BG[33158];
extern unsigned char IMG_BP_BIG_ICON_1[3968];
extern unsigned char IMG_BP_BIG_ICON_2[3968];
extern unsigned char IMG_BP_BIG_ICON_3[3968];
extern unsigned char IMG_BP_DOWN_ARRAW[392];
extern unsigned char IMG_BP_ICON_ANI_1[1768];
extern unsigned char IMG_BP_ICON_ANI_2[1768];
extern unsigned char IMG_BP_ICON_ANI_3[1768];
extern unsigned char IMG_BP_SPE_LINE[504];
extern unsigned char IMG_BP_UNIT[1100];
extern unsigned char IMG_BP_UP_ARRAW[586];
//heart rate(6)
extern unsigned char IMG_HR_BG[33548];
extern unsigned char IMG_HR_BIG_ICON_1[7928];
extern unsigned char IMG_HR_BIG_ICON_2[7928];
extern unsigned char IMG_HR_BPM[1988];
extern unsigned char IMG_HR_DOWN_ARRAW[548];
extern unsigned char IMG_HR_ICON_ANI_1[2116];
extern unsigned char IMG_HR_ICON_ANI_2[2116];
extern unsigned char IMG_HR_UP_ARRAW[484];
//idle circle bg
extern unsigned char IMG_IDLE_CIRCLE_BG[36488];
//idle hr(2)
extern unsigned char IMG_IDLE_HR_BG[8720];
extern unsigned char IMG_IDLE_HR_ICON[888];
//idle step
extern unsigned char IMG_IDLE_STEP_BG[8720];
extern unsigned char IMG_IDLE_STEP_ICON[568];
//idle temp(3)
extern unsigned char IMG_IDLE_TEMP_BG[8720];
extern unsigned char IMG_IDLE_TEMP_C_ICON[800];
extern unsigned char IMG_IDLE_TEMP_F_ICON[800];
//idle net mode(2)
extern unsigned char IMG_IDLE_NET_LTEM[344];
extern unsigned char IMG_IDLE_NET_NB[316];
//ota(5)
extern unsigned char IMG_OTA_FAILED_ICON[16208];
extern unsigned char IMG_OTA_FINISH_ICON[16208];
extern unsigned char IMG_OTA_LOGO[7208];
extern unsigned char IMG_OTA_NO[3208];
extern unsigned char IMG_OTA_YES[3208];
//power off(1)
extern unsigned char IMG_PWROFF_BUTTON[32266];
//power on(6)
extern unsigned char IMG_PWRON_ANI_1[13388];
extern unsigned char IMG_PWRON_ANI_2[13388];
extern unsigned char IMG_PWRON_ANI_3[13388];
extern unsigned char IMG_PWRON_ANI_4[13388];
extern unsigned char IMG_PWRON_ANI_5[13388];
extern unsigned char IMG_PWRON_ANI_6[13388];
//reset(13)
extern unsigned char IMG_RESET_ANI_1[16208];
extern unsigned char IMG_RESET_ANI_2[16208];
extern unsigned char IMG_RESET_ANI_3[16208];
extern unsigned char IMG_RESET_ANI_4[16208];
extern unsigned char IMG_RESET_ANI_5[16208];
extern unsigned char IMG_RESET_ANI_6[16208];
extern unsigned char IMG_RESET_ANI_7[16208];
extern unsigned char IMG_RESET_ANI_8[16208];
extern unsigned char IMG_RESET_FAIL[16208];
extern unsigned char IMG_RESET_LOGO[7208];
extern unsigned char IMG_RESET_NO[3208];
extern unsigned char IMG_RESET_SUCCESS[16208];
extern unsigned char IMG_RESET_YES[3208];
//dot gif(4)
extern unsigned char IMG_RUNNING_ANI_1[2114];
extern unsigned char IMG_RUNNING_ANI_2[2114];
extern unsigned char IMG_RUNNING_ANI_3[2114];
//select icon(2)
extern unsigned char IMG_SELECT_ICON_NO[808];
extern unsigned char IMG_SELECT_ICON_YES[808];
//settings(16)
extern unsigned char IMG_SET_BG[12852];
extern unsigned char IMG_SET_INFO_BG[17008];
extern unsigned char IMG_SET_PG2_1[272];
extern unsigned char IMG_SET_PG2_2[272];
extern unsigned char IMG_SET_PG3_1[344];
extern unsigned char IMG_SET_PG3_2[344];
extern unsigned char IMG_SET_PG3_3[344];
extern unsigned char IMG_SET_PG4_1[476];
extern unsigned char IMG_SET_PG4_2[476];
extern unsigned char IMG_SET_PG4_3[476];
extern unsigned char IMG_SET_PG4_4[476];
extern unsigned char IMG_SET_PG5_1[608];
extern unsigned char IMG_SET_PG5_2[608];
extern unsigned char IMG_SET_PG5_3[608];
extern unsigned char IMG_SET_PG5_4[608];
extern unsigned char IMG_SET_PG5_5[608];
extern unsigned char IMG_SET_PG6_1[740];
extern unsigned char IMG_SET_PG6_2[740];
extern unsigned char IMG_SET_PG6_3[740];
extern unsigned char IMG_SET_PG6_4[740];
extern unsigned char IMG_SET_PG6_5[740];
extern unsigned char IMG_SET_PG6_6[740];
extern unsigned char IMG_SET_QR_ICON[1258];
extern unsigned char IMG_SET_SWITCH_OFF_ICON[4152];
extern unsigned char IMG_SET_SWITCH_ON_ICON[4152];
extern unsigned char IMG_SET_TEMP_UNIT_C_ICON[2528];
extern unsigned char IMG_SET_TEMP_UNIT_F_ICON[2528];
//signal(5)
extern unsigned char IMG_SIG_0[1304];
extern unsigned char IMG_SIG_1[1304];
extern unsigned char IMG_SIG_2[1304];
extern unsigned char IMG_SIG_3[1304];
extern unsigned char IMG_SIG_4[1304];
//sleep(14)
extern unsigned char IMG_SLEEP_ANI_1[2968];
extern unsigned char IMG_SLEEP_ANI_2[2968];
extern unsigned char IMG_SLEEP_ANI_3[2968];
extern unsigned char IMG_SLEEP_BIG_H[528];
extern unsigned char IMG_SLEEP_BIG_M[768];
extern unsigned char IMG_SLEEP_BEGIN[1576];
extern unsigned char IMG_SLEEP_END[1576];
extern unsigned char IMG_SLEEP_DEEP_ICON[1576];
extern unsigned char IMG_SLEEP_LIGHT_ICON[1576];
extern unsigned char IMG_SLEEP_H[260];
extern unsigned char IMG_SLEEP_M[372];
extern unsigned char IMG_SLEEP_HOUR[552];
extern unsigned char IMG_SLEEP_MIN[968];
extern unsigned char IMG_SLEEP_LINE[254];
//sos(4)
extern unsigned char IMG_SOS_ANI_1[40880];
extern unsigned char IMG_SOS_ANI_2[40880];
extern unsigned char IMG_SOS_ANI_3[40880];
extern unsigned char IMG_SOS_ANI_4[40880];
//spo2(6)
extern unsigned char IMG_SPO2_ANI_1[2328];
extern unsigned char IMG_SPO2_ANI_2[2328];
extern unsigned char IMG_SPO2_ANI_3[2328];
extern unsigned char IMG_SPO2_BG[33158];
extern unsigned char IMG_SPO2_BIG_ICON_1[5288];
extern unsigned char IMG_SPO2_BIG_ICON_2[5288];
extern unsigned char IMG_SPO2_BIG_ICON_3[5288];
extern unsigned char IMG_SPO2_DOWN_ARRAW[484];
extern unsigned char IMG_SPO2_UP_ARRAW[484];
//step(12)
extern unsigned char IMG_STEP_ANI_1[2328];
extern unsigned char IMG_STEP_ANI_2[2328];
extern unsigned char IMG_STEP_STEP_ICON[2136];
extern unsigned char IMG_STEP_CAL_ICON[2136];
extern unsigned char IMG_STEP_SLEEP_ICON[1744];
extern unsigned char IMG_STEP_UNIT_CN_ICON[400];
extern unsigned char IMG_STEP_UNIT_EN_ICON[1160];
extern unsigned char IMG_STEP_KCAL_CN[820];
extern unsigned char IMG_STEP_KCAL_EN[820];
extern unsigned char IMG_STEP_KM_CN[820];
extern unsigned char IMG_STEP_KM_EN[596];
extern unsigned char IMG_STEP_LINE[205];
//sync(3)
extern unsigned char IMG_SYNC_ERR[42128];
extern unsigned char IMG_SYNC_FINISH[42128];
extern unsigned char IMG_SYNC_LOGO[36458];
//temperature(4)
extern unsigned char IMG_TEMP_BIG_ICON_1[5888];
extern unsigned char IMG_TEMP_BIG_ICON_2[5888];
extern unsigned char IMG_TEMP_BIG_ICON_3[5888];
extern unsigned char IMG_TEMP_ICON_C[2568];
extern unsigned char IMG_TEMP_ICON_F[2864];
extern unsigned char IMG_TEMP_UNIT_C[1756];
extern unsigned char IMG_TEMP_UNIT_F[1756];
extern unsigned char IMG_TEMP_C_BG[31968];
extern unsigned char IMG_TEMP_F_BG[33158];
extern unsigned char IMG_TEMP_DOWN_ARRAW[484];
extern unsigned char IMG_TEMP_UP_ARRAW[484];
//wrist off(1)
extern unsigned char IMG_WRIST_OFF_ICON[23372];
//font_16_num
extern unsigned char IMG_FONT_16_NUM_0[296];
extern unsigned char IMG_FONT_16_NUM_1[296];
extern unsigned char IMG_FONT_16_NUM_2[296];
extern unsigned char IMG_FONT_16_NUM_3[296];
extern unsigned char IMG_FONT_16_NUM_4[296];
extern unsigned char IMG_FONT_16_NUM_5[296];
extern unsigned char IMG_FONT_16_NUM_6[296];
extern unsigned char IMG_FONT_16_NUM_7[296];
extern unsigned char IMG_FONT_16_NUM_8[296];
extern unsigned char IMG_FONT_16_NUM_9[296];
extern unsigned char IMG_FONT_16_COLON[136];
extern unsigned char IMG_FONT_16_DOT[168];
extern unsigned char IMG_FONT_16_PERC[488];
extern unsigned char IMG_FONT_16_SLASH[246];
//font_20_num
extern unsigned char IMG_FONT_20_NUM_0[488];
extern unsigned char IMG_FONT_20_NUM_1[488];
extern unsigned char IMG_FONT_20_NUM_2[488];
extern unsigned char IMG_FONT_20_NUM_3[488];
extern unsigned char IMG_FONT_20_NUM_4[488];
extern unsigned char IMG_FONT_20_NUM_5[488];
extern unsigned char IMG_FONT_20_NUM_6[488];
extern unsigned char IMG_FONT_20_NUM_7[488];
extern unsigned char IMG_FONT_20_NUM_8[488];
extern unsigned char IMG_FONT_20_NUM_9[488];
extern unsigned char IMG_FONT_20_COLON[248];
extern unsigned char IMG_FONT_20_DOT[248];
extern unsigned char IMG_FONT_20_PERC[768];
extern unsigned char IMG_FONT_20_SLASH[360];
//font_24_num
extern unsigned char IMG_FONT_24_NUM_0[680];
extern unsigned char IMG_FONT_24_NUM_1[680];
extern unsigned char IMG_FONT_24_NUM_2[680];
extern unsigned char IMG_FONT_24_NUM_3[680];
extern unsigned char IMG_FONT_24_NUM_4[680];
extern unsigned char IMG_FONT_24_NUM_5[680];
extern unsigned char IMG_FONT_24_NUM_6[680];
extern unsigned char IMG_FONT_24_NUM_7[680];
extern unsigned char IMG_FONT_24_NUM_8[680];
extern unsigned char IMG_FONT_24_NUM_9[680];
extern unsigned char IMG_FONT_24_COLON[344];
extern unsigned char IMG_FONT_24_DOT[344];
extern unsigned char IMG_FONT_24_PERC[1112];
extern unsigned char IMG_FONT_24_SLASH[528];
//font_28_num
extern unsigned char IMG_FONT_28_NUM_0[904];
extern unsigned char IMG_FONT_28_NUM_1[904];
extern unsigned char IMG_FONT_28_NUM_2[904];
extern unsigned char IMG_FONT_28_NUM_3[904];
extern unsigned char IMG_FONT_28_NUM_4[904];
extern unsigned char IMG_FONT_28_NUM_5[904];
extern unsigned char IMG_FONT_28_NUM_6[904];
extern unsigned char IMG_FONT_28_NUM_7[904];
extern unsigned char IMG_FONT_28_NUM_8[904];
extern unsigned char IMG_FONT_28_NUM_9[904];
extern unsigned char IMG_FONT_28_COLON[456];
extern unsigned char IMG_FONT_28_DOT[456];
extern unsigned char IMG_FONT_28_PERC[1464];
extern unsigned char IMG_FONT_28_SLASH[690];
//font_38_num
extern unsigned char IMG_FONT_38_NUM_0[1680];
extern unsigned char IMG_FONT_38_NUM_1[1680];
extern unsigned char IMG_FONT_38_NUM_2[1680];
extern unsigned char IMG_FONT_38_NUM_3[1680];
extern unsigned char IMG_FONT_38_NUM_4[1680];
extern unsigned char IMG_FONT_38_NUM_5[1680];
extern unsigned char IMG_FONT_38_NUM_6[1680];
extern unsigned char IMG_FONT_38_NUM_7[1680];
extern unsigned char IMG_FONT_38_NUM_8[1680];
extern unsigned char IMG_FONT_38_NUM_9[1680];
extern unsigned char IMG_FONT_38_COLON[844];
extern unsigned char IMG_FONT_38_DOT[844];
extern unsigned char IMG_FONT_38_PERC[2668];
extern unsigned char IMG_FONT_38_SLASH[1208];
//font_42_num
extern unsigned char IMG_FONT_42_NUM_0[2024];
extern unsigned char IMG_FONT_42_NUM_1[2024];
extern unsigned char IMG_FONT_42_NUM_2[2024];
extern unsigned char IMG_FONT_42_NUM_3[2024];
extern unsigned char IMG_FONT_42_NUM_4[2024];
extern unsigned char IMG_FONT_42_NUM_5[2024];
extern unsigned char IMG_FONT_42_NUM_6[2024];
extern unsigned char IMG_FONT_42_NUM_7[2024];
extern unsigned char IMG_FONT_42_NUM_8[2024];
extern unsigned char IMG_FONT_42_NUM_9[2024];
extern unsigned char IMG_FONT_42_COLON[1016];
extern unsigned char IMG_FONT_42_DOT[1016];
extern unsigned char IMG_FONT_42_PERC[3284];
extern unsigned char IMG_FONT_42_SLASH[1538];
//font_60_num
extern unsigned char IMG_FONT_60_NUM_0[4088];
extern unsigned char IMG_FONT_60_NUM_1[4088];
extern unsigned char IMG_FONT_60_NUM_2[4088];
extern unsigned char IMG_FONT_60_NUM_3[4088];
extern unsigned char IMG_FONT_60_NUM_4[4088];
extern unsigned char IMG_FONT_60_NUM_5[4088];
extern unsigned char IMG_FONT_60_NUM_6[4088];
extern unsigned char IMG_FONT_60_NUM_7[4088];
extern unsigned char IMG_FONT_60_NUM_8[4088];
extern unsigned char IMG_FONT_60_NUM_9[4088];
extern unsigned char IMG_FONT_60_COLON[2048];
extern unsigned char IMG_FONT_60_DOT[2048];
extern unsigned char IMG_FONT_60_PERC[6728];
extern unsigned char IMG_FONT_60_SLASH[3080];
//fall
extern unsigned char IMG_FALL_ICON[20808];
extern unsigned char IMG_FALL_YES[7208];
extern unsigned char IMG_FALL_NO[7208];
extern unsigned char IMG_FALL_CONFIRM[7208];
extern unsigned char IMG_FALL_CANCEL[7208];
#endif
#endif/*__FONT_H__*/
