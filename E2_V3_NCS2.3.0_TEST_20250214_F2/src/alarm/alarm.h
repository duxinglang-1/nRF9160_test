/****************************************Copyright (c)************************************************
** File name:			    alarm.h
** Last modified Date:          
** Last Version:		   
** Descriptions:		   	ʹ�õ�ncs�汾-1.2		
** Created by:				л��
** Created date:			2020-11-03
** Version:			    	1.0
** Descriptions:			ϵͳ���ӹ���h�ļ�
******************************************************************************************************/
extern bool vibrate_start_flag;
extern bool vibrate_stop_flag;
extern bool alarm_is_running;
extern bool find_is_running;

extern void AlarmRemindInit(void);
extern void AlarmRemindStart(void);
extern void AlarmRemindStop(void);
extern void AlarmMsgProcess(void);
extern void FindDeviceEntryScreen(void);
extern void FindDeviceStop(void);
