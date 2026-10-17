/* Private includes -----------------------------------------------------------*/
//includes
#include "string.h"
#include "stdio.h"

#include "main.h"
#include "stm32f4xx_it.h"
#include "rtc.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "event_groups.h"
#include "Serial.h"
#include "version.h"

#include "user_TasksInit.h"
#include "user_MessageSendTask.h"

#include "ui.h"
#include "ui_EnvPage.h"
#include "ui_HRPage.h"
#include "ui_SPO2Page.h"
#include "ui_HomePage.h"
#include "ui_DateTimeSetPage.h"

#include "HWDataAccess.h"

struct
{
	RTC_DateTypeDef nowdate;
	RTC_TimeTypeDef nowtime;
	int8_t humi;
	int8_t temp;
	uint8_t HR;
	uint8_t SPO2;
	uint16_t stepNum;
}BLEMessage;

struct
{
	RTC_DateTypeDef nowdate;
	RTC_TimeTypeDef nowtime;
}TimeSetMessage;



void StrCMD_Get(uint8_t * str,uint8_t * cmd)
{
	uint8_t i=0;
  while(str[i]!='=')
  {
      cmd[i] = str[i];
      i++;
  }
}

uint8_t TimeFormat_Get(uint8_t * str)
{
	TimeSetMessage.nowdate.RTC_Year = (str[8]-'0')*10+str[9]-'0';
	TimeSetMessage.nowdate.RTC_Month = (str[10]-'0')*10+str[11]-'0';
	TimeSetMessage.nowdate.RTC_Date = (str[12]-'0')*10+str[13]-'0';
	TimeSetMessage.nowtime.RTC_Hours = (str[14]-'0')*10+str[15]-'0';
	TimeSetMessage.nowtime.RTC_Minutes = (str[16]-'0')*10+str[17]-'0';
	TimeSetMessage.nowtime.RTC_Seconds = (str[18]-'0')*10+str[19]-'0';
	if(TimeSetMessage.nowdate.RTC_Year>0 && TimeSetMessage.nowdate.RTC_Year<99
		&& TimeSetMessage.nowdate.RTC_Month>0 && TimeSetMessage.nowdate.RTC_Month<=12
		&& TimeSetMessage.nowdate.RTC_Date>0 && TimeSetMessage.nowdate.RTC_Date<=31
		&& TimeSetMessage.nowtime.RTC_Hours>=0 && TimeSetMessage.nowtime.RTC_Hours<=23
		&& TimeSetMessage.nowtime.RTC_Minutes>=0 && TimeSetMessage.nowtime.RTC_Minutes<=59
		&& TimeSetMessage.nowtime.RTC_Seconds>=0 && TimeSetMessage.nowtime.RTC_Seconds<=59)
	{
		RTC_SetDate1(TimeSetMessage.nowdate.RTC_Year, TimeSetMessage.nowdate.RTC_Month, TimeSetMessage.nowdate.RTC_Date);
		RTC_SetTime1(TimeSetMessage.nowtime.RTC_Hours, TimeSetMessage.nowtime.RTC_Minutes, TimeSetMessage.nowtime.RTC_Seconds);
		printf("TIMESETOK\r\n");
	}
	return 0;   

}

/**
  * @brief  send the message via BLE, use uart
  * @param  argument: Not used
  * @retval None
  */
void MessageSendTask(void *argument)
{
	while(1)
	{

		if(HardInt_uart_flag)
		{
			HardInt_uart_flag = 0;
			{
			uint8_t IdleBreakstr = 0;
			xQueueSend(IdleBreak_MessageQueue,&IdleBreakstr,0);
			printf("RecStr:%s\r\n",HardInt_receive_str);
			if(!strcmp(HardInt_receive_str,"OV"))
			{
				printf("OK\r\n");
			}
			else if(!strcmp(HardInt_receive_str,"OV+VERSION"))
			{
				printf("VERSION=V%d.%d.%d\r\n", VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH);
			}
			else if(!strcmp(HardInt_receive_str,"OV+SEND"))
			{
				RTC_GetTime(RTC_Format_BIN, &(BLEMessage.nowtime));
				RTC_GetDate(RTC_Format_BIN, &BLEMessage.nowdate);
				BLEMessage.humi = HWInterface.AHT21.humidity;
				BLEMessage.temp = HWInterface.AHT21.temperature;
				BLEMessage.HR = HWInterface.HR_meter.HrRate;
				BLEMessage.SPO2 = HWInterface.HR_meter.SPO2;
				BLEMessage.stepNum = HWInterface.IMU.Steps;

				printf("data:%2d-%02d\r\n",BLEMessage.nowdate.RTC_Month,BLEMessage.nowdate.RTC_Date);
				printf("time:%02d:%02d:%02d\r\n",BLEMessage.nowtime.RTC_Hours,BLEMessage.nowtime.RTC_Minutes,BLEMessage.nowtime.RTC_Seconds);
				printf("humidity:%d%%\r\n",BLEMessage.humi);
				printf("temperature:%d\r\n",BLEMessage.temp);
				printf("Heart Rate:%d%%\r\n",BLEMessage.HR);
				printf("SPO2:%d%%\r\n",BLEMessage.SPO2);
				printf("Step today:%d\r\n",BLEMessage.stepNum);
			}
			//set time//OV+ST=20230629125555
			else if(strlen(HardInt_receive_str)==20)
			{
				uint8_t cmd[10];
				memset(cmd,0,sizeof(cmd));
				StrCMD_Get(HardInt_receive_str,cmd);
				if(ui_APPSy_EN && !strcmp(cmd,"OV+ST"))
				{
					TimeFormat_Get(HardInt_receive_str);
				}
			}
			memset(HardInt_receive_str,0,sizeof(HardInt_receive_str));
			}
		}


		vTaskDelay(pdMS_TO_TICKS(1));
	}
}


