#ifndef __IMEI_H__
#define __IMEI_H__

/****************************************************************************************************
Code128����1981���Ƴ�����һ�ֳ��ȿɱ䡢�����Ե���ĸ�������롣������һά����Ƚ���������Խ�Ϊ���ӣ�֧�ֵ���ԪҲ��Խ϶࣬���в�ͬ�ı��뷽ʽ�ɹ��������ã������Ӧ�õ���Ҳ�ϴ�
Code128���ԣ�
1������A��B��C���ֲ�ͬ�ı������ͣ����ṩ��׼ASCII��128����Ԫ�ı���ʹ�ã�
2������˫��ɨ�裻
3�������о����Ƿ���ϼ���λ��
4�����볤�ȿɵ�����������ʼλ�ͽ���λ���ڣ����ɳ���232����Ԫ��
5��ͬһ��128�룬������A��B��C���ֲ�ͬ������򻥻����ȿ�������Ԫѡ��ķ�Χ��Ҳ�����̱���ĳ��ȡ�

Code128�����뷽ʽ�ı��뷶Χ��
1��Code128A����׼���ֺ���ĸ�����Ʒ��������ַ���
2��Code128B����׼���ֺ���ĸ��Сд��ĸ�������ַ���
3��Code128C/EAN128��[00]-[99]�����ֶԼ��ϣ���100������ֻ�ܱ�ʾż��λ���ȵ����֡�

Code128������򣺿�ʼλ �� ��FNC1(ΪEAN128��ʱ��)�� �� ����λ �� ����λ �� ����λ

Code128����λ���㣺����ʼλ��Ӧ��IDֵ �� ÿλ���������������е�λ�á�ÿλ���ݶ�Ӧ��IDֵ��% 103

Code128�����
	ID		Code128A	Code128B	Code128C	BandCode	����ֵ
	0		SP��		SP			0			212222		bbsbbssbbss
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


Code128����ʾ������ 95270078 Ϊ��
Code128A�� ��ʼλ��Ӧ��IDΪ103����1λ����9��Ӧ��IDΪ25����2λ����5��Ӧ��IDΪ21��
�������ƣ����Լ������λ = (103 + 1*25 + 2*21 + 3*18 + 4*23 + 5*16 + 6*16 + 7*23 + 8*24) % 103 = 21��������λ��IDΪ21��

���ձ����95270078 �����ʾΪ��
��ʼλStartA(bbsbssssbss)+����λ[9(bbbssbsbbss)+5(bbsbbbssbss)+2(bbssbbbssbs)+7(bbbsbbsbbbs)+0(bssbbbsbbss)+0(bssbbbsbbss)+7(bbbsbbsbbbs)+8(bbbsbssbbss)]+����λ21(bbsbbbssbss)+����λStop(bbsssbbbsbsbb),
����bbsbssssbssbbbssbsbbssbbsbbbssbssbbssbbbssbsbbbsbbsbbbsbssbbbsbbssbssbbbsbbssbbbsbbsbbbsbbbsbssbbssbbsbbbssbssbbsssbbbsbsbb�� 
��Ҫ��ӡ��ֻ�轫b�ú�ɫ�߱����s�ð�ɫ�߱����һ���򵥵����������ɳ��������ˣ�

128B��128A���ƣ�128Cֻ�ܶԳ���Ϊż�������ִ����룬ÿ��������Ϊһλ�������������Ϣѹ����һ�룬��ӡ�����������Ҳ�ͽ϶̡�����������1λ���� 95��ӦIDΪ95����2λ����27��ӦIDΪ27����3λ����00��ӦIDΪ0����4λ����78��ӦIDΪ78�����Լ���λ = (105 + 1*95 + 2*27 + 3*0 + 4*78) % 103 = 51

EAN128��Code128C��ͬ��ֻ���ڿ�ʼλ��Ӷ�һ������λFNC1��IDΪ102����ͬʱ��FNC1��Ϊ��1λ���ݼ��뵽����λ�ļ��㡣

���ַ�ʽ�ı������������£�


���뷽ʽ	��ʼλ	FNC1	����λ							����λ																		����λ	������
Code128A	StartA	��		9 + 5 + 2 + 7 + 0 + 0 + 7 + 8	(103 + 1*25 + 2*21 + 3*18 + 4*23 + 5*16 + 6*16 + 7*23 + 8*24) % 103 = 21	Stop	bbsbssssbssbbbssbsbbssbbsbbbssbssbbssbbbssbsbbbsbbsbbbsbssbbbsbbssbssbbbsbbssbbbsbbsbbbsbbbsbssbbssbbsbbbssbssbbsssbbbsbsbb
Code128B	StartB	��		9 + 5 + 2 + 7 + 0 + 0 + 7 + 8	(104 + 1*25 + 2*21 + 3*18 + 4*23 + 5*16 + 6*16 + 7*23 + 8*24) % 103 = 22	Stop	bbsbssbssssbbbssbsbbssbbsbbbssbssbbssbbbssbsbbbsbbsbbbsbssbbbsbbssbssbbbsbbssbbbsbbsbbbsbbbsbssbbssbbssbbbsbssbbsssbbbsbsbb
Code128C	StartC	��		95 + 27 + 00 + 78				(105 + 1*95 + 2*27 + 3*0 + 4*78) % 103 = 51									Stop	bbsbssbbbssbsbbbbsbsssbbbsbbssbssbbsbbssbbssbbssssbsbssbbsbbbsbsssbbsssbbbsbsbb
EAN128		StartC	FNC1	95 + 27 + 00 + 78				(105 + 1*102 + 2*95 + 3*27 + 4*0 + 5*78) % 103 = 44							Stop	bbsbssbbbssbbbbsbsbbbsbsbbbbsbsssbbbsbbssbssbbsbbssbbssbbssssbsbssbsssbbsbbbsbbsssbbbsbsbb
****************************************************************************************************/
#define IMEI_LEN	15

typedef struct{
	uint8_t byte;
	uint8_t str[15];
}bar_code128_t;

#endif/*__IMEI_H__*/