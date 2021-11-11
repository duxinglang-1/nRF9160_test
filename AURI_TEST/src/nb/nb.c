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
#include "sos.h"
#include "gps.h"
#include "uart_ble.h"
#ifdef CONFIG_IMU_SUPPORT
#include "lsm6dso.h"
#include "fall.h"
#endif
#include "transfer_cache.h"
#include "logger.h"

#define LTE_TAU_WAKEUP_EARLY_TIME	(60)
#define MQTT_CONNECTED_KEEP_TIME	(1*60)

static void SendDataCallBack(struct k_timer *timer_id);
K_TIMER_DEFINE(send_data_timer, SendDataCallBack, NULL);
static void ParseDataCallBack(struct k_timer *timer_id);
K_TIMER_DEFINE(parse_data_timer, ParseDataCallBack, NULL);
static void MqttDisConnectCallBack(struct k_timer *timer_id);
K_TIMER_DEFINE(mqtt_disconnect_timer, MqttDisConnectCallBack, NULL);
static void GetNetWorkTimeCallBack(struct k_timer *timer_id);
K_TIMER_DEFINE(get_nw_time_timer, GetNetWorkTimeCallBack, NULL);
static void GetNetWorkSignalCallBack(struct k_timer *timer_id);
K_TIMER_DEFINE(get_nw_rsrp_timer, GetNetWorkSignalCallBack, NULL);
static void NBReconnectCallBack(struct k_timer *timer_id);
K_TIMER_DEFINE(nb_reconnect_timer, NBReconnectCallBack, NULL);
#ifdef CONFIG_DEVICE_POWER_MANAGEMENT
static void TauWakeUpUartCallBack(struct k_timer *timer_id);
K_TIMER_DEFINE(tau_wakeup_uart_timer, TauWakeUpUartCallBack, NULL);
#endif


static struct k_work_q *app_work_q;
static struct k_delayed_work nb_link_work;
static struct k_delayed_work mqtt_link_work;

NB_SIGNL_LEVEL g_nb_sig = NB_SIG_LEVEL_NO;

static struct modem_param_info modem_param;
static bool nb_redraw_sig_flag = false;
static bool get_modem_infor = false;
static bool send_data_flag = false;
static bool parse_data_flag = false;
static bool mqtt_disconnect_flag = false;
static bool power_on_data_flag = true;
static bool nb_connecting_flag = false;
static bool mqtt_connecting_flag = false;
static bool nb_reconnect_flag = false;

#if defined(CONFIG_MQTT_LIB_TLS)
static sec_tag_t sec_tag_list[] = { CONFIG_SEC_TAG };
#endif/*CONFIG_MQTT_LIB_TLS*/ 

/* Buffers for MQTT client. */
static u8_t rx_buffer[CONFIG_MQTT_MESSAGE_BUFFER_SIZE];
static u8_t tx_buffer[CONFIG_MQTT_MESSAGE_BUFFER_SIZE];
static u8_t payload_buf[CONFIG_MQTT_PAYLOAD_BUFFER_SIZE];

/* The mqtt client struct */
static struct mqtt_client client;

/* MQTT Broker details. */
static struct sockaddr_storage broker;

/* Connected flag */
static bool nb_connected = false;
static bool mqtt_connected = false;

/* File descriptor */
static struct pollfd fds;

bool test_nb_on = false;
bool nb_is_running = false;
bool get_modem_info_flag = false;
bool get_modem_time_flag = false;
bool get_modem_signal_flag = false;
bool get_modem_cclk_flag = false;
bool test_nb_flag = false;
bool nb_test_update_flag = false;
bool g_nw_registered = false;

u8_t g_imsi[IMSI_MAX_LEN+1] = {0};
u8_t g_imei[IMEI_MAX_LEN+1] = {0};
u8_t g_iccid[ICCID_MAX_LEN+1] = {0};
u8_t g_modem[MODEM_MAX_LEN+1] = {0};
u8_t g_timezone[5] = {0};
u8_t g_rsrp = 0;
u16_t g_tau_time = 0;
u16_t g_act_time = 0;

static void NbSendDataStart(void);
static void NbSendDataStop(void);
static void MqttReceData(u8_t *data, u32_t datalen);
static void MqttDicConnectStart(void);
static void MqttDicConnectStop(void);

u8_t nb_test_info[256] = {0};

#if defined(CONFIG_LWM2M_CARRIER)
K_SEM_DEFINE(carrier_registered, 0, 1);

void lwm2m_carrier_event_handler(const lwm2m_carrier_event_t *event)
{
	switch (event->type) {
	case LWM2M_CARRIER_EVENT_BSDLIB_INIT:
		LOGD("LWM2M_CARRIER_EVENT_BSDLIB_INIT");
		break;
	case LWM2M_CARRIER_EVENT_CONNECT:
		LOGD("LWM2M_CARRIER_EVENT_CONNECT");
		break;
	case LWM2M_CARRIER_EVENT_DISCONNECT:
		LOGD("LWM2M_CARRIER_EVENT_DISCONNECT");
		break;
	case LWM2M_CARRIER_EVENT_READY:
		LOGD("LWM2M_CARRIER_EVENT_READY");
		k_sem_give(&carrier_registered);
		break;
	case LWM2M_CARRIER_EVENT_FOTA_START:
		LOGD("LWM2M_CARRIER_EVENT_FOTA_START");
		break;
	case LWM2M_CARRIER_EVENT_REBOOT:
		LOGD("LWM2M_CARRIER_EVENT_REBOOT");
		break;
	}
}
#endif /* defined(CONFIG_LWM2M_CARRIER) */

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

	return mqtt_publish(c, &param);
}

static char *get_mqtt_topic(void)
{
	static char str_sub_topic[256] = {0};

	sprintf(str_sub_topic, "%s%s", CONFIG_MQTT_SUB_TOPIC, g_imei);
	return str_sub_topic;
}

/**@brief Function to subscribe to the configured topic
 */
static int subscribe(void)
{
	struct mqtt_topic subscribe_topic = {
		.topic = {
			.utf8 = (u8_t *)get_mqtt_topic(),
			.size = strlen(subscribe_topic.topic.utf8)
		},
		.qos = MQTT_QOS_1_AT_LEAST_ONCE
	};

	const struct mqtt_subscription_list subscription_list = {
		.list = &subscribe_topic,
		.list_count = 1,
		.message_id = 0000
	};

	LOGD("Subscribing to:%s, len:%d", subscribe_topic.topic.utf8,
		subscribe_topic.topic.size);
	return mqtt_subscribe(&client, &subscription_list);
}

/**@brief Function to read the published payload.
 */
static int publish_get_payload(struct mqtt_client *c, size_t length)
{
	u8_t *buf = payload_buf;
	u8_t *end = buf + length;

	if(length > sizeof(payload_buf))
	{
		return -EMSGSIZE;
	}

	while(buf < end)
	{
		int ret = mqtt_read_publish_payload(c, buf, end - buf);

		if(ret < 0)
		{
			int err;

			if(ret != -EAGAIN)
			{
				return ret;
			}
			LOGD("mqtt_read_publish_payload: EAGAIN");
			err = poll(&fds, 1,
				   CONFIG_MQTT_KEEPALIVE * MSEC_PER_SEC);
			if(err > 0 && (fds.revents & POLLIN) == POLLIN)
			{
				continue;
			}
			else
			{
				return -EIO;
			}
		}

		if(ret == 0)
		{
			return -EIO;
		}

		buf += ret;
	}

	return 0;
}

/**@brief MQTT client event handler
 */
static void mqtt_evt_handler(struct mqtt_client *const c,
		      const struct mqtt_evt *evt)
{
	int err;

	switch(evt->type)
	{
	case MQTT_EVT_CONNACK:
		if(evt->result != 0)
		{
			LOGD("MQTT connect failed %d", evt->result);
			break;
		}

		mqtt_connected = true;
		LOGD("MQTT client connected!");		
		subscribe();

		if(power_on_data_flag)
		{
			SendPowerOnData();
			power_on_data_flag = false;
		}

		NbSendDataStart();
		MqttDicConnectStart();
		break;

	case MQTT_EVT_DISCONNECT:
		LOGD("MQTT client disconnected %d", evt->result);
		mqtt_connected = false;
		
		NbSendDataStop();
		MqttDicConnectStop();
		break;

	case MQTT_EVT_PUBLISH: 
		{
			const struct mqtt_publish_param *p = &evt->param.publish;
			LOGD("MQTT PUBLISH result=%d len=%d", evt->result, p->message.payload.len);
			err = publish_get_payload(c, p->message.payload.len);
			if(err >= 0)
			{
				MqttReceData(payload_buf, p->message.payload.len);
			}
			else
			{
				LOGD("mqtt_read_publish_payload: Failed! %d", err);
				LOGD("Disconnecting MQTT client...");
				err = mqtt_disconnect(c);
				if(err)
				{
					LOGD("Could not disconnect: %d", err);
				}
			}
		} 
		break;

	case MQTT_EVT_PUBACK:
		if(evt->result != 0)
		{
			LOGD("MQTT PUBACK error %d", evt->result);
			break;
		}
		LOGD("PUBACK packet id: %u", evt->param.puback.message_id);
		break;

	case MQTT_EVT_SUBACK:
		if(evt->result != 0)
		{
			LOGD("MQTT SUBACK error %d", evt->result);
			break;
		}
		LOGD("SUBACK packet id: %u", evt->param.suback.message_id);
		break;

	default:
		LOGD("default: %d", evt->type);
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
		LOGD("ERROR: getaddrinfo failed %d", err);
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
			LOGD("IPv4 Address found %s", ipv4_addr);
			break;
		}
		else
		{
			LOGD("ai_addrlen = %u should be %u or %u",
				(unsigned int)addr->ai_addrlen,
				(unsigned int)sizeof(struct sockaddr_in),
				(unsigned int)sizeof(struct sockaddr_in6));

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
	static struct mqtt_utf8 password;
	static struct mqtt_utf8 username;

	mqtt_client_init(client);

	broker_init();

	/* MQTT client configuration */
	client->broker = &broker;
	
	client->evt_cb = mqtt_evt_handler;
	
	client->client_id.utf8 = (u8_t *)g_imei;	//xb add 2021-03-24 CONFIG_MQTT_CLIENT_ID;
	client->client_id.size = strlen(g_imei);

	password.utf8 = (u8_t *)CONFIG_MQTT_PASSWORD;
	password.size = strlen(CONFIG_MQTT_PASSWORD);
	client->password = &password;

	username.utf8 = (u8_t *)CONFIG_MQTT_USER_NAME;
	username.size = strlen(CONFIG_MQTT_USER_NAME);
	client->user_name = &username;
	
	client->protocol_version = MQTT_VERSION_3_1_1;

	/* MQTT buffers configuration */
	client->rx_buf = rx_buffer;
	client->rx_buf_size = sizeof(rx_buffer);
	client->tx_buf = tx_buffer;
	client->tx_buf_size = sizeof(tx_buffer);

#if defined(CONFIG_MQTT_LIB_TLS)
    struct mqtt_sec_config *tls_config = &client->transport.tls.config;
    
    client->transport.type = MQTT_TRANSPORT_SECURE;
    
    tls_config->peer_verify = CONFIG_PEER_VERIFY;
    tls_config->cipher_count = 0;
    tls_config->cipher_list = NULL;
    tls_config->sec_tag_count = ARRAY_SIZE(sec_tag_list);
    tls_config->sec_tag_list = sec_tag_list;
    tls_config->hostname = CONFIG_MQTT_BROKER_HOSTNAME;
#else
    client->transport.type = MQTT_TRANSPORT_NON_SECURE;
#endif/* defined(CONFIG_MQTT_LIB_TLS) */
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

void mqtt_is_connecting(void)
{
	return mqtt_connecting_flag;
}

static void mqtt_link(struct k_work_q *work_q)
{
	int err,i=0;

	LOGD("begin");

	mqtt_connecting_flag = true;
	
#ifdef CONFIG_DEVICE_POWER_MANAGEMENT
	uart_sleep_out();
#endif

	client_init(&client);

	err = mqtt_connect(&client);
	if(err != 0)
	{
		LOGD("ERROR: mqtt_connect %d", err);
		return;
	}

	err = fds_init(&client);
	if(err != 0)
	{
		LOGD("ERROR: fds_init %d", err);
		return;
	}

	while(1)
	{
		err = poll(&fds, 1, mqtt_keepalive_time_left(&client));
		if(err < 0)
		{
			LOGD("ERROR: poll %d", errno);
			break;
		}

		err = mqtt_live(&client);
		if((err != 0) && (err != -EAGAIN))
		{
			LOGD("ERROR: mqtt_live %d", err);
			break;
		}

		if((fds.revents & POLLIN) == POLLIN)
		{
			err = mqtt_input(&client);
			if(err != 0)
			{
				LOGD("ERROR: mqtt_input %d", err);
				break;
			}
		}

		if((fds.revents & POLLERR) == POLLERR)
		{
			LOGD("POLLERR");
			break;
		}

		if((fds.revents & POLLNVAL) == POLLNVAL)
		{
			LOGD("POLLNVAL");
			break;
		}
	}
	LOGD("[%s]: Disconnecting MQTT client...", __func__);

	err = mqtt_disconnect(&client);
	if(err)
	{
		LOGD("[%s]: Could not disconnect MQTT client. Error: %d", __func__, err);
	}

	mqtt_connecting_flag = false;
}

static void SendDataCallBack(struct k_timer *timer)
{
	send_data_flag = true;
}

static void NbSendDataStart(void)
{
	k_timer_start(&send_data_timer, K_MSEC(100), NULL);
}

static void NbSendDataStop(void)
{
	k_timer_stop(&send_data_timer);
}

static void NbSendData(void)
{
	u8_t *p_data;
	u32_t data_len;
	int ret;

	ret = get_data_from_send_cache(&p_data, &data_len);
	if(ret)
	{
		ret = data_publish(&client, MQTT_QOS_1_AT_LEAST_ONCE, p_data, data_len);
		if(!ret)
		{
			delete_data_from_send_cache();
		}

		k_timer_start(&send_data_timer, K_MSEC(500), NULL);
	}
}

bool MqttIsConnected(void)
{
	LOGD("mqtt_connected:%d", mqtt_connected);
	return mqtt_connected;
}

void DisConnectMqttLink(void)
{
	int err;
		
	if(k_timer_remaining_get(&mqtt_disconnect_timer) > 0)
		k_timer_stop(&mqtt_disconnect_timer);

	if(mqtt_connected)
	{
		err = mqtt_disconnect(&client);
		if(err)
		{
			LOGD("Could not disconnect MQTT client. Error: %d", err);
		}
		
		mqtt_connected = false;
	}
}

void DisconnectAppMqttLink(void)
{
	if(k_timer_remaining_get(&mqtt_disconnect_timer) > 0)
		k_timer_stop(&mqtt_disconnect_timer);

	if(mqtt_connected)
	{
		mqtt_connected = false;
		mqtt_disconnect_flag = true;
	}
}

static void MqttDisConnect(void)
{
	int err;

	LOGD("begin");
	err = mqtt_disconnect(&client);
	if(err)
	{
		LOGD("Could not disconnect MQTT client. Error: %d", err);
	}
}

static void MqttDisConnectCallBack(struct k_timer *timer_id)
{
	LOGD("begin");
	mqtt_disconnect_flag = true;
}

static void MqttDicConnectStart(void)
{
	LOGD("begin");
	k_timer_start(&mqtt_disconnect_timer, K_SECONDS(MQTT_CONNECTED_KEEP_TIME), NULL);
}

static void MqttDicConnectStop(void)
{
	k_timer_stop(&mqtt_disconnect_timer);
}


#if CONFIG_MODEM_INFO
/**@brief Callback handler for LTE RSRP data. */
static void modem_rsrp_handler(char rsrp_value)
{
	u8_t *ptr;
	bool flag=false;
	u8_t strbuf[128] = {0};
	u8_t tmpbuf[128] = {0};

	/* RSRP raw values that represent actual signal strength are
	 * 0 through 97 (per "nRF91 AT Commands" v1.1). If the received value
	 * falls outside this range, we should not send the value.
	 */
	LOGD("rsrp_value:%d", rsrp_value);

	if(test_nb_flag)
	{
		sprintf(nb_test_info, "signal-rsrp:%d (%ddBm)", rsrp_value,(rsrp_value-141));
		nb_test_update_flag = true;
	}

	if(at_cmd_write("AT+CSCON?", strbuf, sizeof(strbuf), NULL) == 0)
	{
		//+CSCON: <n>,<mode>[,<state>[,<access]]
		//<n>
		//0 – Unsolicited indications disabled
		//1 – Enabled: <mode>
		//2 – Enabled: <mode>[,<state>]
		//3 – Enabled: <mode>[,<state>[,<access>]]
		//<mode>
		//0 – Idle
		//1 – Connected
		//<state>
		//7 – E-UTRAN connected
		//<access>
		//4 – Radio access of type E-UTRAN FDD
		LOGD("%s", strbuf);
		ptr = strstr(strbuf, "+CSCON: ");
		if(ptr)
		{
			u8_t mode;
			u16_t len;

			len = strlen(strbuf);
			strbuf[len-2] = ',';
			strbuf[len-1] = 0x00;
			
			ptr += strlen("+CSCON: ");
			GetStringInforBySepa(ptr,",",2,tmpbuf);
			mode = atoi(tmpbuf);
			LOGD("mode:%d", mode);
			if(mode == 1)//connected
			{
				flag = true;	
			}
			else if(mode == 0)//idle
			{
				LOGD("reg stat:%d", g_nw_registered);
				if(g_nw_registered) 			//registered
				{
					flag = false;				//don't show no signal when ue is psm idle mode in registered, 
				}
				else							//not registered
				{
					flag = true;	
				}

			#ifdef CONFIG_DEVICE_POWER_MANAGEMENT
				if(k_timer_remaining_get(&tau_wakeup_uart_timer) > 0)
					k_timer_stop(&tau_wakeup_uart_timer);
				
				if(g_tau_time > LTE_TAU_WAKEUP_EARLY_TIME)
					k_timer_start(&tau_wakeup_uart_timer, K_SECONDS(g_tau_time-LTE_TAU_WAKEUP_EARLY_TIME), NULL);
				else if(g_tau_time > 0)
					k_timer_start(&tau_wakeup_uart_timer, K_SECONDS(g_tau_time), NULL);
			#endif
			}
		}
	}

	if(flag)
	{
		g_rsrp = rsrp_value;
		nb_redraw_sig_flag = true;
	}
}

/**brief Initialize LTE status containers. */
void modem_data_init(void)
{
	int err;

	err = modem_info_init();
	if(err)
	{
		LOGD("Modem info could not be established: %d", err);
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
	if(test_nb_flag)
	{
		strcpy(nb_test_info, "modem_configure");
		TestNBUpdateINfor();
	}

#if 1 //xb test 20200927
	if(at_cmd_write("AT%CESQ=1", NULL, 0, NULL) != 0)
	{
		LOGD("AT_CMD write fail!");
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
		LOGD("Waitng for carrier registration...");
		if(test_nb_flag)
		{
			strcpy(nb_test_info, "Waitng for carrier registration...");
			TestNBUpdateINfor();
		}
		k_sem_take(&carrier_registered, K_FOREVER);
		LOGD("Registered!");
		if(test_nb_flag)
		{
			strcpy(nb_test_info, "Registered!");
			TestNBUpdateINfor();
		}
	#else /* defined(CONFIG_LWM2M_CARRIER) */
		int err;
		LOGD("LTE Link Connecting ...");
		if(test_nb_flag)
		{
			strcpy(nb_test_info, "LTE Link Connecting ...");
			TestNBUpdateINfor();
		}
		err = lte_lc_init_and_connect();
		__ASSERT(err == 0, "LTE link could not be established.");
		LOGD("LTE Link Connected!");
		if(test_nb_flag)
		{
			strcpy(nb_test_info, "LTE Link Connected!");
			TestNBUpdateINfor();
		}
	#endif /* defined(CONFIG_LWM2M_CARRIER) */
	}
#endif /* defined(CONFIG_LTE_LINK_CONTROL) */
}

void test_nb(void)
{
	test_nb_on = true;
}

void MenuStartNB(void)
{
	test_nb_on = true;
}

void MenuStopNB(void)
{
	nb_is_running = false;
	test_nb_flag = false;

	if(k_timer_remaining_get(&get_nw_rsrp_timer) > 0)
		k_timer_stop(&get_nw_rsrp_timer);
}

void NBRedrawSignal(void)
{
<<<<<<< HEAD
	u8_t *ptr;
	bool flag=false;
	u8_t strbuf[128] = {0};
	u8_t tmpbuf[128] = {0};

	if(at_cmd_write("AT+CSCON?", strbuf, sizeof(strbuf), NULL) == 0)
	{
		//+CSCON: <n>,<mode>[,<state>[,<access]]
		//<n>
		//0 – Unsolicited indications disabled
		//1 – Enabled: <mode>
		//2 – Enabled: <mode>[,<state>]
		//3 – Enabled: <mode>[,<state>[,<access>]]
		//<mode>
		//0 – Idle
		//1 – Connected
		//<state>
		//7 – E-UTRAN connected
		//<access>
		//4 – Radio access of type E-UTRAN FDD
		LOGD("%s", strbuf);
		ptr = strstr(strbuf, "+CSCON: ");
		if(ptr)
		{
			u8_t mode;
			u16_t len;

			len = strlen(strbuf);
			strbuf[len-2] = ',';
			strbuf[len-1] = 0x00;
			
			ptr += strlen("+CSCON: ");
			GetStringInforBySepa(ptr,",",2,tmpbuf);
			mode = atoi(tmpbuf);
			LOGD("mode:%d", mode);
			if(mode == 1)//connected
			{
				flag = true;	
			}
			else if(mode == 0)//idle
			{
				LOGD("reg stat:%d", g_nw_registered);
				if(g_nw_registered) 			//registered
				{
					flag = false;				//don't show no signal when ue is psm idle mode in registered, 
				}
				else							//not registered
				{
					flag = true;	
				}

		#ifdef CONFIG_DEVICE_POWER_MANAGEMENT
				if(k_timer_remaining_get(&tau_wakeup_uart_timer) > 0)
					k_timer_stop(&tau_wakeup_uart_timer);
				
				if(g_tau_time > LTE_TAU_WAKEUP_EARLY_TIME)
					k_timer_start(&tau_wakeup_uart_timer, K_SECONDS(g_tau_time-LTE_TAU_WAKEUP_EARLY_TIME), NULL);
				else if(g_tau_time > 0)
					k_timer_start(&tau_wakeup_uart_timer, K_SECONDS(g_tau_time), NULL);
		#endif
			}
		}
	}

	if(g_rsrp > 97)
	{
		if(flag)
			g_nb_sig = NB_SIG_LEVEL_NO;
=======
	if(g_rsrp > 97)
	{
		g_nb_sig = NB_SIG_LEVEL_NO;
>>>>>>> 8bce4a84668dbda5bcff9d2c617fb194336eafb1
	}
	else if(g_rsrp >= 80)
	{
		g_nb_sig = NB_SIG_LEVEL_4;
	}
	else if(g_rsrp >= 60)
	{
		g_nb_sig = NB_SIG_LEVEL_3;
	}
	else if(g_rsrp >= 40)
	{
		g_nb_sig = NB_SIG_LEVEL_2;
	}
	else if(g_rsrp >= 20)
	{
		g_nb_sig = NB_SIG_LEVEL_1;
	}
	else
	{
		g_nb_sig = NB_SIG_LEVEL_0;
	}

	if(screen_id == SCREEN_ID_IDLE)
	{
		scr_msg[screen_id].para |= SCREEN_EVENT_UPDATE_SIG;
		scr_msg[screen_id].act = SCREEN_ACTION_UPDATE;
	}
}

void GetStringInforBySepa(u8_t *sour_buf, u8_t *key_buf, u8_t pos_index, u8_t *outbuf)
{
	u8_t *ptr1,*ptr2;
	u8_t count=0;
	
	if(sour_buf == NULL || key_buf == NULL || outbuf == NULL || pos_index < 1)
		return;

	ptr1 = sour_buf;
	while(1)
	{
		ptr2 = strstr(ptr1, key_buf);
		if(ptr2)
		{
			count++;
			if(count == pos_index)
			{
				memcpy(outbuf, ptr1, (ptr2-ptr1));
				return;
			}
			else
			{
				ptr1 = ptr2+1;
			}
		}
		else
		{
			return;
		}
	}
}

void GetModemDateTime(void)
{
	char *ptr;
	u8_t timebuf[128] = {0};
	u8_t tmpbuf[10] = {0};
	u8_t tz_dir[3] = {0};
	u8_t tz_count,daylight;

	if(at_cmd_write("AT%CCLK?", timebuf, sizeof(timebuf), NULL) != 0)
	{
		LOGD("Get CCLK fail!");
		if(nb_connected)
			k_timer_start(&get_nw_time_timer, K_MSEC(1000), NULL);
		
		return;
	}
	LOGD("%s", timebuf);
	//%CCLK: "21/02/26,08:31:33+32",0
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

		ptr+=3;
		ptr = strstr(ptr, ",");
		if(ptr)
		{
			ptr+=1;
			memset(tmpbuf, 0, sizeof(tmpbuf));
			memcpy(tmpbuf, ptr, 1);
			daylight = atoi(tmpbuf);
			TimeDecrease(&date_time, daylight*60);
		}

		strcpy(g_timezone, tz_dir);
		sprintf(tmpbuf, "%d", tz_count/4);
		strcat(g_timezone, tmpbuf);
	}
	LOGD("real time:%04d/%02d/%02d,%02d:%02d:%02d,%02d", 
					date_time.year,date_time.month,date_time.day,
					date_time.hour,date_time.minute,date_time.second,
					date_time.week);

	RedrawSystemTime();
	SaveSystemDateTime();
}

void GetNetWorkTimeCallBack(struct k_timer *timer_id)
{
	get_modem_time_flag = true;
}

static void MqttSendData(u8_t *data, u32_t datalen)
{
	int ret;

	ret = add_data_into_send_cache(data, datalen);
	LOGD("data add ret:%d", ret);
	if(mqtt_connected)
	{
		LOGD("begin 001");
		NbSendDataStart();
	}
	else
	{
		LOGD("begin 002");
		if(nb_connected)
		{
			LOGD("begin 003");
			if(!mqtt_connecting_flag)
			{
				LOGD("begin 003.1");
				k_delayed_work_submit_to_queue(app_work_q, &mqtt_link_work, K_NO_WAIT);
			}
			else
			{
				LOGD("begin 003.2");
			}
		}
		else
		{
			LOGD("begin 004", __func__);
			if(k_timer_remaining_get(&nb_reconnect_timer) > 0)
				k_timer_stop(&nb_reconnect_timer);

			k_timer_start(&nb_reconnect_timer, K_SECONDS(10), NULL);
		}
	}
}

void NBSendSettingReply(u8_t *data, u32_t datalen)
{
	u8_t buf[256] = {0};
	u8_t tmpbuf[32] = {0};

	strcpy(buf, "{1:1:0:0:");
	strcat(buf, g_imei);
	strcat(buf, ":");
	strcat(buf, data);
	strcat(buf, ":");
	GetSystemTimeSecString(tmpbuf);
	strcat(buf, tmpbuf);	
	strcat(buf, "}");

	LOGD("eply data:%s", buf);

	MqttSendData(buf, strlen(buf));
}

void NBSendSosWifiData(u8_t *data, u32_t datalen)
{
	u8_t buf[256] = {0};
	
	strcpy(buf, "{1:1:0:0:");
	strcat(buf, g_imei);
	strcat(buf, ":T1:");
	strcat(buf, data);
	strcat(buf, ",");
	strcat(buf, sos_trigger_time);
	strcat(buf, "}");
	
	LOGD("sos wifi data:%s", buf);
	MqttSendData(buf, strlen(buf));
}

void NBSendSosGpsData(u8_t *data, u32_t datalen)
{
	u8_t buf[256] = {0};
	u8_t tmpbuf[32] = {0};
	
	strcpy(buf, "{T2,");
	strcat(buf, g_imei);
	strcat(buf, ",[");
	strcat(buf, data);
	GetSystemTimeSecString(tmpbuf);
	strcat(buf, tmpbuf);
	strcat(buf, "]}");
	
	LOGD("sos gps data:%s", buf);

	MqttSendData(buf, strlen(buf));
}

#ifdef CONFIG_IMU_SUPPORT
void NBSendFallWifiData(u8_t *data, u32_t datalen)
{
	u8_t buf[256] = {0};
	
	strcpy(buf, "{1:1:0:0:");
	strcat(buf, g_imei);
	strcat(buf, ":T3:");
	strcat(buf, data);
	strcat(buf, ",");
	strcat(buf, fall_trigger_time);
	strcat(buf, "}");
	
	LOGD("fall wifi data:%s", buf);
	MqttSendData(buf, strlen(buf));
}

void NBSendFallGpsData(u8_t *data, u32_t datalen)
{
	u8_t buf[256] = {0};
	u8_t tmpbuf[32] = {0};
	
	strcpy(buf, "{T4,");
	strcat(buf, g_imei);
	strcat(buf, ",[");
	strcat(buf, data);
	GetSystemTimeSecString(tmpbuf);
	strcat(buf, tmpbuf);
	strcat(buf, "]}");

	LOGD("fall gps data:%s", buf);
	MqttSendData(buf, strlen(buf));
}
#endif

void NBSendHealthData(u8_t *data, u32_t datalen)
{
	u8_t buf[256] = {0};
	u8_t tmpbuf[32] = {0};
	
	strcpy(buf, "{1:1:0:0:");
	strcat(buf, g_imei);
	strcat(buf, ":T5:");
	strcat(buf, data);
	strcat(buf, ",");
	GetBatterySocString(tmpbuf);
	strcat(buf, tmpbuf);
	strcat(buf, ",");
	memset(tmpbuf, 0, sizeof(tmpbuf));
	GetSystemTimeSecString(tmpbuf);
	strcat(buf, tmpbuf);
	strcat(buf, "}");

	LOGD("health data:%s", buf);
	MqttSendData(buf, strlen(buf));
}

void NBSendLocationData(u8_t *data, u32_t datalen)
{
	u8_t buf[256] = {0};
	u8_t tmpbuf[32] = {0};
	
	strcpy(buf, "{1:1:0:0:");
	strcat(buf, g_imei);
	strcat(buf, ":T6:");
	strcat(buf, data);
	strcat(buf, ",");
	memset(tmpbuf, 0, sizeof(tmpbuf));
	GetSystemTimeSecString(tmpbuf);
	strcat(buf, tmpbuf);
	strcat(buf, "}");

	LOGD("location data:%s", buf);
	MqttSendData(buf, strlen(buf));
}

void NBSendPowerOnInfor(u8_t *data, u32_t datalen)
{
	u8_t buf[256] = {0};
	u8_t tmpbuf[20] = {0};
	
	strcpy(buf, "{1:1:0:0:");
	strcat(buf, g_imei);
	strcat(buf, ":T12:");
	strcat(buf, data);
	strcat(buf, ",");
	memset(tmpbuf, 0, sizeof(tmpbuf));
	GetSystemTimeSecString(tmpbuf);
	strcat(buf, tmpbuf);
	strcat(buf, "}");

	LOGD("pwr on infor:%s", buf);
	MqttSendData(buf, strlen(buf));
}

void NBSendPowerOffInfor(u8_t *data, u32_t datalen)
{
	u8_t buf[256] = {0};
	u8_t tmpbuf[20] = {0};
	
	strcpy(buf, "{1:1:0:0:");
	strcat(buf, g_imei);
	strcat(buf, ":T13:");
	strcat(buf, data);
	strcat(buf, ",");
	memset(tmpbuf, 0, sizeof(tmpbuf));
	GetSystemTimeSecString(tmpbuf);
	strcat(buf, tmpbuf);
	strcat(buf, "}");

	LOGD("pwr off infor:%s", buf);
	MqttSendData(buf, strlen(buf));
}

bool GetParaFromString(u8_t *rece, u32_t rece_len, u8_t *cmd, u8_t *data)
{
	u8_t *p1,*p2;
	u32_t i=0;
	
	if(rece == NULL || cmd == NULL || rece_len == 0)
		return false;

	p1 = rece;
	while((p1 != NULL) && ((p1-rece) < rece_len))
	{
		p2 = strstr(p1, ":");
		if(p2 != NULL)
		{
			i++;
			p1 = p2 + 1;
			if(i == 5)
				break;
		}
		else
		{
			return false;
		}
	}

	p2 = strstr(p1, ":");
	if(p2 != NULL)
	{
		memcpy(cmd, p1, (p2-p1));

		p1 = p2 + 1;
		p2 = strstr(p1, "}");
		if(p2 != NULL)
		{
			memcpy(data, p1, (p2-p1));
			return true;
		}
		else
		{
			return false;
		}
	}
	else
	{
		return false;
	}
}

void ParseData(u8_t *data, u32_t datalen)
{
	bool ret = false;
	u8_t strcmd[10] = {0};
	u8_t strdata[256] = {0};
	
	if(data == NULL || datalen == 0)
		return;

	ret = GetParaFromString(data, datalen, strcmd, strdata);
	LOGD("ret:%d, cmd:%s, data:%s", ret,strcmd,strdata);
	if(ret)
	{
		if(strcmp(strcmd, "S7") == 0)
		{
			u8_t *ptr;
			u8_t strtmp[128] = {0};

			//后台下发定位上报间隔
			ptr = strstr(strdata, ",");
			if(ptr != NULL)
			{
				memcpy(strtmp, strdata, (ptr-strdata));
				global_settings.dot_interval.time = atoi(strtmp);

				ptr++;
				memset(strtmp, 0, sizeof(strtmp));
				strcpy(strtmp, ptr);
				global_settings.dot_interval.steps = atoi(strtmp);
			}
		}
		else if(strcmp(strcmd, "S8") == 0)
		{
			//后台下发健康检测间隔
			global_settings.health_interval = atoi(strdata);
			
		}
		else if(strcmp(strcmd, "S9") == 0)
		{
			//后台下发抬腕亮屏设置
			global_settings.wake_screen_by_wrist = atoi(strdata);
			
		}
		else if(strcmp(strcmd, "S10") == 0)
		{
			//后台下发脱腕检测设置
			global_settings.wrist_off_check = atoi(strdata);
			
		}
		else if(strcmp(strcmd, "S11") == 0)
		{
			u8_t *ptr;
			u8_t strtmp[128] = {0};
			
			//后台下发血压校准值
			ptr = strstr(strdata, ",");
			if(ptr != NULL)
			{
				memcpy(strtmp, strdata, (ptr-strdata));
				global_settings.bp_calibra.systolic = atoi(strtmp);

				ptr++;
				memset(strtmp, 0, sizeof(strtmp));
				strcpy(strtmp, ptr);
				global_settings.bp_calibra.diastolic = atoi(strtmp);
			}
		}

		SaveSystemSettings();

		strcmd[0] = 'T';
		NBSendSettingReply(strcmd, strlen(strcmd));
	}
}

static void ParseDataCallBack(struct k_timer *timer_id)
{
	parse_data_flag = true;
}

static void ParseReceData(void)
{
	u8_t *p_data;
	u32_t data_len;
	int ret;

	ret = get_data_from_rece_cache(&p_data, &data_len);
	if(ret)
	{
		ParseData(p_data, data_len);
		delete_data_from_rece_cache();
		
		k_timer_start(&parse_data_timer, K_MSEC(100), NULL);
	}
}

static void ParseReceDataStart(void)
{
	k_timer_start(&parse_data_timer, K_MSEC(500), NULL);
}

static void MqttReceData(u8_t *data, u32_t datalen)
{
	int ret;

	ret = add_data_into_rece_cache(data, datalen);
	LOGD("data add ret:%d", ret);
	ParseReceDataStart();
}

static int configure_low_power(void)
{
	int err;

#if defined(CONFIG_LTE_PSM_ENABLE)
	/** Power Saving Mode */
	err = lte_lc_psm_req(true);
	if(err)
	{
		LOGD("lte_lc_psm_req, error: %d", err);
	}
#else
	err = lte_lc_psm_req(false);
	if(err)
	{
		LOGD("lte_lc_psm_req, error: %d", err);
	}
#endif

#if defined(CONFIG_LTE_EDRX_ENABLE)
	/** enhanced Discontinuous Reception */
	err = lte_lc_edrx_req(true);
	if(err)
	{
		LOGD("lte_lc_edrx_req, error: %d", err);
	}
#else
	err = lte_lc_edrx_req(false);
	if(err)
	{
		LOGD("lte_lc_edrx_req, error: %d", err);
	}
#endif

#if 0//defined(CONFIG_LTE_RAI_ENABLE)
	/** Release Assistance Indication  */
	err = at_cmd_write(CMD_SET_RAI, NULL, 0, NULL);
	if(err)
	{
		LOGD("lte_lc_rai_req, error: %d", err);
	}
#endif

	return err;
}

void GetModemSignal(void)
{
	char *ptr;
	int i=0,len;
	u8_t strbuf[64] = {0};
	u8_t tmpbuf[64] = {0};
	s32_t rsrp;
	static s32_t rsrpbk = 0;
	
	if(at_cmd_write(CMD_GET_RSRP, tmpbuf, sizeof(tmpbuf), NULL) != 0)
	{
		LOGD("Get rsrp fail!");
		return;
	}

	LOGD("rsrp:%s", tmpbuf);
	len = strlen(tmpbuf);
	ptr = tmpbuf;
	while(i<5)
	{
		ptr = strstr(ptr, ",");
		ptr++;
		i++;
	}

	memcpy((char*)strbuf, ptr, len-(ptr-(char*)tmpbuf));
	rsrp = atoi(strbuf);
	if(rsrp != rsrpbk)
	{
		rsrpbk = rsrp;
		if(test_nb_flag)
		{
			sprintf(nb_test_info, "signal-rsrp:%d (%ddBm)", rsrp,(rsrp-141));
			nb_test_update_flag = true;
		}
		else
		{
			modem_rsrp_handler(rsrp);
		}
	}
}

#ifdef CONFIG_DEVICE_POWER_MANAGEMENT
void TauWakeUpUartCallBack(struct k_timer *timer_id)
{
	uart_wake_flag = true;
	get_modem_signal_flag = true;
}
#endif

void GetNetWorkSignalCallBack(struct k_timer *timer_id)
{
	get_modem_signal_flag = true;
}

void DecodeModemMonitor(u8_t *buf, u32_t len)
{
	u8_t reg_status = 0;
	u8_t *ptr;
	u8_t tmpbuf[128] = {0};
	u8_t strbuf[128] = {0};

	if(buf == NULL || len == 0)
		return;
	
	LOGD("%s", buf);

	//%XMONITOR: 1,"","","46000","1D29",9,8,"0D1E2D42",11,3684,45,38,"","00000100","11100000","00111110"
	ptr = strstr(buf, "%XMONITOR: ");
	if(ptr)
	{
		//补上一个逗号作为末尾的分隔符,替换以前的0x0d,0x0a
		buf[len-2] = ',';
		buf[len-1] = 0x00;
		//指令头
		ptr += strlen("%XMONITOR: ");
		//reg_status
		GetStringInforBySepa(ptr, ",", 1, tmpbuf);
		reg_status = atoi(tmpbuf);
		//0 – Not registered. UE is not currently searching for an operator to register to.
		//1 – Registered, home network.
		//2 – Not registered, but UE is currently trying to attach or searching an operator to register to.
		//3 – Registration denied.
		//4 – Unknown (e.g. out of E-UTRAN coverage).
		//5 – Registered, roaming.
		//8 – Attached for emergency bearer services only.
		//90 – Not registered due to UICC failure.
		if(reg_status == 1 || reg_status == 5)
		{
			u8_t tau = 0;
			u8_t act = 0;
			u16_t flag;
			
			g_nw_registered = true;

		#if 0
			//full name
			memset(tmpbuf, 0, sizeof(tmpbuf));
			GetStringInforBySepa(ptr, ",", 2, tmpbuf);
			LOGD("full name:%s", tmpbuf);
			//short name
			memset(tmpbuf, 0, sizeof(tmpbuf));
			GetStringInforBySepa(ptr, ",", 3, tmpbuf);
			LOGD("short name:%s", tmpbuf);
			//plmn
			memset(tmpbuf, 0, sizeof(tmpbuf));
			GetStringInforBySepa(ptr, ",", 4, tmpbuf);
			LOGD("plmn:%s", tmpbuf);
			//tac
			memset(tmpbuf, 0, sizeof(tmpbuf));
			GetStringInforBySepa(ptr, ",", 5, tmpbuf);
			LOGD("tac:%s", tmpbuf);
			//AcT
			memset(tmpbuf, 0, sizeof(tmpbuf));
			GetStringInforBySepa(ptr, ",", 6, tmpbuf);
			LOGD("AcT:%s", tmpbuf);
			//band
			memset(tmpbuf, 0, sizeof(tmpbuf));
			GetStringInforBySepa(ptr, ",", 7, tmpbuf);
			LOGD("band:%s",  tmpbuf);
			//cell_id
			memset(tmpbuf, 0, sizeof(tmpbuf));
			GetStringInforBySepa(ptr, ",", 8, tmpbuf);
			LOGD("cell_id:%s", tmpbuf);
			//phys_cell_id
			memset(tmpbuf, 0, sizeof(tmpbuf));
			GetStringInforBySepa(ptr, ",", 9, tmpbuf);
			LOGD("phys_cell_id:%s", tmpbuf);
			//EARFCN
			memset(tmpbuf, 0, sizeof(tmpbuf));
			GetStringInforBySepa(ptr, ",", 10, tmpbuf);
			LOGD("EARFCN:%s", tmpbuf);
		#endif	
			//rsrp
			memset(tmpbuf, 0, sizeof(tmpbuf));
			GetStringInforBySepa(ptr, ",", 11, tmpbuf);
			LOGD("rsrp:%s", tmpbuf);
			g_rsrp = atoi(tmpbuf);
			modem_rsrp_handler(g_rsrp);
		#if 0	
			//snr
			memset(tmpbuf, 0, sizeof(tmpbuf));
			GetStringInforBySepa(ptr, ",", 12, tmpbuf);
			LOGD("snr:%s", tmpbuf);
			//NW-provided_eDRX_value
			memset(tmpbuf, 0, sizeof(tmpbuf));
			GetStringInforBySepa(ptr, ",", 13, tmpbuf);
			LOGD("NW-provided_eDRX_value:%s", tmpbuf);
		#endif
			//Active-Time
			memset(tmpbuf, 0, sizeof(tmpbuf));
			GetStringInforBySepa(ptr, ",", 14, tmpbuf);
			LOGD("Active-Time:%s", tmpbuf);
			memcpy(strbuf, &tmpbuf[1], 8);
			act = (strbuf[0]-0x30)*0x80+(strbuf[1]-0x30)*0x40+(strbuf[2]-0x30)*0x20;
			switch(act>>5)
			{
			case 0://0 0 0 – value is incremented in multiples of 2 seconds
				flag = 2;
				break;
			case 1://0 0 1 – value is incremented in multiples of 1 minute
				flag = 60;
				break;
			case 2://0 1 0 – value is incremented in multiples of 6 minutes
				flag = 6*60;
				break;
			case 3://0 1 1 – value indicates that the timer is deactivated
				flag = 0;
				break;
			}
			g_act_time = flag*((strbuf[3]-0x30)*0x10+(strbuf[4]-0x30)*0x08+(strbuf[5]-0x30)*0x04+(strbuf[6]-0x30)*0x02+(strbuf[7]-0x30)*0x01);
			LOGD("g_act_time:%d", g_act_time);

			//Periodic-TAU-ext
			memset(tmpbuf, 0, sizeof(tmpbuf));
			GetStringInforBySepa(ptr, ",", 15, tmpbuf);
			LOGD("Periodic-TAU-ext:%s", tmpbuf);
			//Periodic-TAU
			memset(tmpbuf, 0, sizeof(tmpbuf));
			GetStringInforBySepa(ptr, ",", 16, tmpbuf);
			LOGD("Periodic-TAU:%s", tmpbuf);
			memset(strbuf,0,sizeof(strbuf));
			memcpy(strbuf, &tmpbuf[1], 8);
			tau = (strbuf[0]-0x30)*0x80+(strbuf[1]-0x30)*0x40+(strbuf[2]-0x30)*0x20;
			switch(tau>>5)
			{
			case 0://0 0 0 – value is incremented in multiples of 2 seconds
				flag = 2;
				break;
			case 1://0 0 1 – value is incremented in multiples of 1 minute
				flag = 60;
				break;
			case 2://0 1 0 – value is incremented in multiples of 6 minute
				flag = 6*60;
				break;
			case 7://1 1 1 – value indicates that the timer is deactivated
				flag = 0;
				break;
			}
			g_tau_time = flag*((strbuf[3]-0x30)*0x10+(strbuf[4]-0x30)*0x08+(strbuf[5]-0x30)*0x04+(strbuf[6]-0x30)*0x02+(strbuf[7]-0x30)*0x01);
			LOGD("g_tau_time:%d", g_tau_time);
		}
	}
}

void GetModemInfor(void)
{
	u8_t tmpbuf[256] = {0};
	
	if(at_cmd_write(CMD_GET_IMEI, tmpbuf, sizeof(tmpbuf), NULL) == 0)
	{
		LOGD("imei:%s", tmpbuf);

		strncpy(g_imei, tmpbuf, IMEI_MAX_LEN);
	}

	if(at_cmd_write(CMD_GET_IMSI, tmpbuf, sizeof(tmpbuf), NULL) == 0)
	{
		LOGD("imsi:%s", tmpbuf);

		strncpy(g_imsi, tmpbuf, IMSI_MAX_LEN);
		
		if(strstr(g_imsi, "90128"))
		{
			if(at_cmd_write("AT+CGDCONT=0,\"IP\",\"arkessalp.com\"", tmpbuf, sizeof(tmpbuf), NULL) != 0)
			{
				LOGD("set apn fail!");
			}
		}

		if(at_cmd_write(CMD_GET_APN, tmpbuf, sizeof(tmpbuf), NULL) == 0)
		{
			LOGD("apn:%s", tmpbuf);	
		}
	}

	if(at_cmd_write(CMD_GET_ICCID, tmpbuf, sizeof(tmpbuf), NULL) == 0)
	{
		LOGD("iccid:%s", &tmpbuf[9]);

		strncpy(g_iccid, &tmpbuf[9], ICCID_MAX_LEN);
	}

	if(at_cmd_write(CMD_GET_MODEM_V, tmpbuf, sizeof(tmpbuf), NULL) == 0)
	{
		LOGD("MODEM version:%s", &tmpbuf);

		strncpy(g_modem, &tmpbuf, MODEM_MAX_LEN);
	}
}

void GetModemStatus(void)
{
	char *ptr;
	int i=0,len,err;
	u8_t strbuf[64] = {0};
	u8_t tmpbuf[256] = {0};

	if(at_cmd_write(CMD_GET_MODEM_PARA, tmpbuf, sizeof(tmpbuf), NULL) == 0)
	{
		LOGD("MODEM parameter:%s", &tmpbuf);

		DecodeModemMonitor(tmpbuf, strlen(tmpbuf));
	}

#if 1
	if(at_cmd_write(CMD_GET_RSRP, tmpbuf, sizeof(tmpbuf), NULL) == 0)
	{
		LOGD("rsrp:%s", tmpbuf);
		
		len = strlen(tmpbuf);
		tmpbuf[len-2] = ',';
		tmpbuf[len-1] = 0x00;
		
		ptr = strstr(tmpbuf, "+CESQ: ");
		ptr += strlen("+CESQ: ");
		GetStringInforBySepa(ptr, ",", 6, strbuf);
		g_rsrp = atoi(strbuf);
		modem_rsrp_handler(g_rsrp);
	}
#endif	
}

void SetModemTurnOn(void)
{
	if(at_cmd_write("AT+CFUN=1", NULL, 0, NULL) == 0)
		LOGD("turn on modem success!");
	else
		LOGD("Can't turn on modem!");
}

void SetModemTurnOff(void)
{
	if(at_cmd_write("AT+CFUN=4", NULL, 0, NULL) == 0)
		LOGD("turn off modem success!");
	else
		LOGD("Can't turn off modem!");
}

<<<<<<< HEAD
	nb_connected = false;
=======
void GetModemAPN(void)
{
	u8_t tmpbuf[128] = {0};
	
	if(at_cmd_write(CMD_GET_APN, tmpbuf, sizeof(tmpbuf), NULL) == 0)
	{
		LOGD("apn:%s", tmpbuf);	
	}
>>>>>>> 8bce4a84668dbda5bcff9d2c617fb194336eafb1
}

void GetModemAPN(void)
{
	u8_t tmpbuf[128] = {0};
	
	if(at_cmd_write(CMD_GET_APN, tmpbuf, sizeof(tmpbuf), NULL) == 0)
	{
		LOGD("apn:%s", tmpbuf);	
	}
}

void SetModemAPN(void)
{
	u8_t tmpbuf[128] = {0};
	
	if(at_cmd_write("AT+CGDCONT=0,\"IP\",\"arkessalp.com\"", tmpbuf, sizeof(tmpbuf), NULL) != 0)
	{
		LOGD("set apn fail!");
	}

	if(at_cmd_write(CMD_GET_APN, tmpbuf, sizeof(tmpbuf), NULL) != 0)
	{
		LOGD("Get apn fail!");
		return;
	}
	LOGD("apn:%s", tmpbuf);	
}


static void NBReconnectCallBack(struct k_timer *timer_id)
{
	nb_reconnect_flag = true;
}

static void nb_link(struct k_work *work)
{
	int err=0;
	u8_t tmpbuf[128] = {0};
	static u32_t retry_count = 0;
	static bool frist_flag = false;

	if(!frist_flag)
	{
		frist_flag = true;
		
		SetModemTurnOn();
		GetModemInfor();
		SetModemTurnOff();
		err = lte_lc_init();
		LOGD("lte_lc_init err:%d", err);
	}

	if(gps_is_working())
	{
		LOGD("gps is working, continue waiting!");

		if(retry_count <= 2)		//2次以内每1分钟重连一次
			k_timer_start(&nb_reconnect_timer, K_SECONDS(60), NULL);
		else if(retry_count <= 4)	//3到4次每5分钟重连一次
			k_timer_start(&nb_reconnect_timer, K_SECONDS(300), NULL);
		else if(retry_count <= 6)	//5到6次每10分钟重连一次
			k_timer_start(&nb_reconnect_timer, K_SECONDS(600), NULL);
		else if(retry_count <= 8)	//7到8次每1小时重连一次
			k_timer_start(&nb_reconnect_timer, K_SECONDS(3600), NULL);
		else						//8次以上每6小时重连一次
			k_timer_start(&nb_reconnect_timer, K_SECONDS(6*3600), NULL);	
	}
	else
	{
		LOGD("linking");
		nb_connecting_flag = true;
	#ifdef CONFIG_DEVICE_POWER_MANAGEMENT
		uart_sleep_out();
	#endif
		configure_low_power();

		err = lte_lc_connect();
		if(err)
		{
			LOGD("Can't connected to LTE network. err:%d", err);
			nb_connected = false;

			retry_count++;
			if(retry_count <= 2)		//2次以内每1分钟重连一次
				k_timer_start(&nb_reconnect_timer, K_SECONDS(60), NULL);
			else if(retry_count <= 4)	//3到4次每5分钟重连一次
				k_timer_start(&nb_reconnect_timer, K_SECONDS(300), NULL);
			else if(retry_count <= 6)	//5到6次每10分钟重连一次
				k_timer_start(&nb_reconnect_timer, K_SECONDS(600), NULL);
			else if(retry_count <= 8)	//7到8次每1小时重连一次
				k_timer_start(&nb_reconnect_timer, K_SECONDS(3600), NULL);
			else						//8次以上每6小时重连一次
				k_timer_start(&nb_reconnect_timer, K_SECONDS(6*3600), NULL);
		}
		else
		{
			LOGD("Connected to LTE network");

			nb_connected = true;
			retry_count = 0;

			GetModemDateTime();
		#ifndef CONFIG_DEVICE_POWER_MANAGEMENT	
			modem_data_init();
		#endif
		}

		GetModemStatus();

		if(!nb_connected)
			SetModemTurnOff();
		
		if(!err && !test_nb_flag)
		{
			k_delayed_work_submit_to_queue(app_work_q, &mqtt_link_work, K_SECONDS(2));
		}

		nb_connecting_flag = false;
	}
}

void GetNBSignal(void)
{
	u8_t str_rsrp[128] = {0};

	if(at_cmd_write("AT+CFUN?", str_rsrp, sizeof(str_rsrp), NULL) != 0)
	{
		LOGD("Get cfun fail!");
		return;
	}
	LOGD("cfun:%s", str_rsrp);

	if(at_cmd_write("AT+CPSMS?", str_rsrp, sizeof(str_rsrp), NULL) != 0)
	{
		LOGD("Get cpsms fail!");
		return;
	}
	LOGD("cpsms:%s", str_rsrp);

	if(at_cmd_write(CMD_GET_RSRP, str_rsrp, sizeof(str_rsrp), NULL) != 0)
	{
		LOGD("Get rsrp fail!");
		return;
	}
	LOGD("rsrp:%s", str_rsrp);

	if(at_cmd_write(CMD_GET_APN, str_rsrp, sizeof(str_rsrp), NULL) != 0)
	{
	#ifdef NB_DEBUG
		LOGD("Get apn fail!");
	#endif
		return;
	}
	LOGD("apn:%s", str_rsrp);

	if(at_cmd_write(CMD_GET_CSQ, str_rsrp, sizeof(str_rsrp), NULL) != 0)
	{
		LOGD("Get csq fail!");
		return;
	}
	LOGD("csq:%s", str_rsrp);
}

bool nb_is_connecting(void)
{
	LOGD("nb_connecting_flag:%d", nb_connecting_flag);
	return nb_connecting_flag;
}

bool nb_is_connected(void)
{
	return nb_connected;
}

void nb_test_update(void)
{
	if(screen_id == SCREEN_ID_NB_TEST)
		scr_msg[screen_id].act = SCREEN_ACTION_UPDATE;
}

void NBMsgProcess(void)
{
	if(test_nb_on)
	{
		test_nb_on = false;
		if(nb_is_running)
			return;

		nb_is_running = true;
		test_nb_flag = true;

		LOGD("Start NB-IoT test!");
	
		if(nb_is_connected())
		{
			sprintf(nb_test_info, "signal-rsrp:%d (%ddBm)", g_rsrp,(g_rsrp-141));
			TestNBUpdateINfor();
		}
		else if(nb_is_connecting())
		{
			strcpy(nb_test_info, "LTE Link Connecting ...");
			TestNBUpdateINfor();
		}
		else
		{
		#ifdef CONFIG_DEVICE_POWER_MANAGEMENT
			uart_sleep_out();
		#endif
			SetModemTurnOff();
			modem_configure();
			k_timer_start(&get_nw_rsrp_timer, K_MSEC(1000), K_MSEC(1000));
		}
	}

	if(get_modem_info_flag)
	{	
	#ifdef CONFIG_DEVICE_POWER_MANAGEMENT
		uart_sleep_out();
	#endif
		GetModemInfor();
		get_modem_info_flag = false;
	}

	if(nb_redraw_sig_flag)
	{
		NBRedrawSignal();
		nb_redraw_sig_flag = false;
	}

	if(get_modem_signal_flag)
	{
	#ifdef CONFIG_DEVICE_POWER_MANAGEMENT
		uart_sleep_out();
	#endif
		GetModemSignal();
		get_modem_signal_flag = false;
	}

	if(get_modem_time_flag)
	{	
	#ifdef CONFIG_DEVICE_POWER_MANAGEMENT
		uart_sleep_out();
	#endif
		GetModemDateTime();
		get_modem_time_flag = false;
	}
	
	if(send_data_flag)
	{
		NbSendData();
		send_data_flag = false;
	}

	if(parse_data_flag)
	{
		ParseReceData();
		parse_data_flag = false;
	}
	
	if(mqtt_disconnect_flag)
	{
		MqttDisConnect();
		mqtt_disconnect_flag = false;
	}

	if(nb_test_update_flag)
	{
		nb_test_update_flag = false;
		nb_test_update();
	}

	if(nb_reconnect_flag)
	{
		nb_reconnect_flag = false;
		if(test_gps_flag)
			return;
		
		k_delayed_work_submit_to_queue(app_work_q, &nb_link_work, K_SECONDS(2));
	}
	
	if(nb_connecting_flag)
	{
		k_sleep(K_MSEC(50));
	}
}

void NB_init(struct k_work_q *work_q)
{
	int err;

	app_work_q = work_q;

	k_delayed_work_init(&nb_link_work, nb_link);
	k_delayed_work_init(&mqtt_link_work, mqtt_link);
#ifdef CONFIG_FOTA_DOWNLOAD
	fota_work_init(work_q);
#endif

	k_delayed_work_submit_to_queue(app_work_q, &nb_link_work, K_SECONDS(5));
}
