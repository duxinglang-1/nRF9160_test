#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <zephyr.h>
#include <sys/printk.h>
#include <dk_buttons_and_leds.h>
#include "settings.h"

global_settings_t global_settings = {0};

const global_settings_t FACTORY_DEFAULT_DATA = {
	TIME_FORMAT_24,			//24 format
	false,					//heart rate turn on
	false,					//blood pressure turn on
	false,					//blood oxygen turn on
	{						//alarm
		0,
		{false,0,0,0},
		{false,0,0,0},
		{false,0,0,0},
		{false,0,0,0},
		{false,0,0,0},
		{false,0,0,0},
		{false,0,0,0},
		{false,0,0,0}
	},
};
void InitSystemSettings(void)
{
	
}

void SaveSysetemSettings(global_settings_t settings)
{
	
}

void ResetSystemSettings(void)
{
	memcpy(global_settings, FACTORY_DEFAULT_DATA, sizeof(global_settings_t));

	SaveSysetemSettings(global_settings);
}