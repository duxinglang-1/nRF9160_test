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
//analog clock
extern unsigned char IMG_ANALOG_CLOCK_BG[115208];
extern unsigned char IMG_ANALOG_CLOCK_DOT_WHITE[458];
extern unsigned char IMG_ANALOG_CLOCK_DOT_RED[250];
extern unsigned char IMG_ANALOG_CLOCK_HAND_SEC[332];
extern unsigned char IMG_ANALOG_CLOCK_HAND_MIN[1268];
extern unsigned char IMG_ANALOG_CLOCK_HAND_HOUR[962];
//battery
extern unsigned char IMG_BAT_RECT_RED[968];
extern unsigned char IMG_BAT_RECT_WHITE[968];
//blood pressure
extern unsigned char IMG_BP_BG[35368];
extern unsigned char IMG_BP_UP_ARRAW[586];
extern unsigned char IMG_BP_DOWN_ARRAW[392];
extern unsigned char IMG_BP_DOWN_LINE[64];
extern unsigned char IMG_BP_MEASUREING[6520];
extern unsigned char IMG_BP_MEASURENING_ANI_1[42596];
extern unsigned char IMG_BP_MEASURENING_ANI_2[42596];
extern unsigned char IMG_BP_MEASURENING_ANI_3[42596];
extern unsigned char IMG_BP_MMHG[1044];
extern unsigned char IMG_BP_TIME_DOT[28];
//heart rate
extern unsigned char IMG_HR_BG[35440];
extern unsigned char IMG_HR_BPM_BIG[1988];
extern unsigned char IMG_HR_BPM_MID[1232];
extern unsigned char IMG_HR_BPM_SMALL[764];
extern unsigned char IMG_HR_DOWN_ARRAW[548];
extern unsigned char IMG_HR_DOWN_LINE[80];
extern unsigned char IMG_HR_ICON[2116];
extern unsigned char IMG_HR_MEASURING[2816];
extern unsigned char IMG_HR_STATIC[2340];
extern unsigned char IMG_HR_TIME_DOT[28];
extern unsigned char IMG_HR_UP_ARRAW[484];
//idle step
extern unsigned char IMG_IDLE_STEP_LOGO[20072];
//ota
extern unsigned char IMG_OTA_DOWNLOADING[7460];
extern unsigned char IMG_OTA_FAILED_ICON[21640];
extern unsigned char IMG_OTA_FINISH[115208];
extern unsigned char IMG_OTA_LOGO[7208];
extern unsigned char IMG_OTA_NO[3208];
extern unsigned char IMG_OTA_RUNNING_FAIL[3572];
extern unsigned char IMG_OTA_RUNNING_STR[5516];
extern unsigned char IMG_OTA_STR[19748];
extern unsigned char IMG_OTA_YES[3208];
//power off
extern unsigned char IMG_PWROFF_ANI_1[29776];
extern unsigned char IMG_PWROFF_ANI_2[29776];
extern unsigned char IMG_PWROFF_ANI_3[29776];
extern unsigned char IMG_PWROFF_ANI_4[29776];
extern unsigned char IMG_PWROFF_ANI_5[29776];
extern unsigned char IMG_PWROFF_ANI_6[29776];
extern unsigned char IMG_PWROFF_ANI_7[29776];
extern unsigned char IMG_PWROFF_ANI_8[29776];
extern unsigned char IMG_PWROFF_BG[115208];
extern unsigned char IMG_PWROFF_BUTTON[12808];
extern unsigned char IMG_PWROFF_LOGO[1508];
extern unsigned char IMG_PWROFF_NO[3208];
extern unsigned char IMG_PWROFF_RUNNING_STR[8626];
extern unsigned char IMG_PWROFF_STR[5732];
extern unsigned char IMG_PWROFF_YES[3208];
//power on
extern unsigned char IMG_PWRON_ANI_1[13388];
extern unsigned char IMG_PWRON_ANI_2[13388];
extern unsigned char IMG_PWRON_ANI_3[13388];
extern unsigned char IMG_PWRON_ANI_4[13388];
extern unsigned char IMG_PWRON_ANI_5[13388];
extern unsigned char IMG_PWRON_ANI_6[13388];
extern unsigned char IMG_PWRON_BG[115208];
extern unsigned char IMG_PWRON_STR[9908];
//3 dot gif
extern unsigned char IMG_RUNNING_ANI_1[2114];
extern unsigned char IMG_RUNNING_ANI_2[2114];
extern unsigned char IMG_RUNNING_ANI_3[2114];
extern unsigned char IMG_RUNNING_ANI_4[2114];
//signal
extern unsigned char IMG_SIG_0[1744];
extern unsigned char IMG_SIG_1[1744];
extern unsigned char IMG_SIG_2[1744];
extern unsigned char IMG_SIG_3[1744];
extern unsigned char IMG_SIG_4[1744];
//sim information
extern unsigned char IMG_SIM_ICCID[2524];
extern unsigned char IMG_SIM_IMSI[2388];
//sleep
extern unsigned char IMG_SLEEP_ANI_1[4608];
extern unsigned char IMG_SLEEP_ANI_2[4608];
extern unsigned char IMG_SLEEP_ANI_3[4608];
extern unsigned char IMG_SLEEP_DEEP_ICON[2600];
extern unsigned char IMG_SLEEP_HOUR[844];
extern unsigned char IMG_SLEEP_LIGHT_ICON[2600];
extern unsigned char IMG_SLEEP_LINE[712];
extern unsigned char IMG_SLEEP_MIN[1460];
//step
extern unsigned char IMG_STEP_ANI_1[4036];
extern unsigned char IMG_STEP_ANI_2[4036];
extern unsigned char IMG_STEP_CAL_ICON[2024];
extern unsigned char IMG_STEP_DIS_ICON[2096];
extern unsigned char IMG_STEP_ICON[2208];
extern unsigned char IMG_STEP_KCAL[708];
extern unsigned char IMG_STEP_KM[484];
extern unsigned char IMG_STEP_LINE[668];
//sync
extern unsigned char IMG_SYNC_ERR[24868];
extern unsigned char IMG_SYNC_FINISH[24868];
extern unsigned char IMG_SYNC_LOGO[20808];
extern unsigned char IMG_SYNC_STR[8288];
//wrist off
extern unsigned char IMG_WRIST_OFF_ICON[23372];
#endif

#endif/*__FONT_H__*/
