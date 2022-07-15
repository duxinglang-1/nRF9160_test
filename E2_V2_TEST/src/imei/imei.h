#ifndef __IMEI_H__
#define __IMEI_H__

/****************************************************************************************************
Code128码于1981年推出，是一种长度可变、连续性的字母数字条码。与其他一维条码比较起来，相对较为复杂，支持的字元也相对较多，又有不同的编码方式可供交互运用，因此其应用弹性也较大。
Code128特性：
1、具有A、B、C三种不同的编码类型，可提供标准ASCII中128个字元的编码使用；
2、允许双向扫描；
3、可自行决定是否加上检验位；
4、条码长度可调，但包括开始位和结束位在内，不可超过232个字元；
5、同一个128码，可以由A、B、C三种不同编码规则互换，既可扩大字元选择的范围，也可缩短编码的长度。

Code128各编码方式的编码范围：
1、Code128A：标准数字和字母，控制符，特殊字符；
2、Code128B：标准数字和字母，小写字母，特殊字符；
3、Code128C/EAN128：[00]-[99]的数字对集合，共100个，即只能表示偶数位长度的数字。

Code128编码规则：开始位 ＋ ［FNC1(为EAN128码时加)］ ＋ 数据位 ＋ 检验位 ＋ 结束位

Code128检验位计算：（开始位对应的ID值 ＋ 每位数据在整个数据中的位置×每位数据对应的ID值）% 103

Code128编码表：
	ID		Code128A	Code128B	Code128C	BandCode	编码值
	0		SP　		SP			0			212222		bbsbbssbbss
	1		!			!			1			222122		bbssbbsbbss
	2		"			"			2			222221		bbssbbssbbs
	3		#			#			3			121223		bssbssbbsss
	4		$			$			4			121322		bssbsssbbss
	5		%			%			5			131222		bsssbssbbss
	6		&			&			6			122213		bssbbssbsss
	7		'			'			7			122312		bssbbsssbss
	8		(			(			8			132212		bsssbbssbss
	9		)			)			9			221213		bbssbssbsss
	10		*			*			10			221312		bbssbsssbss
	11		+			+			11			231212		bbsssbssbss
	12		,			,			12			112232		bsbbssbbbss
	13		-			-			13			122132		bssbbsbbbss
	14		.			.			14			122231		bssbbssbbbs
	15		/			/			15			113222		bsbbbssbbss
	16		0			0			16			123122		bssbbbsbbss
	17		1			1			17			123221		bssbbbssbbs
	18		2			2			18			223211		bbssbbbssbs
	19		3			3			19			221132		bbssbsbbbss
	20		4			4			20			221231		bbssbssbbbs
	21		5			5			21			213212		bbsbbbssbss
	22		6			6			22			223112		bbssbbbsbss
	23		7			7			23			312131		bbbsbbsbbbs
	24		8			8			24			311222		bbbsbssbbss
	25		9			9			25			321122		bbbssbsbbss
	26		:			:			26			321221		bbbssbssbbs
	27		;			;			27			312212		bbbsbbssbss
	28		<			<			28			322112		bbbssbbsbss
	29		=			=			29			322211		bbbssbbssbs
	30		>			>			30			212123		bbsbbsbbsss
	31		?			?			31			212321		bbsbbsssbbs
	32		@			@			32			232121		bbsssbbsbbs
	33		A			A			33			111323		bsbsssbbsss
	34		B			B			34			131123		bsssbsbbsss
	35		C			C			35			131321		bsssbsssbbs
	36		D			D			36			112313		bsbbsssbsss
	37		E			E			37			132113		bsssbbsbsss
	38		F			F			38			132311		bsssbbsssbs
	39		G			G			39			211313		bbsbsssbsss
	40		H			H			40			231113		bbsssbsbsss
	41		I			I			41			231311		bbsssbsssbs
	42		J			J			42			112133		bsbbsbbbsss
	43		K			K			43			112331		bsbbsssbbbs
	44		L			L			44			132131		bsssbbsbbbs
	45		M			M			45			113123		bsbbbsbbsss
	46		N			N			46			113321		bsbbbsssbbs
	47		O			O			47			133121		bsssbbbsbbs
	48		P			P			48			313121		bbbsbbbsbbs
	49		Q			Q			49			211331		bbsbsssbbbs
	50		R			R			50			231131		bbsssbsbbbs
	51		S			S			51			213113		bbsbbbsbsss
	52		T			T			52			213311		bbsbbbsssbs
	53		U			U			53			213131		bbsbbbsbbbs
	54		V			V			54			311123		bbbsbsbbsss
	55		W			W			55			311321		bbbsbsssbbs
	56		X			X			56			331121		bbbsssbsbbs
	57		Y			Y			57			312113		bbbsbbsbsss
	58		Z			Z			58			312311		bbbsbbsssbs
	59		[			[			59			332111		bbbsssbbsbs
	60		\			\			60			314111		bbbsbbbbsbs
	61		]			]			61			221411		bbssbssssbs
	62		^			^			62			431111		bbbbsssbsbs
	63		_			_			63			111224		bsbssbbssss
	64		NUL			`			64			111422		bsbssssbbss
	65		SOH			a			65			121124		bssbsbbssss
	66		STX			b			66			121421		bssbssssbbs
	67		ETX			c			67			141122		bssssbsbbss
	68		EOT			d			68			141221		bssssbssbbs
	69		ENQ			e			69			112214		bsbbssbssss
	70		ACK			f			70			112412		bsbbssssbss
	71		BEL			g			71			122114		bssbbsbssss
	72		BS			h			72			122411		bssbbssssbs
	73		HT			i			73			142112		bssssbbsbss
	74		LF			j			74			142211		bssssbbssbs
	75		VT			k			75			241211		bbssssbssbs
	76		FF			I			76			221114		bbssbsbssss
	77		CR			m			77			413111		bbbbsbbbsbs
	78		SO			n			78			241112		bbssssbsbss
	79		SI			o			79			134111		bsssbbbbsbs
	80		DLE			p			80			111242		bsbssbbbbss
	81		DC1			q			81			121142		bssbsbbbbss
	82		DC2			r			82			121241		bssbssbbbbs
	83		DC3			s			83			114212		bsbbbbssbss
	84		DC4			t			84			124112		bssbbbbsbss
	85		NAK			u			85			124211		bssbbbbssbs
	86		SYN			v			86			411212		bbbbsbssbss
	87		ETB			w			87			421112		bbbbssbsbss
	88		CAN			x			88			421211		bbbbssbssbs
	89		EM			y			89			212141		bbsbbsbbbbs
	90		SUB			z			90			214121		bbsbbbbsbbs
	91		ESC			{			91			412121		bbbbsbbsbbs
	92		FS			|			92			111143		bsbsbbbbsss
	93		GS			}			93			111341		bsbsssbbbbs
	94		RS			~			94			131141		bsssbsbbbbs
	95		US			DEL			95			114113		bsbbbbsbsss
	96		FNC3		FNC3		96			114311		bsbbbbsssbs
	97		FNC2		FNC2		97			411113		bbbbsbsbsss
	98		SHIFT		SHIFT		98			411311		bbbbsbsssbs
	99		CODEC		CODEC		99			113141		bsbbbsbbbbs
	100		CODEB		FNC4		CODEB		114131		bsbbbbsbbbs
	101		FNC4		CODEA		CODEA		311141		bbbsbsbbbbs
	102		FNC1		FNC1		FNC1		411131		bbbbsbsbbbs
	103		StartA		StartA		StartA		211412		bbsbssssbss
	104		StartB		StartB		StartB		211214		bbsbssbssss
	105		StartC		StartC		StartC		211232		bbsbssbbbss
	106		Stop		Stop		Stop		2331112		bbsssbbbsbsbb


Code128编码示例：以 95270078 为例
Code128A， 开始位对应的ID为103，第1位数据9对应的ID为25，第2位数据5对应的ID为21，
依此类推，可以计算检验位 = (103 + 1*25 + 2*21 + 3*18 + 4*23 + 5*16 + 6*16 + 7*23 + 8*24) % 103 = 21，即检验位的ID为21。

对照编码表，95270078 编码表示为：
开始位StartA(bbsbssssbss)+数据位[9(bbbssbsbbss)+5(bbsbbbssbss)+2(bbssbbbssbs)+7(bbbsbbsbbbs)+0(bssbbbsbbss)+0(bssbbbsbbss)+7(bbbsbbsbbbs)+8(bbbsbssbbss)]+检验位21(bbsbbbssbss)+结束位Stop(bbsssbbbsbsbb),
即：bbsbssssbssbbbssbsbbssbbsbbbssbssbbssbbbssbsbbbsbbsbbbsbssbbbsbbssbssbbbsbbssbbbsbbsbbbsbbbsbssbbssbbsbbbssbssbbsssbbbsbsbb。 
若要打印，只需将b用黑色线标出，s用白色线标出，一个简单的条形码生成程序就完成了！

128B与128A类似，128C只能对长度为偶数的数字串编码，每两个数字为一位，所以输出的信息压缩了一半，打印的条形码因此也就较短。接上例，第1位数据 95对应ID为95，第2位数据27对应ID为27，第3位数据00对应ID为0，第4位数据78对应ID为78，所以检验位 = (105 + 1*95 + 2*27 + 3*0 + 4*78) % 103 = 51

EAN128与Code128C相同，只是在开始位后加多一个控制位FNC1（ID为102），同时将FNC1做为第1位数据加入到检验位的计算。

各种方式的编码结果罗列如下：


编码方式	开始位	FNC1	数据位							检验位																		结束位	编码结果
Code128A	StartA	无		9 + 5 + 2 + 7 + 0 + 0 + 7 + 8	(103 + 1*25 + 2*21 + 3*18 + 4*23 + 5*16 + 6*16 + 7*23 + 8*24) % 103 = 21	Stop	bbsbssssbssbbbssbsbbssbbsbbbssbssbbssbbbssbsbbbsbbsbbbsbssbbbsbbssbssbbbsbbssbbbsbbsbbbsbbbsbssbbssbbsbbbssbssbbsssbbbsbsbb
Code128B	StartB	无		9 + 5 + 2 + 7 + 0 + 0 + 7 + 8	(104 + 1*25 + 2*21 + 3*18 + 4*23 + 5*16 + 6*16 + 7*23 + 8*24) % 103 = 22	Stop	bbsbssbssssbbbssbsbbssbbsbbbssbssbbssbbbssbsbbbsbbsbbbsbssbbbsbbssbssbbbsbbssbbbsbbsbbbsbbbsbssbbssbbssbbbsbssbbsssbbbsbsbb
Code128C	StartC	无		95 + 27 + 00 + 78				(105 + 1*95 + 2*27 + 3*0 + 4*78) % 103 = 51									Stop	bbsbssbbbssbsbbbbsbsssbbbsbbssbssbbsbbssbbssbbssssbsbssbbsbbbsbsssbbsssbbbsbsbb
EAN128		StartC	FNC1	95 + 27 + 00 + 78				(105 + 1*102 + 2*95 + 3*27 + 4*0 + 5*78) % 103 = 44							Stop	bbsbssbbbssbbbbsbsbbbsbsbbbbsbsssbbbsbbssbssbbsbbssbbssbbssssbsbssbsssbbsbbbsbbsssbbbsbsbb
****************************************************************************************************/
#define IMEI_LEN	15

typedef struct{
	u8_t byte;
	u8_t str[15];
}bar_code128_t;

#endif/*__IMEI_H__*/