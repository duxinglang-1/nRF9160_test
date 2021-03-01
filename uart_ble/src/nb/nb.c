/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr.h>
#include <stdio.h>
#include <drivers/uart.h>
#include <string.h>
#include <net/mqtt.h>
#include <net/socket.h>
#include <modem/lte_lc.h>
#if defined(CONFIG_LWM2M_CARRIER)
#include <lwm2m_carrier.h>
#endif
#if defined(CONFIG_BSD_LIBRARY)
#include <modem/bsdlib.h>
#include <bsd.h>
#include <modem/lte_lc.h>
#include <modem/modem_info.h>
#endif /* CONFIG_BSD_LIBRARY */
#include "lcd.h"
#include "font.h"
#include "settings.h"
#include "datetime.h"
#include "nb.h"
#include "screen.h"

#include <logging/log_ctrl.h>
#include <logging/log.h>
LOG_MODULE_REGISTER(nb, CONFIG_LOG_DEFAULT_LEVEL);

static struct k_work_q *app_work_q;
static struct k_delayed_work link_work;

#define SHOW_LOG_IN_SCREEN		//xb add 20201029 将NB测试状态LOG信息显示在屏幕上

NB_SIGNL_LEVEL g_nb_sig = NB_SIG_LEVEL_NO;

static struct modem_param_info modem_param;
static bool nb_redraw_sig_flag = false;
static bool get_modem_infor = false;

//add by liming
#if defined(CONFIG_MQTT_LIB_TLS)
static sec_tag_t sec_tag_list[] = { CONFIG_SEC_TAG };
#endif /* defined(CONFIG_MQTT_LIB_TLS) */ 

/* Buffers for MQTT client. */
static u8_t rx_buffer[CONFIG_MQTT_MESSAGE_BUFFER_SIZE];
static u8_t tx_buffer[CONFIG_MQTT_MESSAGE_BUFFER_SIZE];
static u8_t payload_buf[CONFIG_MQTT_PAYLOAD_BUFFER_SIZE];

/* The mqtt client struct */
static struct mqtt_client client;

/* MQTT Broker details. */
static struct sockaddr_storage broker;

/* Connected flag */
static bool connected;

/* File descriptor */
static struct pollfd fds;

//#define TEST_BNT_LED   //liming

bool app_nb_on = false;
bool nb_is_running = false;

u8_t g_imsi[IMSI_MAX_LEN] = {0};
u8_t g_imei[IMEI_MAX_LEN] = {0};

#ifdef SHOW_LOG_IN_SCREEN
static u8_t tmpbuf[128] = {0};

void show_infor(u8_t *strbuf)
{
	LCD_Clear(BLACK);
	LCD_ShowStringInRect(20,90,200,50,strbuf);
}
#endif

#if defined(CONFIG_LWM2M_CARRIER)
K_SEM_DEFINE(carrier_registered, 0, 1);

void lwm2m_carrier_event_handler(const lwm2m_carrier_event_t *event)
{
	switch (event->type) {
	case LWM2M_CARRIER_EVENT_BSDLIB_INIT:
		LOG_INF("LWM2M_CARRIER_EVENT_BSDLIB_INIT\n");
	#ifdef SHOW_LOG_IN_SCREEN	
		show_infor("LWM2M_CARRIER_EVENT_BSDLIB_INIT");
	#endif
		break;
	case LWM2M_CARRIER_EVENT_CONNECT:
		LOG_INF("LWM2M_CARRIER_EVENT_CONNECT\n");
	#ifdef SHOW_LOG_IN_SCREEN
		show_infor("LWM2M_CARRIER_EVENT_CONNECT");
	#endif
		break;
	case LWM2M_CARRIER_EVENT_DISCONNECT:
		LOG_INF("LWM2M_CARRIER_EVENT_DISCONNECT\n");
	#ifdef SHOW_LOG_IN_SCREEN
		show_infor("LWM2M_CARRIER_EVENT_DISCONNECT");
	#endif
		break;
	case LWM2M_CARRIER_EVENT_READY:
		LOG_INF("LWM2M_CARRIER_EVENT_READY\n");
	#ifdef SHOW_LOG_IN_SCREEN
		show_infor("LWM2M_CARRIER_EVENT_READY");
	#endif
		k_sem_give(&carrier_registered);
		break;
	case LWM2M_CARRIER_EVENT_FOTA_START:
		LOG_INF("LWM2M_CARRIER_EVENT_FOTA_START\n");
	#ifdef SHOW_LOG_IN_SCREEN
		show_infor("LWM2M_CARRIER_EVENT_FOTA_START");
	#endif
		break;
	case LWM2M_CARRIER_EVENT_REBOOT:
		LOG_INF("LWM2M_CARRIER_EVENT_REBOOT\n");
	#ifdef SHOW_LOG_IN_SCREEN	
		show_infor("LWM2M_CARRIER_EVENT_REBOOT");
	#endif
		break;
	}
}
#endif /* defined(CONFIG_LWM2M_CARRIER) */

/**@brief Function to print strings without null-termination
 */
static void data_print(u8_t *prefix, u8_t *data, size_t len)
{
	char buf[len + 1];

	memcpy(buf, data, len);
	buf[len] = 0;
	LOG_INF("%s%s\n", prefix, buf);
}

/**@brief Function to publish data on the configured topic
 */
static int data_publish(struct mqtt_client *c, enum mqtt_qos qos,
	u8_t *data, size_t len)
{
	struct mqtt_publish_param param;

	param.message.topic.qos = qos;
	param.message.topic.topic.utf8 = CONFIG_MQTT_PUB_TOPIC;
	param.message.topic.topic.size = strlen(CONFIG_MQTT_PUB_TOPIC);
	param.message.payload.data = data;
	param.message.payload.len = len;
	param.message_id = sys_rand32_get();
	param.dup_flag = 0;
	param.retain_flag = 0;

	data_print("Publishing: ", data, len);
	LOG_INF("to topic: %s len: %u\n",
		CONFIG_MQTT_PUB_TOPIC,
		(unsigned int)strlen(CONFIG_MQTT_PUB_TOPIC));
#ifdef SHOW_LOG_IN_SCREEN	
	sprintf(tmpbuf, "to topic: %s len: %u",
		CONFIG_MQTT_PUB_TOPIC,
		(unsigned int)strlen(CONFIG_MQTT_PUB_TOPIC));
	show_infor(tmpbuf);
#endif

	return mqtt_publish(c, &param);
}

/**@brief Function to subscribe to the configured topic
 */
static int subscribe(void)
{
	struct mqtt_topic subscribe_topic = {
		.topic = {
			.utf8 = CONFIG_MQTT_SUB_TOPIC,
			.size = strlen(CONFIG_MQTT_SUB_TOPIC)
		},
		.qos = MQTT_QOS_1_AT_LEAST_ONCE
	};

	const struct mqtt_subscription_list subscription_list = {
		.list = &subscribe_topic,
		.list_count = 1,
		.message_id = 1234
	};

	LOG_INF("Subscribing to: %s len %u\n", CONFIG_MQTT_SUB_TOPIC,
		(unsigned int)strlen(CONFIG_MQTT_SUB_TOPIC));
#ifdef SHOW_LOG_IN_SCREEN
	sprintf(tmpbuf, "Subscribing to: %s len %u", CONFIG_MQTT_SUB_TOPIC,
		(unsigned int)strlen(CONFIG_MQTT_SUB_TOPIC));
	show_infor(tmpbuf);
#endif

	return mqtt_subscribe(&client, &subscription_list);
}

/**@brief Function to read the published payload.
 */
static int publish_get_payload(struct mqtt_client *c, size_t length)
{
	u8_t *buf = payload_buf;
	u8_t *end = buf + length;

	if (length > sizeof(payload_buf)) {
		return -EMSGSIZE;
	}

	while (buf < end) {
		int ret = mqtt_read_publish_payload(c, buf, end - buf);

		if (ret < 0) {
			int err;

			if (ret != -EAGAIN) {
				return ret;
			}

			LOG_INF("mqtt_read_publish_payload: EAGAIN\n");
		#ifdef SHOW_LOG_IN_SCREEN	
			show_infor("mqtt_read_publish_payload: EAGAIN");
		#endif
		
			err = poll(&fds, 1,
				   CONFIG_MQTT_KEEPALIVE * MSEC_PER_SEC);
			if (err > 0 && (fds.revents & POLLIN) == POLLIN) {
				continue;
			} else {
				return -EIO;
			}
		}

		if (ret == 0) {
			return -EIO;
		}

		buf += ret;
	}

	return 0;
}

/**@brief MQTT client event handler
 */
void mqtt_evt_handler(struct mqtt_client *const c,
		      const struct mqtt_evt *evt)
{
	int err;

	switch(evt->type)
	{
	case MQTT_EVT_CONNACK:
		if (evt->result != 0)
		{
			LOG_INF("MQTT connect failed %d\n", evt->result);
		#ifdef SHOW_LOG_IN_SCREEN	
			sprintf(tmpbuf, "MQTT connect failed %d", evt->result);
			show_infor(tmpbuf);
		#endif
		
			break;
		}

		connected = true;
		LOG_INF("[%s:%d] MQTT client connected!\n", __func__, __LINE__);
	#ifdef SHOW_LOG_IN_SCREEN	
		sprintf(tmpbuf, "[%s:%d] MQTT client connected!", __func__, __LINE__);
		show_infor(tmpbuf);
	#endif
		subscribe();
		break;

	case MQTT_EVT_DISCONNECT:
		LOG_INF("[%s:%d] MQTT client disconnected %d\n", __func__,
		       __LINE__, evt->result);
	#ifdef SHOW_LOG_IN_SCREEN	
		sprintf(tmpbuf, "[%s:%d] MQTT client disconnected %d", __func__,
		       __LINE__, evt->result);
		show_infor(tmpbuf);
	#endif
	
		connected = false;
		break;

	case MQTT_EVT_PUBLISH: 
		{
			const struct mqtt_publish_param *p = &evt->param.publish;

			LOG_INF("[%s:%d] MQTT PUBLISH result=%d len=%d\n", __func__,
			       __LINE__, evt->result, p->message.payload.len);
		#ifdef SHOW_LOG_IN_SCREEN
			sprintf(tmpbuf, "[%s:%d] MQTT PUBLISH result=%d len=%d", __func__,
			       __LINE__, evt->result, p->message.payload.len);
			show_infor(tmpbuf);
		#endif
		
			err = publish_get_payload(c, p->message.payload.len);
			if(err >= 0)
			{
				data_print("Received: ", payload_buf,
					p->message.payload.len);
				/* Echo back received data */
				data_publish(&client, MQTT_QOS_1_AT_LEAST_ONCE,
					payload_buf, p->message.payload.len);
			}
			else
			{
				LOG_INF("mqtt_read_publish_payload: Failed! %d\n", err);
				LOG_INF("Disconnecting MQTT client...\n");
			#ifdef SHOW_LOG_IN_SCREEN	
				sprintf(tmpbuf, "mqtt_read_publish_payload: Failed! %d", err);
				show_infor(tmpbuf);
				show_infor("Disconnecting MQTT client...");
			#endif
			
				err = mqtt_disconnect(c);
				if(err)
				{
					LOG_INF("Could not disconnect: %d\n", err);
				#ifdef SHOW_LOG_IN_SCREEN	
					sprintf(tmpbuf, "Could not disconnect: %d", err);
					show_infor(tmpbuf);
				#endif
				}
			}
		} 
		break;

	case MQTT_EVT_PUBACK:
		if(evt->result != 0)
		{
			LOG_INF("MQTT PUBACK error %d\n", evt->result);
		#ifdef SHOW_LOG_IN_SCREEN
			sprintf(tmpbuf, "MQTT PUBACK error %d", evt->result);
			show_infor(tmpbuf);
		#endif
			break;
		}

		LOG_INF("[%s:%d] PUBACK packet id: %u\n", __func__, __LINE__,
				evt->param.puback.message_id);
	#ifdef SHOW_LOG_IN_SCREEN
		sprintf(tmpbuf, "[%s:%d] PUBACK packet id: %u", __func__, __LINE__,
				evt->param.puback.message_id);
		show_infor(tmpbuf);
	#endif
		break;

	case MQTT_EVT_SUBACK:
		if(evt->result != 0)
		{
			LOG_INF("MQTT SUBACK error %d\n", evt->result);
		#ifdef SHOW_LOG_IN_SCREEN
			sprintf(tmpbuf, "MQTT SUBACK error %d", evt->result);
			show_infor(tmpbuf);
		#endif
			break;
		}

		LOG_INF("[%s:%d] SUBACK packet id: %u\n", __func__, __LINE__,
				evt->param.suback.message_id);
	#ifdef SHOW_LOG_IN_SCREEN
		sprintf(tmpbuf, "[%s:%d] SUBACK packet id: %u", __func__, __LINE__,
				evt->param.suback.message_id);
		show_infor(tmpbuf);
	#endif
		break;

	default:
		LOG_INF("[%s:%d] default: %d\n", __func__, __LINE__,
				evt->type);
	#ifdef SHOW_LOG_IN_SCREEN
		sprintf(tmpbuf, "[%s:%d] default: %d", __func__, __LINE__,
				evt->type);
		show_infor(tmpbuf);
	#endif
		break;
	}
}

/**@brief Resolves the configured hostname and
 * initializes the MQTT broker structure
 */
static void broker_init(void)
{
	int err;
	struct addrinfo *result;
	struct addrinfo *addr;
	struct addrinfo hints = {
		.ai_family = AF_INET,
		.ai_socktype = SOCK_STREAM
	};

	err = getaddrinfo(CONFIG_MQTT_BROKER_HOSTNAME, NULL, &hints, &result);
	if(err)
	{
		LOG_INF("ERROR: getaddrinfo failed %d\n", err);
	#ifdef SHOW_LOG_IN_SCREEN
		sprintf(tmpbuf, "ERROR: getaddrinfo failed %d", err);
		show_infor(tmpbuf);	
	#endif
	
		return;
	}

	addr = result;
	err = -ENOENT;

	/* Look for address of the broker. */
	while(addr != NULL)
	{
		/* IPv4 Address. */
		if (addr->ai_addrlen == sizeof(struct sockaddr_in))
		{
			struct sockaddr_in *broker4 =
				((struct sockaddr_in *)&broker);
			char ipv4_addr[NET_IPV4_ADDR_LEN];

			broker4->sin_addr.s_addr =
				((struct sockaddr_in *)addr->ai_addr)
				->sin_addr.s_addr;
			broker4->sin_family = AF_INET;
			broker4->sin_port = htons(CONFIG_MQTT_BROKER_PORT);

			inet_ntop(AF_INET, &broker4->sin_addr.s_addr,
				  ipv4_addr, sizeof(ipv4_addr));
			LOG_INF("IPv4 Address found %s\n", ipv4_addr);
		#ifdef SHOW_LOG_IN_SCREEN
			sprintf(tmpbuf, "IPv4 Address found %s", ipv4_addr);
			show_infor(tmpbuf);
		#endif

			break;
		}
		else
		{
			LOG_INF("ai_addrlen = %u should be %u or %u\n",
				(unsigned int)addr->ai_addrlen,
				(unsigned int)sizeof(struct sockaddr_in),
				(unsigned int)sizeof(struct sockaddr_in6));
		#ifdef SHOW_LOG_IN_SCREEN
			sprintf(tmpbuf, "ai_addrlen = %u should be %u or %u",
				(unsigned int)addr->ai_addrlen,
				(unsigned int)sizeof(struct sockaddr_in),
				(unsigned int)sizeof(struct sockaddr_in6));
			show_infor(tmpbuf);
		#endif
		}

		addr = addr->ai_next;
		break;
	}

	/* Free the address. */
	freeaddrinfo(result);
}

/**@brief Initialize the MQTT client structure
 */
static void client_init(struct mqtt_client *client)
{
	mqtt_client_init(client);

	broker_init();

	/* MQTT client configuration */
	client->broker = &broker;
	client->evt_cb = mqtt_evt_handler;
	client->client_id.utf8 = (u8_t *)CONFIG_MQTT_CLIENT_ID;
	client->client_id.size = strlen(CONFIG_MQTT_CLIENT_ID);
	client->password = NULL;
	client->user_name = NULL;
	client->protocol_version = MQTT_VERSION_3_1_1;

	/* MQTT buffers configuration */
	client->rx_buf = rx_buffer;
	client->rx_buf_size = sizeof(rx_buffer);
	client->tx_buf = tx_buffer;
	client->tx_buf_size = sizeof(tx_buffer);
/* MQTT transport configuration */  //add by liming
#if defined(CONFIG_MQTT_LIB_TLS)
    struct mqtt_sec_config *tls_config = &client->transport.tls.config;
    
    client->transport.type = MQTT_TRANSPORT_SECURE;
    
    tls_config->peer_verify = CONFIG_PEER_VERIFY;
    tls_config->cipher_count = 0;
    tls_config->cipher_list = NULL;
    tls_config->sec_tag_count = ARRAY_SIZE(sec_tag_list);
    tls_config->sec_tag_list = sec_tag_list;
    tls_config->hostname = CONFIG_MQTT_BROKER_HOSTNAME;
#else /* MQTT transport configuration */
    client->transport.type = MQTT_TRANSPORT_NON_SECURE;
#endif /* defined(CONFIG_MQTT_LIB_TLS) */

}

/**@brief Initialize the file descriptor structure used by poll.
 */
static int fds_init(struct mqtt_client *c)
{
	if(c->transport.type == MQTT_TRANSPORT_NON_SECURE)
	{
		fds.fd = c->transport.tcp.sock;
	}
	else
	{
	#if defined(CONFIG_MQTT_LIB_TLS)
		fds.fd = c->transport.tls.sock;
	#else
		return -ENOTSUP;
	#endif
	}

	fds.events = POLLIN;

	return 0;
}

#if CONFIG_MODEM_INFO
/**@brief Callback handler for LTE RSRP data. */
static void modem_rsrp_handler(char rsrp_value)
{
	/* RSRP raw values that represent actual signal strength are
	 * 0 through 97 (per "nRF91 AT Commands" v1.1). If the received value
	 * falls outside this range, we should not send the value.
	 */
	LOG_INF("rsrp_value:%d\n", rsrp_value);

	if(rsrp_value > 97)
	{
		g_nb_sig = NB_SIG_LEVEL_NO;
	}
	else if(rsrp_value >= 80)
	{
		g_nb_sig = NB_SIG_LEVEL_4;
	}
	else if(rsrp_value >= 60)
	{
		g_nb_sig = NB_SIG_LEVEL_3;
	}
	else if(rsrp_value >= 40)
	{
		g_nb_sig = NB_SIG_LEVEL_2;
	}
	else if(rsrp_value >= 20)
	{
		g_nb_sig = NB_SIG_LEVEL_1;
	}
	else
	{
		g_nb_sig = NB_SIG_LEVEL_0;
	}

	nb_redraw_sig_flag = true;
}

/**brief Initialize LTE status containers. */
void modem_data_init(void)
{
	int err;

	err = modem_info_init();
	if(err)
	{
		LOG_INF("Modem info could not be established: %d", err);
		return;
	}

	modem_info_params_init(&modem_param);
	modem_info_rsrp_register(modem_rsrp_handler);
}
#endif /* CONFIG_MODEM_INFO */

/**@brief Configures modem to provide LTE link. Blocks until link is
 * successfully established.
 */
static void modem_configure(void)
{
#ifdef SHOW_LOG_IN_SCREEN
	show_infor("modem_configure");
#endif

#if 1 //xb test 20200927
	if(at_cmd_write("AT%CESQ=1", NULL, 0, NULL) != 0)
	{
		LOG_INF("AT_CMD write fail!\n");
		return;
	}
#endif

#if defined(CONFIG_LTE_LINK_CONTROL)
	if (IS_ENABLED(CONFIG_LTE_AUTO_INIT_AND_CONNECT))
	{
		/* Do nothing, modem is already turned on
		 * and connected.
		 */
	}
	else
	{
	#if defined(CONFIG_LWM2M_CARRIER)
		/* Wait for the LWM2M_CARRIER to configure the modem and
		 * start the connection.
		 */
		LOG_INF("Waitng for carrier registration...\n");
	  #ifdef SHOW_LOG_IN_SCREEN
		show_infor("Waitng for carrier registration...");
	  #endif
		k_sem_take(&carrier_registered, K_FOREVER);
		LOG_INF("Registered!\n");
	  #ifdef SHOW_LOG_IN_SCREEN
		show_infor("Registered!");
	  #endif
	#else /* defined(CONFIG_LWM2M_CARRIER) */
		int err;

		LOG_INF("LTE Link Connecting ...\n");
	  #ifdef SHOW_LOG_IN_SCREEN	
		show_infor("LTE Link Connecting ...");
	  #endif
		err = lte_lc_init_and_connect();
		__ASSERT(err == 0, "LTE link could not be established.");
		LOG_INF("LTE Link Connected!\n");
	  #ifdef SHOW_LOG_IN_SCREEN
		show_infor("LTE Link Connected!");
	  #endif
#endif /* defined(CONFIG_LWM2M_CARRIER) */
	}
#endif /* defined(CONFIG_LTE_LINK_CONTROL) */
}

void test_nb(void)
{
	int err;

	nb_is_running = true;
	
	LOG_INF("Start NB-IoT test!\n");

	EnterNBTestScreen();
	
#ifdef SHOW_LOG_IN_SCREEN
	show_infor("Start NB-IoT test!");
#endif

	modem_configure();

	client_init(&client);

	err = mqtt_connect(&client);
	if(err != 0)
	{
		LOG_INF("ERROR: mqtt_connect %d\n", err);
	#ifdef SHOW_LOG_IN_SCREEN
		sprintf(tmpbuf, "ERROR: mqtt_connect %d", err);
		show_infor(tmpbuf);
	#endif
	
		return;
	}

	err = fds_init(&client);
	if(err != 0)
	{
		LOG_INF("ERROR: fds_init %d\n", err);
	#ifdef SHOW_LOG_IN_SCREEN
		sprintf(tmpbuf, "ERROR: fds_init %d", err);
		show_infor(tmpbuf);
	#endif
	
		return;
	}

	while(1)
	{
		err = poll(&fds, 1, mqtt_keepalive_time_left(&client));
		if(err < 0)
		{
			LOG_INF("ERROR: poll %d\n", errno);
		#ifdef SHOW_LOG_IN_SCREEN
			sprintf(tmpbuf, "ERROR: poll %d", errno);
			show_infor(tmpbuf);
		#endif
		
			break;
		}

		err = mqtt_live(&client);
		if((err != 0) && (err != -EAGAIN))
		{
			LOG_INF("ERROR: mqtt_live %d\n", err);
		#ifdef SHOW_LOG_IN_SCREEN
			sprintf(tmpbuf, "ERROR: mqtt_live %d", err);
			show_infor(tmpbuf);
		#endif
		
			break;
		}

		if((fds.revents & POLLIN) == POLLIN)
		{
			err = mqtt_input(&client);
			if(err != 0)
			{
				LOG_INF("ERROR: mqtt_input %d\n", err);
			#ifdef SHOW_LOG_IN_SCREEN
				sprintf(tmpbuf, "ERROR: mqtt_input %d", err);
				show_infor(tmpbuf);
			#endif
			
				break;
			}
		}

		if((fds.revents & POLLERR) == POLLERR)
		{
			LOG_INF("POLLERR\n");
		#ifdef SHOW_LOG_IN_SCREEN	
			show_infor("POLLERR");
		#endif
		
			break;
		}

		if((fds.revents & POLLNVAL) == POLLNVAL)
		{
			LOG_INF("POLLNVAL\n");
		#ifdef SHOW_LOG_IN_SCREEN
			show_infor("POLLNVAL");
		#endif
		
			break;
		}
	}

	LOG_INF("Disconnecting MQTT client...\n");
#ifdef SHOW_LOG_IN_SCREEN	
	show_infor("Disconnecting MQTT client...");
#endif

	err = mqtt_disconnect(&client);
	if(err)
	{
		LOG_INF("Could not disconnect MQTT client. Error: %d\n", err);
	#ifdef SHOW_LOG_IN_SCREEN	
		sprintf(tmpbuf, "Could not disconnect MQTT client. Error: %d", err);
		show_infor(tmpbuf);
	#endif
	}
}

void APP_Ask_NB(void)
{
	app_nb_on = true;
}

void NBRedrawSignal(void)
{
	if(screen_id == SCREEN_ID_IDLE)
	{
		scr_msg[screen_id].para |= SCREEN_EVENT_UPDATE_SIG;
		scr_msg[screen_id].act = SCREEN_ACTION_UPDATE;
	}
}

void GetModemDateTime(void)
{
	char *ptr;
	u8_t timebuf[128] = {0};
	u8_t tmpbuf[10] = {0};
	u8_t tz_dir[2],tz_count;

	if(at_cmd_write("AT+CCLK?", timebuf, sizeof(timebuf), NULL) != 0)
	{
		LOG_INF("Get CCLK fail!\n");
		return;
	}
	LOG_INF("%s\n", timebuf);

	//+CCLK: "21/02/26,08:31:33+32"
	ptr = strstr(timebuf, "\"");
	if(ptr)
	{
		ptr++;
		
		memcpy(tmpbuf, ptr, 2);
		date_time.year = 2000+atoi(tmpbuf);
		ptr+=3;

		memset(tmpbuf, 0, sizeof(tmpbuf));
		memcpy(tmpbuf, ptr, 2);
		date_time.month= atoi(tmpbuf);
		ptr+=3;

		memset(tmpbuf, 0, sizeof(tmpbuf));
		memcpy(tmpbuf, ptr, 2);
		date_time.day = atoi(tmpbuf);
		ptr+=3;

		memset(tmpbuf, 0, sizeof(tmpbuf));
		memcpy(tmpbuf, ptr, 2);
		date_time.hour = atoi(tmpbuf);
		ptr+=3;

		memset(tmpbuf, 0, sizeof(tmpbuf));
		memcpy(tmpbuf, ptr, 2);
		date_time.minute = atoi(tmpbuf);
		ptr+=3;

		memset(tmpbuf, 0, sizeof(tmpbuf));
		memcpy(tmpbuf, ptr, 2);
		date_time.second = atoi(tmpbuf);
		ptr+=2;

		memcpy(tz_dir, ptr, 1);
		ptr+=1;

		memset(tmpbuf, 0, sizeof(tmpbuf));
		memcpy(tmpbuf, ptr, 2);
		tz_count = atoi(tmpbuf);
		if(tz_dir[0] == '+')
		{
			TimeIncrease(&date_time, tz_count*15);
		}
		else if(tz_dir[0] == '-')
		{
			TimeDecrease(&date_time, tz_count*15);
		}
	}

	LOG_INF("real time:%04d/%02d/%02d,%02d:%02d:%02d,%02d\n", 
					date_time.year,date_time.month,date_time.day,
					date_time.hour,date_time.minute,date_time.second,
					date_time.week);

	RedrawSystemTime();
	SaveSystemDateTime();
}

void GetModemInfor(void)
{
	char *ptr;
	int i=0,len,err;
	u8_t tmpbuf[128] = {0};
	u8_t strbuf[128] = {0};

	if(at_cmd_write(CMD_GET_IMEI, tmpbuf, sizeof(tmpbuf), NULL) != 0)
	{
		LOG_INF("Get imei fail!\n");
		return;
	}

	LOG_INF("imei:%s\n", tmpbuf);
	strncpy(g_imei, tmpbuf, IMEI_MAX_LEN);

	if(at_cmd_write(CMD_GET_IMSI, tmpbuf, sizeof(tmpbuf), NULL) != 0)
	{
		LOG_INF("Get imsi fail!\n");
		return;
	}

	LOG_INF("imsi:%s\n", tmpbuf);
	strncpy(g_imsi, tmpbuf, IMSI_MAX_LEN);

	if(at_cmd_write(CMD_GET_RSRP, tmpbuf, sizeof(tmpbuf), NULL) != 0)
	{
		LOG_INF("Get rsrp fail!\n");
		return;
	}

	LOG_INF("rsrp:%s\n", tmpbuf);
	len = strlen(tmpbuf);
	ptr = tmpbuf;
	while(i<5)
	{
		ptr = strstr(ptr, ",");
		ptr++;
		i++;
	}

	memcpy((char*)strbuf, ptr, len-(ptr-(char*)tmpbuf));
	modem_rsrp_handler(atoi(strbuf));
}

static void nb_link(struct k_work *work)
{
	int err;

	err = lte_lc_init_and_connect();
	if(err)
	{
		LOG_INF("Can't connected to LTE network");
	}
	else
	{
		LOG_INF("Connected to LTE network");

		GetModemDateTime();
		modem_data_init();
	}

	GetModemInfor();
}

void testnetworktime(void)
{
	u8_t tmpbuf[128] = {0};

	if(at_cmd_write("AT%XNETTIME?", tmpbuf, sizeof(tmpbuf), NULL) != 0)
	{
		LOG_INF("Get cfun fail!\n");
		return;
	}
	LOG_INF("%s\n", tmpbuf);

}

void GetNBSignal(void)
{
	u8_t str_rsrp[128] = {0};

	if(at_cmd_write("AT+CFUN?", str_rsrp, sizeof(str_rsrp), NULL) != 0)
	{
		LOG_INF("Get cfun fail!\n");
		return;
	}
	LOG_INF("cfun:%s\n", str_rsrp);

	if(at_cmd_write("AT+CPSMS?", str_rsrp, sizeof(str_rsrp), NULL) != 0)
	{
		LOG_INF("Get cpsms fail!\n");
		return;
	}
	LOG_INF("cpsms:%s\n", str_rsrp);
	
	if(at_cmd_write(CMD_GET_RSRP, str_rsrp, sizeof(str_rsrp), NULL) != 0)
	{
		LOG_INF("Get rsrp fail!\n");
		return;
	}

	LOG_INF("rsrp:%s\n", str_rsrp);
}

void NBMsgProcess(void)
{
	if(app_nb_on)
	{
		app_nb_on = false;
		if(nb_is_running)
			return;
		
		test_nb();
	}

	if(nb_redraw_sig_flag)
	{
		NBRedrawSignal();
		nb_redraw_sig_flag = false;
	}
}

void NB_init(struct k_work_q *work_q)
{
	int err;

	app_work_q = work_q;

	k_work_init(&link_work, nb_link);
	k_work_submit_to_queue(app_work_q, &link_work);
}
