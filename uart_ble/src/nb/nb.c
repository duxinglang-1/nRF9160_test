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
#include "lsm6dso.h"
#include "transfer_cache.h"

#include <logging/log_ctrl.h>
#include <logging/log.h>
LOG_MODULE_REGISTER(nb, CONFIG_LOG_DEFAULT_LEVEL);

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

bool app_nb_on = false;
bool nb_is_running = false;
bool get_modem_info_flag = false;
bool get_modem_time_flag = false;
bool get_modem_signal_flag = false;
bool get_modem_cclk_flag = false;
bool test_nb_flag = false;
bool nb_test_update_flag = false;

u8_t g_imsi[IMSI_MAX_LEN+1] = {0};
u8_t g_imei[IMEI_MAX_LEN+1] = {0};

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
		LOG_INF("LWM2M_CARRIER_EVENT_BSDLIB_INIT\n");
		if(test_nb_flag)
		{
			strcpy(nb_test_info, "LWM2M_CARRIER_EVENT_BSDLIB_INIT");
			TestNBUpdateINfor();
		}
		break;
	case LWM2M_CARRIER_EVENT_CONNECT:
		LOG_INF("LWM2M_CARRIER_EVENT_CONNECT\n");
		if(test_nb_flag)
		{
			strcpy(nb_test_info, "LWM2M_CARRIER_EVENT_CONNECT");
			TestNBUpdateINfor();
		}
		break;
	case LWM2M_CARRIER_EVENT_DISCONNECT:
		LOG_INF("LWM2M_CARRIER_EVENT_DISCONNECT\n");
		if(test_nb_flag)
		{
			strcpy(nb_test_info, "LWM2M_CARRIER_EVENT_DISCONNECT");
			TestNBUpdateINfor();
		}
		break;
	case LWM2M_CARRIER_EVENT_READY:
		LOG_INF("LWM2M_CARRIER_EVENT_READY\n");
		if(test_nb_flag)
		{
			strcpy(nb_test_info, "LWM2M_CARRIER_EVENT_READY");
			TestNBUpdateINfor();
		}
		k_sem_give(&carrier_registered);
		break;
	case LWM2M_CARRIER_EVENT_FOTA_START:
		LOG_INF("LWM2M_CARRIER_EVENT_FOTA_START\n");
		if(test_nb_flag)
		{
			strcpy(nb_test_info, "LWM2M_CARRIER_EVENT_FOTA_START");
			TestNBUpdateINfor();
		}
		break;
	case LWM2M_CARRIER_EVENT_REBOOT:
		LOG_INF("LWM2M_CARRIER_EVENT_REBOOT\n");
		if(test_nb_flag)
		{
			strcpy(nb_test_info, "LWM2M_CARRIER_EVENT_REBOOT");
			TestNBUpdateINfor();
		}
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

	//data_print("Publishing: ", data, len);
	//LOG_INF("to topic: %s len: %u\n",
	//	CONFIG_MQTT_PUB_TOPIC,
	//	(unsigned int)strlen(CONFIG_MQTT_PUB_TOPIC));

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
		.message_id = 1234
	};

	LOG_INF("Subscribing to:%s, len:%d\n", subscribe_topic.topic.utf8,
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

			LOG_INF("mqtt_read_publish_payload: EAGAIN\n");
		
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
			LOG_INF("MQTT connect failed %d\n", evt->result);
			if(test_nb_flag)
			{
				sprintf(nb_test_info, "MQTT connect failed %d", evt->result);
				TestNBUpdateINfor();
			}
			
			break;
		}

		mqtt_connected = true;
		LOG_INF("[%s:%d] MQTT client connected!\n", __func__, __LINE__);
		if(test_nb_flag)
		{
			sprintf(nb_test_info, "MQTT connect failed %d", evt->result);
			TestNBUpdateINfor();
		}
		
		subscribe();

		NbSendDataStart();
		MqttDicConnectStart();
		break;

	case MQTT_EVT_DISCONNECT:
		LOG_INF("[%s:%d] MQTT client disconnected %d\n", __func__, __LINE__, evt->result);
		if(test_nb_flag)
		{
			sprintf(nb_test_info, "MQTT client disconnected %d", evt->result);
			TestNBUpdateINfor();
		}
		
		mqtt_connected = false;
		
		NbSendDataStop();
		MqttDicConnectStop();
		break;

	case MQTT_EVT_PUBLISH: 
		{
			const struct mqtt_publish_param *p = &evt->param.publish;

			LOG_INF("[%s:%d] MQTT PUBLISH result=%d len=%d\n", __func__, __LINE__, evt->result, p->message.payload.len);
			if(test_nb_flag)
			{
				sprintf(nb_test_info, "MQTT PUBLISH result:%d len:%d\n", evt->result, p->message.payload.len);
				TestNBUpdateINfor();
			}
			err = publish_get_payload(c, p->message.payload.len);
			if(err >= 0)
			{
				data_print("Received: ", payload_buf,
					p->message.payload.len);
				/* Echo back received data */
				//data_publish(&client, MQTT_QOS_1_AT_LEAST_ONCE,
				//	payload_buf, p->message.payload.len);

				MqttReceData(payload_buf, p->message.payload.len);
			}
			else
			{
				LOG_INF("mqtt_read_publish_payload: Failed! %d\n", err);
				LOG_INF("Disconnecting MQTT client...\n");
				if(test_nb_flag)
				{
					strcpy(nb_test_info, "Disconnecting MQTT client...");
					TestNBUpdateINfor();
				}
				err = mqtt_disconnect(c);
				if(err)
				{
					LOG_INF("Could not disconnect: %d\n", err);
					if(test_nb_flag)
					{
						sprintf(nb_test_info, "Could not disconnect:%d", err);
						TestNBUpdateINfor();
					}
				}
			}
		} 
		break;

	case MQTT_EVT_PUBACK:
		if(evt->result != 0)
		{
			LOG_INF("MQTT PUBACK error %d\n", evt->result);
			if(test_nb_flag)
			{
				sprintf(nb_test_info, "Could not disconnect:%d", err);
				TestNBUpdateINfor();
			}
			
			break;
		}

		LOG_INF("[%s:%d] PUBACK packet id: %u\n", __func__, __LINE__,
				evt->param.puback.message_id);
		if(test_nb_flag)
		{
			sprintf(nb_test_info, "PUBACK packet id: %u", evt->param.puback.message_id);
			TestNBUpdateINfor();
		}
		break;

	case MQTT_EVT_SUBACK:
		if(evt->result != 0)
		{
			LOG_INF("MQTT SUBACK error %d\n", evt->result);
			if(test_nb_flag)
			{
				sprintf(nb_test_info, "MQTT SUBACK error:%d", evt->result);
				TestNBUpdateINfor();
			}
			
			break;
		}

		LOG_INF("[%s:%d] SUBACK packet id: %u\n", __func__, __LINE__,
				evt->param.suback.message_id);
		if(test_nb_flag)
		{
			sprintf(nb_test_info, "SUBACK packet id: %u", evt->param.suback.message_id);
			TestNBUpdateINfor();
		}
		break;

	default:
		LOG_INF("[%s:%d] default: %d\n", __func__, __LINE__,
				evt->type);
		if(test_nb_flag)
		{
			sprintf(nb_test_info, "default: %d", evt->type);
			TestNBUpdateINfor();
		}
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
		if(test_nb_flag)
		{
			sprintf(nb_test_info, "ERROR: getaddrinfo failed:%d", err);
			TestNBUpdateINfor();
		}
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
			if(test_nb_flag)
			{
				sprintf(nb_test_info, "IPv4 Address found:%s", ipv4_addr);
				TestNBUpdateINfor();
			}
			break;
		}
		else
		{
			LOG_INF("ai_addrlen = %u should be %u or %u\n",
				(unsigned int)addr->ai_addrlen,
				(unsigned int)sizeof(struct sockaddr_in),
				(unsigned int)sizeof(struct sockaddr_in6));
			
			if(test_nb_flag)
			{
				sprintf(nb_test_info, "ai_addrlen = %u should be %u or %u",
				(unsigned int)addr->ai_addrlen,
				(unsigned int)sizeof(struct sockaddr_in),
				(unsigned int)sizeof(struct sockaddr_in6));
				TestNBUpdateINfor();
			}
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

static void mqtt_link(struct k_work_q *work_q)
{
	int err,i=0;

	LOG_INF("[%s] begin\n", __func__);

	client_init(&client);

	err = mqtt_connect(&client);
	if(err != 0)
	{
		LOG_INF("ERROR: mqtt_connect %d\n", err);
		return;
	}

	err = fds_init(&client);
	if(err != 0)
	{
		LOG_INF("ERROR: fds_init %d\n", err);
		return;
	}

	while(1)
	{
		err = poll(&fds, 1, mqtt_keepalive_time_left(&client));
		if(err < 0)
		{
			LOG_INF("ERROR: poll %d\n", errno);
			break;
		}

		err = mqtt_live(&client);
		if((err != 0) && (err != -EAGAIN))
		{
			LOG_INF("ERROR: mqtt_live %d\n", err);
			break;
		}

		if((fds.revents & POLLIN) == POLLIN)
		{
			err = mqtt_input(&client);
			if(err != 0)
			{
				LOG_INF("ERROR: mqtt_input %d\n", err);
				break;
			}
		}

		if((fds.revents & POLLERR) == POLLERR)
		{
			LOG_INF("POLLERR\n");
			break;
		}

		if((fds.revents & POLLNVAL) == POLLNVAL)
		{
			LOG_INF("POLLNVAL\n");
			break;
		}

		//k_sleep(K_MSEC(5000));
	}

	LOG_INF("[%s]: Disconnecting MQTT client...\n", __func__);

	err = mqtt_disconnect(&client);
	if(err)
	{
		LOG_INF("[%s]: Could not disconnect MQTT client. Error: %d\n", __func__, err);
	}
}

static void SendDataCallBack(struct k_timer *timer)
{
	send_data_flag = true;
}

static void NbSendDataStart(void)
{
	k_timer_start(&send_data_timer, K_MSEC(500), NULL);
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

		k_timer_start(&send_data_timer, K_MSEC(1000), NULL);
	}
}

static void MqttDisConnect(void)
{
	int err;

	LOG_INF("[%s]: begin\n", __func__);
	err = mqtt_disconnect(&client);
	if(err)
	{
		LOG_INF("[%s]: Could not disconnect MQTT client. Error: %d\n", __func__, err);
	}
}

static void MqttDisConnectCallBack(struct k_timer *timer_id)
{
	LOG_INF("[%s]: begin\n", __func__);
	mqtt_disconnect_flag = true;
}

static void MqttDicConnectStart(void)
{
	LOG_INF("[%s]: begin\n", __func__);
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
	/* RSRP raw values that represent actual signal strength are
	 * 0 through 97 (per "nRF91 AT Commands" v1.1). If the received value
	 * falls outside this range, we should not send the value.
	 */
	LOG_INF("rsrp_value:%d\n", rsrp_value);
	if(test_nb_flag)
	{
		sprintf(nb_test_info, "rsrp_value:%d", rsrp_value);
		nb_test_update_flag = true;
	}
	
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
	if(test_nb_flag)
	{
		strcpy(nb_test_info, "modem_configure");
		TestNBUpdateINfor();
	}

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
		if(test_nb_flag)
		{
			strcpy(nb_test_info, "Waitng for carrier registration...");
			TestNBUpdateINfor();
		}
		k_sem_take(&carrier_registered, K_FOREVER);
		LOG_INF("Registered!\n");
		if(test_nb_flag)
		{
			strcpy(nb_test_info, "Registered!");
			TestNBUpdateINfor();
		}
	#else /* defined(CONFIG_LWM2M_CARRIER) */
		int err;

		LOG_INF("LTE Link Connecting ...\n");
		if(test_nb_flag)
		{
			strcpy(nb_test_info, "LTE Link Connecting ...");
			TestNBUpdateINfor();
		}
		err = lte_lc_init_and_connect();
		__ASSERT(err == 0, "LTE link could not be established.");
		LOG_INF("LTE Link Connected!\n");
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
	int err;

	nb_is_running = true;
	test_nb_flag = true;
	
	LOG_INF("Start NB-IoT test!\n");

	EnterNBTestScreen();
	TestNBShowInfor();

	modem_configure();

	k_timer_start(&get_nw_rsrp_timer, K_MSEC(1000), K_MSEC(1000));
#if 0
	client_init(&client);

	err = mqtt_connect(&client);
	if(err != 0)
	{
		LOG_INF("ERROR: mqtt_connect %d\n", err);
		if(test_nb_flag)
		{
			sprintf(nb_test_info, "ERROR: mqtt_connect %d", err);
			TestNBUpdateINfor();
		}

		return;
	}

	err = fds_init(&client);
	if(err != 0)
	{
		LOG_INF("ERROR: fds_init %d\n", err);
		if(test_nb_flag)
		{
			sprintf(nb_test_info, "ERROR: fds_init %d", err);
			TestNBUpdateINfor();
		}
	
		return;
	}

	while(1)
	{
		err = poll(&fds, 1, mqtt_keepalive_time_left(&client));
		if(err < 0)
		{
			LOG_INF("ERROR: poll %d\n", err);
			if(test_nb_flag)
			{
				sprintf(nb_test_info, "ERROR: poll %d", err);
				TestNBUpdateINfor();
			}
		
			break;
		}

		err = mqtt_live(&client);
		if((err != 0) && (err != -EAGAIN))
		{
			LOG_INF("ERROR: mqtt_live %d\n", err);
			if(test_nb_flag)
			{
				sprintf(nb_test_info, "ERROR: mqtt_live %d", err);
				TestNBUpdateINfor();
			}
		
			break;
		}

		if((fds.revents & POLLIN) == POLLIN)
		{
			err = mqtt_input(&client);
			if(err != 0)
			{
				LOG_INF("ERROR: mqtt_input %d\n", err);
				if(test_nb_flag)
				{
					sprintf(nb_test_info, "ERROR: mqtt_input %d", err);
					TestNBUpdateINfor();
				}
			
				break;
			}
		}

		if((fds.revents & POLLERR) == POLLERR)
		{
			LOG_INF("POLLERR\n");
			if(test_nb_flag)
			{
				strcpy(nb_test_info, "POLLERR");
				TestNBUpdateINfor();
			}
		
			break;
		}

		if((fds.revents & POLLNVAL) == POLLNVAL)
		{
			LOG_INF("POLLNVAL\n");
			if(test_nb_flag)
			{
				strcpy(nb_test_info, "POLLNVAL");
				TestNBUpdateINfor();
			}
		
			break;
		}
	}

	LOG_INF("Disconnecting MQTT client...\n");
	if(test_nb_flag)
	{
		strcpy(nb_test_info, "Disconnecting MQTT client...");
		TestNBUpdateINfor();
	}

	err = mqtt_disconnect(&client);
	if(err)
	{
		LOG_INF("Could not disconnect MQTT client. Error: %d\n", err);
		if(test_nb_flag)
		{
			sprintf(nb_test_info, "Could not disconnect MQTT client. Error: %d\n", err);
			TestNBUpdateINfor();
		}
	}
#endif	
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
	u8_t tz_dir[2],tz_count,daylight;

	if(at_cmd_write("AT%CCLK?", timebuf, sizeof(timebuf), NULL) != 0)
	{
		LOG_INF("Get CCLK fail!\n");

		if(nb_connected)
			k_timer_start(&get_nw_time_timer, K_MSEC(1000), NULL);
		
		return;
	}
	LOG_INF("%s\n", timebuf);

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
		
	}

	LOG_INF("real time:%04d/%02d/%02d,%02d:%02d:%02d,%02d\n", 
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
	LOG_INF("[%s]: data add ret:%d\n", __func__, ret);
	
	if(mqtt_connected)
	{
		LOG_INF("[%s]: begin 001\n", __func__);

		NbSendDataStart();
	}
	else
	{
		LOG_INF("[%s]: begin 002\n", __func__);

		if(nb_connected)
		{
			k_delayed_work_submit_to_queue(app_work_q, &mqtt_link_work, K_NO_WAIT);
		}
		else
		{
			if(k_work_pending(&nb_link_work))
				k_delayed_work_cancel(&nb_link_work);
			
			k_delayed_work_submit_to_queue(app_work_q, &nb_link_work, K_NO_WAIT);
		}
	}
}

void NBSendSettingReply(u8_t *data, u32_t datalen)
{
	u8_t buf[128] = {0};
	u8_t tmpbuf[128] = {0};

	strcpy(buf, "{1:1:0:0:");
	strcat(buf, g_imei);
	strcat(buf, ":");
	strcat(buf, data);
	strcat(buf, ":");
	GetSystemTimeSecString(tmpbuf);
	strcat(buf, tmpbuf);	
	strcat(buf, "}");

	LOG_INF("[%s] reply data:%s\n", __func__, buf);
	MqttSendData(buf, strlen(buf));
}

void NBSendSosWifiData(u8_t *data, u32_t datalen)
{
	u8_t buf[128] = {0};
	u8_t tmpbuf[128] = {0};
	
	LOG_INF("[%s] wifi data:%s len:%d\n", __func__, data, datalen);

	strcpy(buf, "{1:1:0:0:");
	strcat(buf, g_imei);
	strcat(buf, ":T1:");
	strcat(buf, data);
	strcat(buf, ",");
	strcat(buf, sos_trigger_time);
	strcat(buf, "}");

	LOG_INF("[%s] sos wifi data:%s\n", __func__, buf);
	MqttSendData(buf, strlen(buf));
}

void NBSendSosGpsData(u8_t *data, u32_t datalen)
{
	u8_t buf[128] = {0};
	u8_t tmpbuf[128] = {0};
	
	LOG_INF("[%s] gps data:%s len:%d\n", __func__, data, datalen);

	strcpy(buf, "{T2,");
	strcat(buf, g_imei);
	strcat(buf, ",[");
	strcat(buf, data);
	GetSystemTimeSecString(tmpbuf);
	strcat(buf, tmpbuf);
	strcat(buf, "]}");

	LOG_INF("[%s] sos gps data:%s\n", __func__, buf);
	MqttSendData(buf, strlen(buf));
}

void NBSendFallWifiData(u8_t *data, u32_t datalen)
{
	u8_t buf[128] = {0};
	u8_t tmpbuf[128] = {0};
	
	LOG_INF("[%s] wifi data:%s len:%d\n", __func__, data, datalen);

	strcpy(buf, "{1:1:0:0:");
	strcat(buf, g_imei);
	strcat(buf, ":T3:");
	strcat(buf, data);
	strcat(buf, ",");
	strcat(buf, fall_trigger_time);
	strcat(buf, "}");

	LOG_INF("[%s] fall wifi data:%s\n", __func__, buf);
	MqttSendData(buf, strlen(buf));
}

void NBSendFallGpsData(u8_t *data, u32_t datalen)
{
	u8_t buf[128] = {0};
	u8_t tmpbuf[128] = {0};
	
	LOG_INF("[%s] gps data:%s len:%d\n", __func__, data, datalen);

	strcpy(buf, "{T4,");
	strcat(buf, g_imei);
	strcat(buf, ",[");
	strcat(buf, data);
	GetSystemTimeSecString(tmpbuf);
	strcat(buf, tmpbuf);
	strcat(buf, "]}");

	LOG_INF("[%s] fall gps data:%s\n", __func__, buf);
	MqttSendData(buf, strlen(buf));
}

void NBSendHealthData(u8_t *data, u32_t datalen)
{
	u8_t buf[128] = {0};
	u8_t tmpbuf[128] = {0};
	
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

	LOG_INF("[%s] health data:%s\n", __func__, buf);
	MqttSendData(buf, strlen(buf));
}

void NBSendLocationData(u8_t *data, u32_t datalen)
{
	u8_t buf[128] = {0};
	u8_t tmpbuf[128] = {0};
	
	strcpy(buf, "{1:1:0:0:");
	strcat(buf, g_imei);
	strcat(buf, ":T6:");
	strcat(buf, data);
	strcat(buf, ",");
	memset(tmpbuf, 0, sizeof(tmpbuf));
	GetSystemTimeSecString(tmpbuf);
	strcat(buf, tmpbuf);
	strcat(buf, "}");

	LOG_INF("[%s] location data:%s\n", __func__, buf);
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
	LOG_INF("[%s]: ret:%d, cmd:%s, data:%s\n", __func__, ret,strcmd,strdata);
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
	LOG_INF("[%s]: data add ret:%d\n", __func__, ret);
	
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
		LOG_INF("lte_lc_psm_req, error: %d\n", err);
	}
#else
	err = lte_lc_psm_req(false);
	if(err)
	{
		LOG_INF("lte_lc_psm_req, error: %d\n", err);
	}
#endif

#if defined(CONFIG_LTE_EDRX_ENABLE)
	/** enhanced Discontinuous Reception */
	err = lte_lc_edrx_req(true);
	if(err)
	{
		LOG_INF("lte_lc_edrx_req, error: %d\n", err);
	}
#else
	err = lte_lc_edrx_req(false);
	if(err)
	{
		LOG_INF("lte_lc_edrx_req, error: %d\n", err);
	}
#endif

#if defined(CONFIG_LTE_RAI_ENABLE)
	/** Release Assistance Indication  */
	err = at_cmd_write(CMD_SET_RAI, NULL, 0, NULL);
	if(err)
	{
		LOG_INF("lte_lc_rai_req, error: %d\n", err);
	}
#endif

	return err;
}

void GetModemSignal(void)
{
	char *ptr;
	int i=0,len;
	u8_t strbuf[128] = {0};
	u8_t tmpbuf[128] = {0};
	s32_t rsrp;
	static s32_t rsrpbk = 0;
	
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
	rsrp = atoi(strbuf);
	if(rsrp != rsrpbk)
	{
		rsrpbk = rsrp;
		sprintf(nb_test_info, "NB signal(rsrp):%ddb", (rsrp-141));
		nb_test_update_flag = true;
	}

}

void GetNetWorkSignalCallBack(struct k_timer *timer_id)
{
	get_modem_signal_flag = true;
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

static void SetModemTurnOff(void)
{
	if(at_cmd_write("AT+CFUN=4", NULL, 0, NULL) != 0)
	{
		LOG_INF("Can't turn off modem!");
		return;
	}	
	LOG_INF("turn off modem success!");
}

void SetModemAPN(void)
{
	u8_t tmpbuf[128] = {0};
	
	if(at_cmd_write("AT+CGDCONT=0,\"IP\",\"arkessalp.com\"", tmpbuf, sizeof(tmpbuf), NULL) != 0)
	{
		LOG_INF("[%s] set apn fail!\n", __func__);
	}

	if(at_cmd_write(CMD_GET_APN, tmpbuf, sizeof(tmpbuf), NULL) != 0)
	{
		LOG_INF("[%s] Get apn fail!\n", __func__);
		return;
	}
	LOG_INF("[%s] apn:%s\n", __func__, tmpbuf);	
}

static void nb_link(struct k_work *work)
{
	int err;
	u8_t tmpbuf[128] = {0};
	static u32_t retry_count = 0;
	
	LOG_INF("[%s] begin\n", __func__);

	configure_low_power();

	err = lte_lc_init_and_connect();
	if(err)
	{
		LOG_INF("Can't connected to LTE network");
		SetModemTurnOff();

		nb_connected = false;
		
		retry_count++;
		if(retry_count <= 5)	//5次以内每半分钟重连一次
			k_delayed_work_submit_to_queue(app_work_q, &nb_link_work, K_SECONDS(30));
		else if(retry_count <= 10)	//6到10次每分钟重连一次
			k_delayed_work_submit_to_queue(app_work_q, &nb_link_work, K_SECONDS(60));
		else if(retry_count <= 15)	//11到15次每5分钟重连一次
			k_delayed_work_submit_to_queue(app_work_q, &nb_link_work, K_SECONDS(300));		
		else if(retry_count <= 20)	//16到20次每10分钟重连一次
			k_delayed_work_submit_to_queue(app_work_q, &nb_link_work, K_SECONDS(600));
		else if(retry_count <= 25)	//21到25次每30分钟重连一次
			k_delayed_work_submit_to_queue(app_work_q, &nb_link_work, K_SECONDS(1800));
		else						//26次以上每1小时重连一次
			k_delayed_work_submit_to_queue(app_work_q, &nb_link_work, K_SECONDS(3600));
	}
	else
	{
		LOG_INF("Connected to LTE network");
		nb_connected = true;
		retry_count = 0;
		
		GetModemDateTime();
		modem_data_init();
	}

	GetModemInfor();
	
	if(!err)
	{
		k_delayed_work_submit_to_queue(app_work_q, &mqtt_link_work, K_SECONDS(2));
	}
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

	if(at_cmd_write(CMD_GET_APN, str_rsrp, sizeof(str_rsrp), NULL) != 0)
	{
		LOG_INF("Get apn fail!\n");
		return;
	}
	LOG_INF("apn:%s\n", str_rsrp);

	if(at_cmd_write(CMD_GET_CSQ, str_rsrp, sizeof(str_rsrp), NULL) != 0)
	{
		LOG_INF("Get csq fail!\n");
		return;
	}
	LOG_INF("csq:%s\n", str_rsrp);
}

void nb_test_update(void)
{
	if(screen_id == SCREEN_ID_NB_TEST)
		scr_msg[screen_id].act = SCREEN_ACTION_UPDATE;
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

	if(get_modem_info_flag)
	{
		GetModemInfor();
		get_modem_info_flag = false;
	}

	if(get_modem_signal_flag)
	{
		GetModemSignal();
		get_modem_signal_flag = false;
	}

	if(get_modem_time_flag)
	{
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
}

void NB_init(struct k_work_q *work_q)
{
	int err;

	app_work_q = work_q;

	k_delayed_work_init(&nb_link_work, nb_link);
	k_delayed_work_init(&mqtt_link_work, mqtt_link);

	k_delayed_work_submit_to_queue(app_work_q, &nb_link_work, K_SECONDS(2));
}
