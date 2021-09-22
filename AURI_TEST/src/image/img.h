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

#if defined(LCD_VGM068A4W01_SH1106G)||defined(LCD_VGM096064A6W01_SP5090)
//LOGO
extern unsigned char logo_1_96X64[774];
extern unsigned char logo_2_96X64[774];
extern unsigned char logo_3_96X64[774];
extern unsigned char logo_4_96X64[774];
extern unsigned char logo_5_96X64[774];
//BATTERY
extern unsigned char IMG_BAT_0[38];
extern unsigned char IMG_BAT_1[38];
extern unsigned char IMG_BAT_2[38];
extern unsigned char IMG_BAT_3[38];
extern unsigned char IMG_BAT_4[38];
extern unsigned char IMG_BAT_5[38];
//SIGNAL
extern unsigned char IMG_SIG_0[22];
extern unsigned char IMG_SIG_1[22];
extern unsigned char IMG_SIG_2[22];
extern unsigned char IMG_SIG_3[22];
extern unsigned char IMG_SIG_4[22];
//AM&PM
extern unsigned char IMG_AM_CN[40];
extern unsigned char IMG_AM_EN[34];
extern unsigned char IMG_PM_CN[40];
extern unsigned char IMG_PM_EN[34];
//DATE
extern unsigned char IMG_DATE_LINK[24];
extern unsigned char IMG_DATE_NUM_0[36];
extern unsigned char IMG_DATE_NUM_1[36];
extern unsigned char IMG_DATE_NUM_2[36];
extern unsigned char IMG_DATE_NUM_3[36];
extern unsigned char IMG_DATE_NUM_4[36];
extern unsigned char IMG_DATE_NUM_5[36];
extern unsigned char IMG_DATE_NUM_6[36];
extern unsigned char IMG_DATE_NUM_7[36];
extern unsigned char IMG_DATE_NUM_8[36];
extern unsigned char IMG_DATE_NUM_9[36];
//TIME
extern unsigned char IMG_TIME_SPE_NO[42];
extern unsigned char IMG_TIME_SPE[42];
extern unsigned char IMG_TIME_NUM_0[57];
extern unsigned char IMG_TIME_NUM_1[57];
extern unsigned char IMG_TIME_NUM_2[57];
extern unsigned char IMG_TIME_NUM_3[57];
extern unsigned char IMG_TIME_NUM_4[57];
extern unsigned char IMG_TIME_NUM_5[57];
extern unsigned char IMG_TIME_NUM_6[57];
extern unsigned char IMG_TIME_NUM_7[57];
extern unsigned char IMG_TIME_NUM_8[57];
extern unsigned char IMG_TIME_NUM_9[57];
//MONTH
extern unsigned char IMG_MON_JAN[78];
extern unsigned char IMG_MON_FEB[78];
extern unsigned char IMG_MON_MAR[78];
extern unsigned char IMG_MON_APR[78];
extern unsigned char IMG_MON_MAY[78];
extern unsigned char IMG_MON_JUN[78];
extern unsigned char IMG_MON_JUL[78];
extern unsigned char IMG_MON_AUG[78];
extern unsigned char IMG_MON_SEP[78];
extern unsigned char IMG_MON_OCT[78];
extern unsigned char IMG_MON_NOV[78];
extern unsigned char IMG_MON_DEC[78];
//WEEK
extern unsigned char IMG_WEEK_1[93];
extern unsigned char IMG_WEEK_2[93];
extern unsigned char IMG_WEEK_3[93];
extern unsigned char IMG_WEEK_4[93];
extern unsigned char IMG_WEEK_5[93];
extern unsigned char IMG_WEEK_6[93];
extern unsigned char IMG_WEEK_7[93];
extern unsigned char IMG_WEEK_MON[84];
extern unsigned char IMG_WEEK_TUE[84];
extern unsigned char IMG_WEEK_WED[84];
extern unsigned char IMG_WEEK_THU[84];
extern unsigned char IMG_WEEK_FRI[84];
extern unsigned char IMG_WEEK_SAT[84];
extern unsigned char IMG_WEEK_SUN[84];
//BLE
extern unsigned char IMG_BLE_LINK[20];
extern unsigned char IMG_BLE_UNLINK[20];
//STEP
extern unsigned char IMG_STEP_ICON_1[390];
extern unsigned char IMG_STEP_ICON_2[390];
extern unsigned char IMG_STEP_CN[50];
extern unsigned char IMG_STEP_EN[90];
extern unsigned char IMG_STEP_NOTE_ICON_1[390];
extern unsigned char IMG_STEP_NOTE_ICON_2[390];
extern unsigned char IMG_STEP_NOTE_CN[66];
extern unsigned char IMG_STEP_NOTE_EN[122];
//DISTANCE
extern unsigned char IMG_DISTANCE_ICON_1[390];
extern unsigned char IMG_DISTANCE_ICON_2[390];
extern unsigned char IMG_KM_CN[102];
extern unsigned char IMG_KM_EN[102];
extern unsigned char IMG_MILE_CN[102];
extern unsigned char IMG_MILE_EN[102];
//CALORIE
extern unsigned char IMG_CAL_ICON_1[390];
extern unsigned char IMG_CAL_ICON_2[390];
extern unsigned char IMG_CAL_CN[106];
extern unsigned char IMG_CAL_EN[94];
//SLEEP
extern unsigned char IMG_SLEEP_ICON_1[390];
extern unsigned char IMG_SLEEP_ICON_2[390];
extern unsigned char IMG_SLEEP_ICON_3[390];
extern unsigned char IMG_SLEEP_ICON_4[390];
extern unsigned char IMG_SLEEP_HR_CN[50];
extern unsigned char IMG_SLEEP_HR_EN[54];
extern unsigned char IMG_SLEEP_MIN_CN[50];
extern unsigned char IMG_SLEEP_MIN_EN[66];
//FALL
extern unsigned char IMG_FALL_ICON_CN_1[774];
extern unsigned char IMG_FALL_ICON_CN_2[774];
extern unsigned char IMG_FALL_ICON_CN_3[774];
extern unsigned char IMG_FALL_ICON_EN_1[774];
extern unsigned char IMG_FALL_ICON_EN_2[774];
extern unsigned char IMG_FALL_ICON_EN_3[774];
extern unsigned char IMG_FALL_MSG_SEND_CN[774];
extern unsigned char IMG_FALL_MSG_CANCEL_CN[774];
extern unsigned char IMG_FALL_MSG_SEND_EN[774];
extern unsigned char IMG_FALL_MSG_CANCEL_EN[774];
//FIND
extern unsigned char IMG_FIND_ICON_1[390];
extern unsigned char IMG_FIND_ICON_2[390];
extern unsigned char IMG_FIND_ICON_3[390];
extern unsigned char IMG_FIND_CN[390];
extern unsigned char IMG_FIND_EN[390];
#if 0
//CHARGING
extern unsigned char IMG_CHARGING_OK[774];
extern unsigned char IMG_CHARGE_PLS_CN[774];
extern unsigned char IMG_CHARGE_PLS_EN[774];
extern unsigned char IMG_CHARGING_CN_0[774];
extern unsigned char IMG_CHARGING_CN_1[774];
extern unsigned char IMG_CHARGING_CN_2[774];
extern unsigned char IMG_CHARGING_CN_3[774];
extern unsigned char IMG_CHARGING_CN_4[774];
extern unsigned char IMG_CHARGING_CN_5[774];
extern unsigned char IMG_CHARGING_EN_0[774];
extern unsigned char IMG_CHARGING_EN_1[774];
extern unsigned char IMG_CHARGING_EN_2[774];
extern unsigned char IMG_CHARGING_EN_3[774];
extern unsigned char IMG_CHARGING_EN_4[774];
extern unsigned char IMG_CHARGING_EN_5[774];
#endif
//SOS
extern unsigned char IMG_SOS_ICON[390];
extern unsigned char IMG_SOS_SENDING_1[390];
extern unsigned char IMG_SOS_SENDING_2[390];
extern unsigned char IMG_SOS_SENDING_3[390];
extern unsigned char IMG_SOS_SENDING_OK[390];
//WRIST OFF
extern unsigned char IMG_WRIST_OFF_CN[774];
extern unsigned char IMG_WRIST_OFF_EN[774];
//ALARM
extern unsigned char IMG_ALARM_ICON_1[390];
extern unsigned char IMG_ALARM_ICON_2[390];
extern unsigned char IMG_ALARM_ICON_3[390];
//NUM
extern unsigned char IMG_MID_NUM_0[62];
extern unsigned char IMG_MID_NUM_1[62];
extern unsigned char IMG_MID_NUM_2[62];
extern unsigned char IMG_MID_NUM_3[62];
extern unsigned char IMG_MID_NUM_4[62];
extern unsigned char IMG_MID_NUM_5[62];
extern unsigned char IMG_MID_NUM_6[62];
extern unsigned char IMG_MID_NUM_7[62];
extern unsigned char IMG_MID_NUM_8[62];
extern unsigned char IMG_MID_NUM_9[58];
//DOT
extern unsigned char IMG_DOT[21];
//POWER
extern unsigned char IMG_PWROFF_KEY_CN[774];
extern unsigned char IMG_PWROFF_KEY_EN[774];
extern unsigned char IMG_PWROFF_CN[774];
extern unsigned char IMG_PWROFF_EN[774];

#endif/*LCD_VGM068A4W01_SH1106G*/

#endif/*__IMG_H__*/
