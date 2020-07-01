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

extern unsigned int peppa_pig_160X160[];	//小猪佩奇图片
extern unsigned int peppa_pig_80X160[];	//小猪佩奇图片
extern unsigned int clock_hour_1_31X31[];
extern unsigned int clock_hour_2_31X31[];
extern unsigned int clock_hour_3_31X31[];
extern unsigned int clock_hour_4_31X31[];
extern unsigned int clock_min_1_31X31[];
extern unsigned int clock_min_2_31X31[];
extern unsigned int clock_min_3_31X31[];
extern unsigned int clock_min_4_31X31[];
extern unsigned int clock_min_5_31X31[];
extern unsigned int clock_min_6_31X31[];
extern unsigned int clock_min_7_31X31[];
extern unsigned int clock_min_8_31X31[];
extern unsigned int clock_min_9_31X31[];
extern unsigned int clock_min_10_31X31[];
extern unsigned int clock_min_11_31X31[];
extern unsigned int clock_min_12_31X31[];
extern unsigned int clock_min_13_31X31[];
extern unsigned int clock_min_14_31X31[];
extern unsigned int clock_min_15_31X31[];
extern unsigned int clock_min_16_31X31[];
extern unsigned int clock_bg_80X160[];

#endif/*__FONT_H__*/
