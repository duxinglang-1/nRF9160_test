#ifndef __IMG_H__
#define __IMG_H__

/*Image2Lcd保存的图像数据组织方式为：图像头数据-调色板数据-图像数据。
“单色/4灰/16灰/256色”的图像数据头如下：
typedef struct _HEADGRAY
{
   unsigned char scan;
   unsigned char gray;
   unsigned short w;
   unsigned short h;
}HEADGRAY;

scan: 扫描模式
Bit7: 0:自左至右扫描，1:自右至左扫描。 
Bit6: 0:自顶至底扫描，1:自底至顶扫描。 
Bit5: 0:字节内象素数据从高位到低位排列，1:字节内象素数据从低位到高位排列。 
Bit4: 0:WORD类型高低位字节顺序与PC相同，1:WORD类型高低位字节顺序与PC相反。 
Bit3~2: 保留。 
Bit1~0: [00]水平扫描，[01]垂直扫描，[10]数据水平,字节垂直，[11]数据垂直,字节水平。 

gray: 灰度值 
   灰度值，1:单色，2:四灰，4:十六灰，8:256色，12:4096色，16:16位彩色，24:24位彩色，32:32位彩色。

w: 图像的宽度。

h: 图像的高度。

“4096色/16位真彩色/18位真彩色/24位真彩色/32位真彩色”的图像数据头如下：
typedef struct _HEADCOLOR
{
   unsigned char scan;
   unsigned char gray;
   unsigned short w;
   unsigned short h;
   unsigned char is565;
   unsigned char rgb;
}HEADCOLOR; 

scan、gray、w、h与HEADGRAY结构中的同名成员变量含义相同。

is565: 在4096色模式下为0表示使用[16bits(WORD)]格式，此时图像数据中每个WORD表示一个象素；为1表示使用[12bits(连续字节流)]格式，此时连续排列的每12Bits代表一个象素。
在16位彩色模式下为0表示R G B颜色分量所占用的位数都为5Bits，为1表示R G B颜色分量所占用的位数分别为5Bits,6Bits,5Bits。
在18位彩色模式下为0表示"6Bits in Low Byte"，为1表示"6Bits in High Byte"。
在24位彩色和32位彩色模式下is565无效。

rgb: 描述R G B颜色分量的排列顺序，rgb中每2Bits表示一种颜色分量，[00]表示空白，[01]表示Red，[10]表示Green，[11]表示Blue。

“256色”的调色板数据结构如下：
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

仅在256色模式下存在调色板数据结构,调色板数据结构紧跟在数据结构HEADGRAY之后。
*/

#if 1
//analog clock(6)
extern unsigned char IMG_ANALOG_CLOCK_BG[115208];
extern unsigned char IMG_ANALOG_CLOCK_DOT_WHITE[458];
extern unsigned char IMG_ANALOG_CLOCK_DOT_RED[250];
extern unsigned char IMG_ANALOG_CLOCK_HAND_SEC[1652];
extern unsigned char IMG_ANALOG_CLOCK_HAND_MIN[2864];
extern unsigned char IMG_ANALOG_CLOCK_HAND_HOUR[2080];
//battery(9)
extern unsigned char IMG_BAT_CHRING_ANI_1[23708];
extern unsigned char IMG_BAT_CHRING_ANI_2[23708];
extern unsigned char IMG_BAT_CHRING_ANI_3[23708];
extern unsigned char IMG_BAT_CHRING_ANI_4[23708];
extern unsigned char IMG_BAT_CHRING_ANI_5[23708];
extern unsigned char IMG_BAT_FULL_ICON[115208];
extern unsigned char IMG_BAT_LOW_ICON[23708];
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
extern unsigned char IMG_BP_BG[35708];
extern unsigned char IMG_BP_DOWN_ARRAW[392];
extern unsigned char IMG_BP_ICON_ANI_1[1768];
extern unsigned char IMG_BP_ICON_ANI_2[1768];
extern unsigned char IMG_BP_ICON_ANI_3[1768];
extern unsigned char IMG_BP_SPE_LINE[504];
extern unsigned char IMG_BP_UP_ARRAW[586];
//function option(10)
extern unsigned char IMG_FUN_OPT_BP_ICON[4240];
extern unsigned char IMG_FUN_OPT_HR_ICON[4240];
extern unsigned char IMG_FUN_OPT_PGDN_ICON[272];
extern unsigned char IMG_FUN_OPT_PGUP_ICON[272];
extern unsigned char IMG_FUN_OPT_SETTING_ICON[4240];
extern unsigned char IMG_FUN_OPT_SLEEP_ICON[4240];
extern unsigned char IMG_FUN_OPT_SPO2_ICON[4240];
extern unsigned char IMG_FUN_OPT_STEP_ICON[4240];
extern unsigned char IMG_FUN_OPT_SYNC_ICON[4240];
extern unsigned char IMG_FUN_OPT_TEMP_ICON[4240];
//heart rate(6)
extern unsigned char IMG_HR_BG[34008];
extern unsigned char IMG_HR_BPM[1988];
extern unsigned char IMG_HR_DOWN_ARRAW[548];
extern unsigned char IMG_HR_ICON_ANI_1[2116];
extern unsigned char IMG_HR_ICON_ANI_2[2116];
extern unsigned char IMG_HR_UP_ARRAW[484];
//idle hr(2)
extern unsigned char IMG_IDLE_HR_BG[7208];
extern unsigned char IMG_IDLE_HR_ICON[1304];
//idle net mode(2)
extern unsigned char IMG_IDLE_NET_LTEM[344];
extern unsigned char IMG_IDLE_NET_NB[316];
//idle temp(3)
extern unsigned char IMG_IDLE_TEMP_BG[7208];
extern unsigned char IMG_IDLE_TEMP_C_ICON[968];
extern unsigned char IMG_IDLE_TEMP_F_ICON[968];
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
//3 dot gif(4)
extern unsigned char IMG_RUNNING_ANI_1[2114];
extern unsigned char IMG_RUNNING_ANI_2[2114];
extern unsigned char IMG_RUNNING_ANI_3[2114];
//select icon(2)
extern unsigned char IMG_SELECT_ICON_NO[808];
extern unsigned char IMG_SELECT_ICON_YES[808];
//settings(8)
extern unsigned char IMG_SET_BG[12852];
extern unsigned char IMG_SET_INFO_BG[17008];
extern unsigned char IMG_SET_PG_1[272];
extern unsigned char IMG_SET_PG_2[272];
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
//sleep(6)
extern unsigned char IMG_SLEEP_ANI_3[4608];
extern unsigned char IMG_SLEEP_DEEP_ICON[2600];
extern unsigned char IMG_SLEEP_HOUR[844];
extern unsigned char IMG_SLEEP_LIGHT_ICON[2600];
extern unsigned char IMG_SLEEP_LINE[712];
extern unsigned char IMG_SLEEP_MIN[1460];
//sos(4)
extern unsigned char IMG_SOS_ANI_1[40880];
extern unsigned char IMG_SOS_ANI_2[40880];
extern unsigned char IMG_SOS_ANI_3[40880];
extern unsigned char IMG_SOS_ANI_4[40880];
//spo2(6)
extern unsigned char IMG_SPO2_ANI_1[2328];
extern unsigned char IMG_SPO2_ANI_2[2328];
extern unsigned char IMG_SPO2_ANI_3[2328];
extern unsigned char IMG_SPO2_BG[32648];
extern unsigned char IMG_SPO2_DOWN_ARRAW[484];
extern unsigned char IMG_SPO2_UP_ARRAW[484];
//sport achieve(4)
extern unsigned char IMG_SPORT_ACHIEVE_BG[14888];
extern unsigned char IMG_SPORT_ACHIEVE_ICON[2120];
extern unsigned char IMG_SPORT_ACHIEVE_LOGO[38650];
//step(6)
extern unsigned char IMG_STEP_ANI_1[4036];
extern unsigned char IMG_STEP_CAL_ICON[2024];
extern unsigned char IMG_STEP_DIS_ICON[2096];
extern unsigned char IMG_STEP_KCAL[708];
extern unsigned char IMG_STEP_KM[484];
extern unsigned char IMG_STEP_LINE[668];
//sync(3)
extern unsigned char IMG_SYNC_ERR[42128];
extern unsigned char IMG_SYNC_FINISH[42128];
extern unsigned char IMG_SYNC_LOGO[36458];
//temperature(4)
extern unsigned char IMG_TEMP_ICON_C[16208];
extern unsigned char IMG_TEMP_ICON_F[16532];
extern unsigned char IMG_TEMP_UNIT_C[520];
extern unsigned char IMG_TEMP_UNIT_F[520];
//wrist off(2)
extern unsigned char IMG_WRIST_OFF_ICON[23372];
#endif
#endif/*__FONT_H__*/
