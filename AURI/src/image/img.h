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

#if 0
//extern unsigned char peppa_pig_80X160[25608];
//extern unsigned char peppa_pig_160X160[51208];
//extern unsigned char peppa_pig_240X240_1[57608];
//extern unsigned char peppa_pig_240X240_2[57600];
//extern unsigned char peppa_pig_320X320_1[51208];
//extern unsigned char peppa_pig_320X320_2[51200];
//extern unsigned char peppa_pig_320X320_3[51200];
//extern unsigned char peppa_pig_320X320_4[51200];
//extern unsigned char RM_LOGO_240X240_1[57608];
//extern unsigned char RM_LOGO_240X240_2[57600];
#endif

#if defined(LCD_VGM068A4W01_SH1106G)||defined(LCD_VGM096064A6W01_SP5090)
extern unsigned char jjph_gc_96X64[774];
extern unsigned char jjph_gc_96X32[390];

extern unsigned char IMG_BAT_0[38];
extern unsigned char IMG_BAT_1[38];
extern unsigned char IMG_BAT_2[38];
extern unsigned char IMG_BAT_3[38];
extern unsigned char IMG_BAT_4[38];
extern unsigned char IMG_BAT_5[38];

extern unsigned char IMG_SIG_0[22];
extern unsigned char IMG_SIG_1[22];
extern unsigned char IMG_SIG_2[22];
extern unsigned char IMG_SIG_3[22];
extern unsigned char IMG_SIG_4[22];

extern unsigned char IMG_COLON[34];
extern unsigned char IMG_NO_COLON[34];
extern unsigned char IMG_BIG_NUM_0[70];
extern unsigned char IMG_BIG_NUM_1[70];
extern unsigned char IMG_BIG_NUM_2[70];
extern unsigned char IMG_BIG_NUM_3[70];
extern unsigned char IMG_BIG_NUM_4[70];
extern unsigned char IMG_BIG_NUM_5[70];
extern unsigned char IMG_BIG_NUM_6[70];
extern unsigned char IMG_BIG_NUM_7[70];
extern unsigned char IMG_BIG_NUM_8[70];
extern unsigned char IMG_BIG_NUM_9[70];

extern unsigned char IMG_NUM_0[46];
extern unsigned char IMG_NUM_1[46];
extern unsigned char IMG_NUM_2[46];
extern unsigned char IMG_NUM_3[46];
extern unsigned char IMG_NUM_4[46];
extern unsigned char IMG_NUM_5[46];
extern unsigned char IMG_NUM_6[46];
extern unsigned char IMG_NUM_7[46];
extern unsigned char IMG_NUM_8[46];
extern unsigned char IMG_NUM_9[46];

extern unsigned char IMG_FALL_ICON[114];
extern unsigned char IMG_FALL_CN[194];
extern unsigned char IMG_FALL_EN[114];

extern unsigned char IMG_SLP_ICON[36];
extern unsigned char IMG_STEP_ICON[66];

extern unsigned char IMG_HOUR_CN[38];
extern unsigned char IMG_MIN_CN[38];
extern unsigned char IMG_HOUR_EN[30];
extern unsigned char IMG_MIN_EN[34];

extern unsigned char IMG_SOS[314];
extern unsigned char IMG_SOS_RECE[342];
extern unsigned char IMG_SOS_SEND[342];

extern unsigned char IMG_WRIST_ICON[90];
extern unsigned char IMG_WRIST_CN[194];
extern unsigned char IMG_WRIST_EN[226];

#endif/*LCD_VGM068A4W01_SH1106G*/

#endif/*__FONT_H__*/
