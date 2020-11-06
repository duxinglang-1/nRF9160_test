/****************************************Copyright (c)************************************************
** File name:			    alarm.h
** Last modified Date:          
** Last Version:		   
** Descriptions:		   	使用的ncs版本-1.2		
** Created by:				谢彪
** Created date:			2020-11-03
** Version:			    	1.0
** Descriptions:			系统闹钟管理h文件
******************************************************************************************************/
extern bool vibrate_start_flag;
extern bool vibrate_stop_flag;
extern bool alarm_is_running;
extern bool find_is_running;

extern void AlarmRemindInit(void);
extern void AlarmRemindStart(void);
extern void AlarmRemindStop(void);

extern void FindDeviceEntryScreen(void);
extern void FindDeviceStop(void);
