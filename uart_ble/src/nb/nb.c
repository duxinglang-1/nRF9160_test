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
#include <modem/at_cmd.h>
#include <modem/at_cmd_parser.h>
#include <modem/at_params.h>
#include <modem/at_notif.h>
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
#ifdef CONFIG_SYNC_SUPPORT
#include "sync.h"
#endif
#include "logger.h"

//#define NB_DEBUG

#define LTE_TAU_WAKEUP_EARLY_TIME	(30)
#define MQTT_CONNECTED_KEEP_TIME	(1*60)

#define AT_CEREG_REG_STATUS_INDEX		1
#define AT_RESPONSE_PREFIX_INDEX		0
#define AT_CEREG_RESPONSE_PREFIX		"+CEREG"
#define AT_CEREG_PARAMS_COUNT_MAX		10

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
static void GetModemInforCallBack(struct k_timer *timer_id);
K_TIMER_DEFINE(get_modem_infor_timer, GetModemInforCallBack, NULL);
static void GetModemStatusCallBack(struct k_timer *timer_id);
K_TIMER_DEFINE(get_modem_status_timer, GetModemStatusCallBack, NULL);
static void NBReconnectCallBack(struct k_timer *timer_id);
K_TIMER_DEFINE(nb_reconnect_timer, NBReconnectCallBack, NULL);

#if defined(CONFIG_LTE_NETWORK_MODE_NBIOT)
/* Preferred network mode: Narrowband-IoT */
static const char nw_mode_preferred[] = "AT%XSYSTEMMODE=0,1,0,0";
/* Fallback network mode: LTE-M */
static const char nw_mode_fallback[] = "AT%XSYSTEMMODE=1,0,0,0";
#elif defined(CONFIG_LTE_NETWORK_MODE_NBIOT_GPS)
/* Preferred network mode: Narrowband-IoT and GPS */
static const char nw_mode_preferred[] = "AT%XSYSTEMMODE=0,1,1,0";
/* Fallback network mode: LTE-M and GPS*/
static const char nw_mode_fallback[] = "AT%XSYSTEMMODE=1,0,1,0";
#elif defined(CONFIG_LTE_NETWORK_MODE_LTE_M)
/* Preferred network mode: LTE-M */
static const char nw_mode_preferred[] = "AT%XSYSTEMMODE=1,0,0,0";
/* Fallback network mode: Narrowband-IoT */
static const char nw_mode_fallback[] = "AT%XSYSTEMMODE=0,1,0,0";
#elif defined(CONFIG_LTE_NETWORK_MODE_LTE_M_GPS)
/* Preferred network mode: LTE-M and GPS*/
static const char nw_mode_preferred[] = "AT%XSYSTEMMODE=1,0,1,0";
/* Fallback network mode: Narrowband-IoT and GPS */
static const char nw_mode_fallback[] = "AT%XSYSTEMMODE=0,1,1,0";
#endif

/* Set the modem to power off mode */
static const char power_off[] = "AT+CFUN=0";
/* Set the modem to Normal mode */
static const char normal[] = "AT+CFUN=1";
/* Set the modem to Offline mode */
static const char offline[] = "AT+CFUN=4";

static struct k_work_q *app_work_q;
static struct k_delayed_work modem_init_work;
static struct k_delayed_work nb_link_work;
static struct k_delayed_work mqtt_link_work;

NB_SIGNL_LEVEL g_nb_sig = NB_SIG_LEVEL_NO;

static struct modem_param_info modem_param;
static bool nb_redraw_sig_flag = false;
static bool send_data_flag = false;
static bool parse_data_flag = false;
static bool mqtt_disconnect_flag = false;
static bool power_on_data_flag = true;
static bool nb_connecting_flag = false;
static bool mqtt_connecting_flag = false;
static bool nb_reconnect_flag = false;
static bool get_modem_status_flag = false;

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
static u8_t broker_hostname[128] = CONFIG_MQTT_DOMESTIC_BROKER_HOSTNAME;
static u32_t broker_port = CONFIG_MQTT_DOMESTIC_BROKER_PORT;
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
u32_t g_tau_ext_time = 0;
static struct k_sem net_link;

#if defined(CONFIG_LTE_NETWORK_MODE_NBIOT)||defined(CONFIG_LTE_NETWORK_MODE_NBIOT_GPS)
NETWORK_MODE g_net_mode = NET_MODE_NB;
#elif defined(CONFIG_LTE_NETWORK_MODE_LTE_M)||defined(CONFIG_LTE_NETWORK_MODE_LTE_M_GPS)
NETWORK_MODE g_net_mode = NET_MODE_LTE_M;
#endif

static NB_APN_PARAMENT nb_apn_table[] = 
{
	//china mobile
	{	
		"46004",
		"cmnbiot2"
	},
	//china unicom
	{
		"46006",
		"unim2m.njm2mapn"
	},
	//china telcom
	{
		"46011",
		"ctnb"
	},	
	//arkessa
	{
		"90128", 
		"arkessalp.com",
	},
	//1NCE
	{
		"90140",
		"iot.1nce.net",	
	},
	//Rakuten
	{
		"44011",
		"iot.biz.rakuten.jp",
	},
	//super
	{
		"47475",
		"super",
	},
};


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
	#ifdef NB_DEBUG	
		LOGD("LWM2M_CARRIER_EVENT_BSDLIB_INIT");
	#endif
		break;
	case LWM2M_CARRIER_EVENT_CONNECT:
	#ifdef NB_DEBUG	
		LOGD("LWM2M_CARRIER_EVENT_CONNECT");
	#endif
		break;
	case LWM2M_CARRIER_EVENT_DISCONNECT:
	#ifdef NB_DEBUG
		LOGD("LWM2M_CARRIER_EVENT_DISCONNECT");
	#endif
		break;
	case LWM2M_CARRIER_EVENT_READY:
	#ifdef NB_DEBUG
		LOGD("LWM2M_CARRIER_EVENT_READY");
	#endif
		k_sem_give(&carrier_registered);
		break;
	case LWM2M_CARRIER_EVENT_FOTA_START:
	#ifdef NB_DEBUG	
		LOGD("LWM2M_CARRIER_EVENT_FOTA_START");
	#endif
		break;
	case LWM2M_CARRIER_EVENT_REBOOT:
	#ifdef NB_DEBUG	
		LOGD("LWM2M_CARRIER_EVENT_REBOOT");
	#endif
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
		.message_id = 0001
	};
#ifdef NB_DEBUG
	LOGD("Subscribing to:%s, len:%d", subscribe_topic.topic.utf8,
		subscribe_topic.topic.size);
#endif
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
		#ifdef NB_DEBUG	
			LOGD("mqtt_read_publish_payload: EAGAIN");
		#endif
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
		#ifdef NB_DEBUG
			LOGD("MQTT connect failed %d", evt->result);
		#endif
			break;
		}

		mqtt_connected = true;
	#ifdef NB_DEBUG
		LOGD("MQTT client connected!");		
	#endif
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
	#ifdef NB_DEBUG
		LOGD("MQTT client disconnected %d", evt->result);
	#endif
		mqtt_connected = false;
		
		NbSendDataStop();
		MqttDicConnectStop();
		break;

	case MQTT_EVT_PUBLISH: 
		{
			const struct mqtt_publish_param *p = &evt->param.publish;
		#ifdef NB_DEBUG
			LOGD("MQTT PUBLISH result=%d len=%d", evt->result, p->message.payload.len);
		#endif
			err = publish_get_payload(c, p->message.payload.len);
			if(err >= 0)
			{
				MqttReceData(payload_buf, p->message.payload.len);
			}
			else
			{
			#ifdef NB_DEBUG
				LOGD("mqtt_read_publish_payload: Failed! %d", err);
				LOGD("Disconnecting MQTT client...");
			#endif	
				err = mqtt_disconnect(c);
				if(err)
				{
				#ifdef NB_DEBUG
					LOGD("Could not disconnect: %d", err);
				#endif
				}
			}
		} 
		break;

	case MQTT_EVT_PUBACK:
		if(evt->result != 0)
		{
		#ifdef NB_DEBUG
			LOGD("MQTT PUBACK error %d", evt->result);
		#endif
			break;
		}
	#ifdef NB_DEBUG	
		LOGD("PUBACK packet id: %u", evt->param.puback.message_id);
	#endif

	#ifdef CONFIG_SYNC_SUPPORT
		SyncNetWorkCallBack(SYNC_STATUS_SENT);
	#endif	
		break;

	case MQTT_EVT_SUBACK:
		if(evt->result != 0)
		{
		#ifdef NB_DEBUG
			LOGD("MQTT SUBACK error %d", evt->result);
		#endif
			break;
		}
	#ifdef NB_DEBUG
		LOGD("SUBACK packet id: %u", evt->param.suback.message_id);
	#endif
		break;

	default:
	#ifdef NB_DEBUG
		LOGD("default: %d", evt->type);
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

	err = getaddrinfo(broker_hostname, NULL, &hints, &result);
	if(err)
	{
	#ifdef NB_DEBUG
		LOGD("ERROR: getaddrinfo failed %d", err);
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
			broker4->sin_port = htons(broker_port);

			inet_ntop(AF_INET, &broker4->sin_addr.s_addr,
				  ipv4_addr, sizeof(ipv4_addr));
		#ifdef NB_DEBUG
			LOGD("IPv4 Address found %s", ipv4_addr);
		#endif
			break;
		}
		else
		{
		#ifdef NB_DEBUG
			LOGD("ai_addrlen = %u should be %u or %u",
				(unsigned int)addr->ai_addrlen,
				(unsigned int)sizeof(struct sockaddr_in),
				(unsigned int)sizeof(struct sockaddr_in6));
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
    tls_config->hostname = broker_hostname;
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

#ifdef NB_DEBUG
	LOGD("begin");
#endif
	mqtt_connecting_flag = true;
	
	client_init(&client);

	err = mqtt_connect(&client);
	if(err != 0)
	{
	#ifdef NB_DEBUG
		LOGD("ERROR: mqtt_connect %d", err);
	#endif
		return;
	}

	err = fds_init(&client);
	if(err != 0)
	{
	#ifdef NB_DEBUG
		LOGD("ERROR: fds_init %d", err);
	#endif
		return;
	}

	while(1)
	{
		err = poll(&fds, 1, mqtt_keepalive_time_left(&client));
		if(err < 0)
		{
		#ifdef NB_DEBUG
			LOGD("ERROR: poll %d", errno);
		#endif
			break;
		}

		err = mqtt_live(&client);
		if((err != 0) && (err != -EAGAIN))
		{
		#ifdef NB_DEBUG
			LOGD("ERROR: mqtt_live %d", err);
		#endif
			break;
		}

		if((fds.revents & POLLIN) == POLLIN)
		{
			err = mqtt_input(&client);
			if(err != 0)
			{
			#ifdef NB_DEBUG
				LOGD("ERROR: mqtt_input %d", err);
			#endif
				break;
			}
		}

		if((fds.revents & POLLERR) == POLLERR)
		{
		#ifdef NB_DEBUG
			LOGD("POLLERR");
		#endif
			break;
		}

		if((fds.revents & POLLNVAL) == POLLNVAL)
		{
		#ifdef NB_DEBUG
			LOGD("POLLNVAL");
		#endif
			break;
		}
	}
#ifdef NB_DEBUG	
	LOGD("Disconnecting MQTT client...");
#endif

	err = mqtt_disconnect(&client);
	if(err)
	{
	#ifdef NB_DEBUG
		LOGD("Could not disconnect MQTT client. Error: %d", err);
	#endif
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
#ifdef NB_DEBUG
	LOGD("mqtt_connected:%d", mqtt_connected);
#endif
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
		#ifdef NB_DEBUG
			LOGD("Could not disconnect MQTT client. Error: %d", err);
		#endif
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

#ifdef NB_DEBUG
	LOGD("begin");
#endif
	err = mqtt_disconnect(&client);
	if(err)
	{
	#ifdef NB_DEBUG
		LOGD("Could not disconnect MQTT client. Error: %d", err);
	#endif
	}
}

static void MqttDisConnectCallBack(struct k_timer *timer_id)
{
#ifdef NB_DEBUG
	LOGD("begin");
#endif
	mqtt_disconnect_flag = true;
}

static void MqttDicConnectStart(void)
{
#ifdef NB_DEBUG
	LOGD("begin");
#endif
	k_timer_start(&mqtt_disconnect_timer, K_SECONDS(MQTT_CONNECTED_KEEP_TIME), NULL);
}

static void MqttDicConnectStop(void)
{
	k_timer_stop(&mqtt_disconnect_timer);
}


/**@brief Callback handler for LTE RSRP data. */
static void modem_rsrp_handler(char rsrp_value)
{
	/* RSRP raw values that represent actual signal strength are
	 * 0 through 97 (per "nRF91 AT Commands" v1.1). If the received value
	 * falls outside this range, we should not send the value.
	 */
#ifdef NB_DEBUG
	LOGD("rsrp_value:%d", rsrp_value);
#endif
	g_rsrp = rsrp_value;
	nb_redraw_sig_flag = true;
}

#ifdef CONFIG_MODEM_INFO
/**brief Initialize LTE status containers. */
void modem_data_init(void)
{
	int err;

	err = modem_info_init();
	if(err)
	{
	#ifdef NB_DEBUG
		LOGD("Modem info could not be established: %d", err);
	#endif
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

	if(at_cmd_write("AT%CESQ=1", NULL, 0, NULL) != 0)
	{
	#ifdef NB_DEBUG
		LOGD("AT_CMD write fail!");
	#endif
	}

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
	#ifdef NB_DEBUG
		LOGD("Waitng for carrier registration...");
	#endif
		if(test_nb_flag)
		{
			strcpy(nb_test_info, "Waitng for carrier registration...");
			TestNBUpdateINfor();
		}
		k_sem_take(&carrier_registered, K_FOREVER);
	#ifdef NB_DEBUG
		LOGD("Registered!");
	#endif
		if(test_nb_flag)
		{
			strcpy(nb_test_info, "Registered!");
			TestNBUpdateINfor();
		}
	#else /* defined(CONFIG_LWM2M_CARRIER) */
		int err;
	#ifdef NB_DEBUG
		LOGD("LTE Link Connecting ...");
	#endif
		if(test_nb_flag)
		{
			strcpy(nb_test_info, "LTE Link Connecting ...");
			TestNBUpdateINfor();
		}
		err = lte_lc_init_and_connect();
		__ASSERT(err == 0, "LTE link could not be established.");
	#ifdef NB_DEBUG
		LOGD("LTE Link Connected!");
	#endif
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

void NBGetNetMode(u8_t *at_mode_set)
{
	u8_t *p1,*p2;
	u8_t tmpbuf[8] = {0};
	
	p1 = strstr(at_mode_set, "AT%XSYSTEMMODE");
	if(p1 != NULL)
	{
		p1 += strlen("AT%XSYSTEMMODE");
		p2 = strstr(p1, ",");
		if(p2 != NULL)
		{
			p1 = p2+1;
			p2 = strstr(p1, ",");
			if(p2 != NULL)
			{
				memcpy(tmpbuf, p1, p2-p1);
				if(atoi(tmpbuf) == 1)
					g_net_mode = NET_MODE_NB;
				else
					g_net_mode = NET_MODE_LTE_M;
			}
		}
	}
}

void NBRedrawNetMode(void)
{
	if(screen_id == SCREEN_ID_IDLE)
	{
		scr_msg[screen_id].para |= SCREEN_EVENT_UPDATE_NET_MODE;
		scr_msg[screen_id].act = SCREEN_ACTION_UPDATE;
	}
}

void NBRedrawSignal(void)
{
	u8_t *ptr;
	bool flag=false;
	u8_t strbuf[128] = {0};
	u8_t tmpbuf[128] = {0};

	if(at_cmd_write(CMD_GET_REG_STATUS, strbuf, sizeof(strbuf), NULL) == 0)
	{
		//+CEREG: <n>,<stat>[,[<tac>],[<ci>],[<AcT>][,<cause_type>],[<reject_cause>][,[<Active-Time>],[<Periodic-TAU>]]]]
		//<n>
		//	0 – Disable unsolicited result codes
		//	1 – Enable unsolicited result codes +CEREG:<stat>
		//	2 – Enable unsolicited result codes +CEREG:<stat>[,<tac>,<ci>,<AcT>]
		//	3 – Enable unsolicited result codes +CEREG:<stat>[,<tac>,<ci>,<AcT>[,<cause_type>,<reject_cause>]]
		//	4 – Enable unsolicited result codes +CEREG: <stat>[,[<tac>],[<ci>],[<AcT>][,,[,[<Active-Time>],[<Periodic-TAU>]]]]
		//	5 – Enable unsolicited result codes +CEREG: <stat>[,[<tac>],[<ci>],[<AcT>][,[<cause_type>],[<reject_cause>][,[<ActiveTime>],[<Periodic-TAU>]]]]
		//<stat>
		//	0 – Not registered. UE is not currently searching for an operator to register to.
		//	1 – Registered, home network.
		//	2 – Not registered, but UE is currently trying to attach or searching an operator to register to.
		//	3 – Registration denied.
		//	4 – Unknown (e.g. out of E-UTRAN coverage).
		//	5 – Registered, roaming.
		//	8 – Attached for emergency bearer services only.
		//	90 – Not registered due to UICC failure.
		//<tac>
		//	String. A 2-byte Tracking Area Code (TAC) in hexadecimal format.
		//<ci>
		//	String. A 4-byte E-UTRAN cell ID in hexadecimal format.
		//<AcT>
		//	7 – E-UTRAN
		//	9 – E-UTRAN NB-S1
		//<cause_type>
		//	0 – <reject_cause> contains an EPS Mobility Management (EMM) cause value. See 3GPP TS 24.301 Annex A.
		//<reject_cause>
		//	EMM cause value. See 3GPP TS 24.301 Annex A
		//<Active-Time>
		//	String. One byte in an 8-bit format.
		//	Indicates the Active Time value (T3324) allocated to the device in E-UTRAN. For the coding and value range, see the GPRS Timer 2 IE in 3GPP TS 24.008 Table 10.5.163/3GPP TS 24.008.
		//<Periodic-TAU>
		//	String. One byte in an 8-bit format.
		//	Indicates the extended periodic TAU value (T3412) allocated to the device in EUTRAN. For the coding and value range, see the GPRS Timer 3 IE in 3GPP TS 24.008 Table 10.5.163a/3GPP TS 24.008.
		u8_t *ptr;
		u8_t reg_status;
		
	#ifdef NB_DEBUG
		LOGD("%s", strbuf);
	#endif

		ptr = strstr(strbuf, "+CEREG: ");
		if(ptr)
		{
			//指令头
			ptr += strlen("+CEREG: ");
			//reg_status
			GetStringInforBySepa(ptr, ",", 2, tmpbuf);
			reg_status = atoi(tmpbuf);
		#ifdef NB_DEBUG
			LOGD("reg_status:%d", reg_status);
		#endif
			
			if(reg_status == 1 || reg_status == 5)
			{
				g_nw_registered = true;
			}
			else
			{
				g_nw_registered = false;
				nb_connected = false;
				
				if(k_timer_remaining_get(&nb_reconnect_timer) == 0)
					k_timer_start(&nb_reconnect_timer, K_SECONDS(10), NULL);
			}
		}
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
	#ifdef NB_DEBUG
		LOGD("%s", strbuf);
	#endif
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
		#ifdef NB_DEBUG
			LOGD("mode:%d", mode);
		#endif
			if(mode == 1)//connected
			{
				flag = true;	
			}
			else if(mode == 0)//idle
			{
			#ifdef NB_DEBUG
				LOGD("reg stat:%d", g_nw_registered);
			#endif
				if(g_nw_registered) 			//registered
				{
					flag = false;				//don't show no signal when ue is psm idle mode in registered, 
				}
				else							//not registered
				{
					flag = true;	
				}
			}
		}
	}

	if(g_rsrp > 97)
	{
		if(flag)
			g_nb_sig = NB_SIG_LEVEL_NO;
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
	static u8_t retry = 5;

	if(at_cmd_write("AT%CCLK?", timebuf, sizeof(timebuf), NULL) != 0)
	{
	#ifdef NB_DEBUG
		LOGD("Get CCLK fail!");
	#endif

	#ifndef NB_SIGNAL_TEST
		if(nb_connected && (retry > 0))
		{
			retry--;
			k_timer_start(&get_nw_time_timer, K_MSEC(1000), NULL);
		}
	#endif
		return;
	}

	retry = 5;
	
#ifdef NB_DEBUG	
	LOGD("%s", timebuf);
#endif
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
#ifdef NB_DEBUG	
	LOGD("real time:%04d/%02d/%02d,%02d:%02d:%02d,%02d", 
					date_time.year,date_time.month,date_time.day,
					date_time.hour,date_time.minute,date_time.second,
					date_time.week);
#endif
	RedrawSystemTime();
	SaveSystemDateTime();
}

static void MqttSendData(u8_t *data, u32_t datalen)
{
	int ret;

	ret = add_data_into_send_cache(data, datalen);
#ifdef NB_DEBUG
	LOGD("data add ret:%d", ret);
#endif
	if(mqtt_connected)
	{
	#ifdef NB_DEBUG
		LOGD("begin 001");
	#endif
		NbSendDataStart();
	}
	else
	{
	#ifdef NB_DEBUG
		LOGD("begin 002");
	#endif
		if(nb_connected)
		{
		#ifdef NB_DEBUG
			LOGD("begin 003");
		#endif
			if(!mqtt_connecting_flag)
			{
			#ifdef NB_DEBUG
				LOGD("begin 003.1");
			#endif
				k_delayed_work_submit_to_queue(app_work_q, &mqtt_link_work, K_NO_WAIT);
			}
			else
			{
			#ifdef NB_DEBUG
				LOGD("begin 003.2");
			#endif
			}
		}
		else
		{
		#ifdef NB_DEBUG
			LOGD("begin 004", __func__);
		#endif
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
#ifdef NB_DEBUG
	LOGD("eply data:%s", buf);
#endif
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
#ifdef NB_DEBUG	
	LOGD("sos wifi data:%s", buf);
#endif
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
#ifdef NB_DEBUG	
	LOGD("sos gps data:%s", buf);
#endif
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
#ifdef NB_DEBUG	
	LOGD("fall wifi data:%s", buf);
#endif
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
#ifdef NB_DEBUG
	LOGD("fall gps data:%s", buf);
#endif
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
#ifdef NB_DEBUG
	LOGD("health data:%s", buf);
#endif
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
#ifdef NB_DEBUG
	LOGD("location data:%s", buf);
#endif
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
#ifdef NB_DEBUG
	LOGD("pwr on infor:%s", buf);
#endif
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
#ifdef NB_DEBUG
	LOGD("pwr off infor:%s", buf);
#endif
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
#ifdef NB_DEBUG
	LOGD("ret:%d, cmd:%s, data:%s", ret,strcmd,strdata);
#endif
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
#ifdef NB_DEBUG
	LOGD("data add ret:%d", ret);
#endif
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
	#ifdef NB_DEBUG
		LOGD("lte_lc_psm_req, error: %d", err);
	#endif
	}
#else
	err = lte_lc_psm_req(false);
	if(err)
	{
	#ifdef NB_DEBUG
		LOGD("lte_lc_psm_req, error: %d", err);
	#endif
	}
#endif

#if defined(CONFIG_LTE_EDRX_ENABLE)
	/** enhanced Discontinuous Reception */
	err = lte_lc_edrx_req(true);
	if(err)
	{
	#ifdef NB_DEBUG
		LOGD("lte_lc_edrx_req, error: %d", err);
	#endif
	}
#else
	err = lte_lc_edrx_req(false);
	if(err)
	{
	#ifdef NB_DEBUG
		LOGD("lte_lc_edrx_req, error: %d", err);
	#endif
	}
#endif

#if defined(CONFIG_LTE_RAI_ENABLE)
	/** Release Assistance Indication  */
	err = at_cmd_write(CMD_SET_RAI, NULL, 0, NULL);
	if(err)
	{
	#ifdef NB_DEBUG
		LOGD("lte_lc_rai_req, error: %d", err);
	#endif
	}
#endif

	return err;
}

void GetModemSignal(void)
{
	char *ptr1,*ptr2;
	int i=0,len;
	u8_t strbuf[64] = {0};
	u8_t tmpbuf[64] = {0};
	s32_t rsrq=0,rsrp,snr;
	static s32_t rsrpbk = 0;
	
	if(at_cmd_write(CMD_GET_CESQ, tmpbuf, sizeof(tmpbuf), NULL) == 0)
	{
	#ifdef NB_DEBUG
		LOGD("%s", tmpbuf);
	#endif
		len = strlen(tmpbuf);
		ptr1 = tmpbuf;
		while(i<4)
		{
			ptr1 = strstr(ptr1, ",");
			ptr1++;
			i++;
		}
		//rsrq
		ptr2 = strstr(ptr1, ",");
		memcpy((char*)strbuf, ptr1, ptr2-ptr1);
		rsrq = atoi(strbuf);
		//rsrp
		ptr2++;
		memset(strbuf, 0, sizeof(strbuf));
		memcpy((char*)strbuf, ptr2, len-(ptr2-(char*)tmpbuf));
		rsrp = atoi(strbuf);
	}

	if(at_cmd_write(CMD_GET_SNR, tmpbuf, sizeof(tmpbuf), NULL) == 0)
	{
	#ifdef NB_DEBUG
		LOGD("%s", tmpbuf);
	#endif
		ptr1 = tmpbuf+9;
		ptr2 = strstr(ptr1, ",");
		memcpy((char*)strbuf, ptr1, ptr2-ptr1);
		snr = atoi(strbuf);
	}

	if(test_nb_flag)
	{
		sprintf(nb_test_info, " snr:%d(%ddB)\nrsrq:%d(%0.1fdB)\nrsrp:%d(%ddBm)", snr,(snr-24),rsrq,(rsrq/2-19.5),rsrp,(rsrp-141));
		nb_test_update_flag = true;
	}
	else
	{
		if(rsrp != rsrpbk)
		{
			rsrpbk = rsrp;
			modem_rsrp_handler(rsrp);
		}
	}
}

static void GetNetWorkTimeCallBack(struct k_timer *timer_id)
{
	get_modem_time_flag = true;
}

static void GetModemInforCallBack(struct k_timer *timer_id)
{
	get_modem_info_flag = true;
}

static void NBReconnectCallBack(struct k_timer *timer_id)
{
	nb_reconnect_flag = true;
}

static void GetNetWorkSignalCallBack(struct k_timer *timer_id)
{
	get_modem_signal_flag = true;
}

static void GetModemStatusCallBack(struct k_timer *timer_id)
{
	get_modem_status_flag = true;
}

void DecodeModemMonitor(u8_t *buf, u32_t len)
{
	u8_t reg_status = 0;
	u8_t *ptr;
	u8_t tmpbuf[128] = {0};
	u8_t strbuf[128] = {0};

	if(buf == NULL || len == 0)
		return;

#ifdef NB_DEBUG	
	LOGD("%s", buf);
#endif
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
		#ifdef NB_DEBUG
			LOGD("full name:%s", tmpbuf);
		#endif
			//short name
			memset(tmpbuf, 0, sizeof(tmpbuf));
			GetStringInforBySepa(ptr, ",", 3, tmpbuf);
		#ifdef NB_DEBUG
			LOGD("short name:%s", tmpbuf);
		#endif
			//plmn
			memset(tmpbuf, 0, sizeof(tmpbuf));
			GetStringInforBySepa(ptr, ",", 4, tmpbuf);
		#ifdef NB_DEBUG
			LOGD("plmn:%s", tmpbuf);
		#endif
			//tac
			memset(tmpbuf, 0, sizeof(tmpbuf));
			GetStringInforBySepa(ptr, ",", 5, tmpbuf);
		#ifdef NB_DEBUG
			LOGD("tac:%s", tmpbuf);
		#endif
			//AcT
			memset(tmpbuf, 0, sizeof(tmpbuf));
			GetStringInforBySepa(ptr, ",", 6, tmpbuf);
		#ifdef NB_DEBUG
			LOGD("AcT:%s", tmpbuf);
		#endif
			//band
			memset(tmpbuf, 0, sizeof(tmpbuf));
			GetStringInforBySepa(ptr, ",", 7, tmpbuf);
		#ifdef NB_DEBUG
			LOGD("band:%s",  tmpbuf);
		#endif
			//cell_id
			memset(tmpbuf, 0, sizeof(tmpbuf));
			GetStringInforBySepa(ptr, ",", 8, tmpbuf);
		#ifdef NB_DEBUG
			LOGD("cell_id:%s", tmpbuf);
		#endif
			//phys_cell_id
			memset(tmpbuf, 0, sizeof(tmpbuf));
			GetStringInforBySepa(ptr, ",", 9, tmpbuf);
		#ifdef NB_DEBUG
			LOGD("phys_cell_id:%s", tmpbuf);
		#endif
			//EARFCN
			memset(tmpbuf, 0, sizeof(tmpbuf));
			GetStringInforBySepa(ptr, ",", 10, tmpbuf);
		#ifdef NB_DEBUG
			LOGD("EARFCN:%s", tmpbuf);
		#endif
		#endif	
			//rsrp
			memset(tmpbuf, 0, sizeof(tmpbuf));
			GetStringInforBySepa(ptr, ",", 11, tmpbuf);
		#ifdef NB_DEBUG
			LOGD("rsrp:%s", tmpbuf);
		#endif
			g_rsrp = atoi(tmpbuf);
			modem_rsrp_handler(g_rsrp);
		#if 0	
			//snr
			memset(tmpbuf, 0, sizeof(tmpbuf));
			GetStringInforBySepa(ptr, ",", 12, tmpbuf);
		#ifdef NB_DEBUG
			LOGD("snr:%s", tmpbuf);
		#endif
			//NW-provided_eDRX_value
			memset(tmpbuf, 0, sizeof(tmpbuf));
			GetStringInforBySepa(ptr, ",", 13, tmpbuf);
		#ifdef NB_DEBUG
			LOGD("NW-provided_eDRX_value:%s", tmpbuf);
		#endif
		#endif
			//Active-Time
			memset(tmpbuf, 0, sizeof(tmpbuf));
			GetStringInforBySepa(ptr, ",", 14, tmpbuf);
		#ifdef NB_DEBUG
			LOGD("Active-Time:%s", tmpbuf);
		#endif
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
		#ifdef NB_DEBUG
			LOGD("g_act_time:%d", g_act_time);
		#endif
			//Periodic-TAU-ext
			memset(tmpbuf, 0, sizeof(tmpbuf));
			GetStringInforBySepa(ptr, ",", 15, tmpbuf);
		#ifdef NB_DEBUG
			LOGD("Periodic-TAU-ext:%s", tmpbuf);
		#endif
			memcpy(strbuf, &tmpbuf[1], 8);
			act = (strbuf[0]-0x30)*0x80+(strbuf[1]-0x30)*0x40+(strbuf[2]-0x30)*0x20;
			switch(act>>5)
			{
			case 0://0 0 0 – value is incremented in multiples of 10 minutes
				flag = 10*60;
				break;
			case 1://0 0 1 – value is incremented in multiples of 1 hour
				flag = 60*60;
				break;
			case 2://0 1 0 – value is incremented in multiples of 10 hours
				flag = 10*60*60;
				break;
			case 3://0 1 1 – value is incremented in multiples of 2 seconds
				flag = 2;
				break;
			case 4://1 0 0 – value is incremented in multiples of 30 seconds
				flag = 30;
				break;
			case 5://1 0 1 – value is incremented in multiples of 1 minute
				flag = 60;
				break;
			case 6://1 1 0 – value is incremented in multiples of 320 hours
				flag = 320*60*60;
				break;
			case 7://1 1 1 – value indicates that the timer is deactivated
				flag = 0;
				break;
			}
			g_tau_ext_time = flag*((strbuf[3]-0x30)*0x10+(strbuf[4]-0x30)*0x08+(strbuf[5]-0x30)*0x04+(strbuf[6]-0x30)*0x02+(strbuf[7]-0x30)*0x01);
		#ifdef NB_DEBUG
			LOGD("g_tau_ext_time:%d", g_tau_ext_time);
		#endif
			//Periodic-TAU
			memset(tmpbuf, 0, sizeof(tmpbuf));
			GetStringInforBySepa(ptr, ",", 16, tmpbuf);
		#ifdef NB_DEBUG
			LOGD("Periodic-TAU:%s", tmpbuf);
		#endif
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
		#ifdef NB_DEBUG
			LOGD("g_tau_time:%d", g_tau_time);
		#endif
		}
		else
		{
			g_nw_registered = false;
			nb_connected = false;
			g_rsrp = 255;
			modem_rsrp_handler(g_rsrp);
		}
	}
}

void SetNetWorkApn(u8_t *imsi_buf)
{
	u32_t i;
	u8_t tmpbuf[256] = {0};

	for(i=0;i<ARRAY_SIZE(nb_apn_table);i++)
	{
		if(strncmp(imsi_buf, nb_apn_table[i].plmn, strlen(nb_apn_table[i].plmn)) == 0)
		{
			u8_t cmdbuf[128] = {0};
			
			sprintf(cmdbuf, "AT+CGDCONT=0,\"IP\",\"%s\"", nb_apn_table[i].apn);
		#ifdef NB_DEBUG
			LOGD("cmdbuf:%s", cmdbuf); 
		#endif
			if(at_cmd_write(cmdbuf, tmpbuf, sizeof(tmpbuf), NULL) != 0)
			{
			#ifdef NB_DEBUG
				LOGD("set apn fail!");
			#endif
			}

			break;
		}
	}

	if(at_cmd_write(CMD_GET_APN, tmpbuf, sizeof(tmpbuf), NULL) == 0)
	{
	#ifdef NB_DEBUG
		LOGD("apn:%s", tmpbuf); 
	#endif
	}
}

void SetNwtWorkMqttBroker(u8_t *imsi_buf)
{
	if(strncmp(imsi_buf, "460", strlen("460")) == 0)
	{
		strcpy(broker_hostname, CONFIG_MQTT_DOMESTIC_BROKER_HOSTNAME);
		broker_port = CONFIG_MQTT_DOMESTIC_BROKER_PORT;
	}
	else
	{
		strcpy(broker_hostname, CONFIG_MQTT_FOREIGN_BROKER_HOSTNAME);
		broker_port = CONFIG_MQTT_FOREIGN_BROKER_PORT;
	}
}

void SetNetWorkParaByPlmn(u8_t *imsi)
{
	SetNetWorkApn(imsi);
	SetNwtWorkMqttBroker(imsi);
}

void GetModemInfor(void)
{
	u8_t tmpbuf[256] = {0};

	if(at_cmd_write(CMD_GET_MODEM_V, tmpbuf, sizeof(tmpbuf), NULL) == 0)
	{
	#ifdef NB_DEBUG
		LOGD("MODEM version:%s", &tmpbuf);
	#endif

		strncpy(g_modem, &tmpbuf, MODEM_MAX_LEN);
	}

#if 0
	if(at_cmd_write(CMD_GET_SUPPORT_BAND, tmpbuf, sizeof(tmpbuf), NULL) == 0)
	{
	#ifdef NB_DEBUG
		LOGD("support band:%s", tmpbuf);
	#endif
	}

	if(at_cmd_write(CMD_GET_CUR_BAND, tmpbuf, sizeof(tmpbuf), NULL) == 0)
	{
	#ifdef NB_DEBUG
		LOGD("current band:%s", tmpbuf);
	#endif
	}

	if(at_cmd_write(CMD_GET_LOCKED_BAND, tmpbuf, sizeof(tmpbuf), NULL) == 0)
	{
	#ifdef NB_DEBUG
		LOGD("locked band:%s", tmpbuf);
	#endif
	}
#endif

	if(at_cmd_write(CMD_GET_IMEI, tmpbuf, sizeof(tmpbuf), NULL) == 0)
	{
	#ifdef NB_DEBUG
		LOGD("imei:%s", tmpbuf);
	#endif
		strncpy(g_imei, tmpbuf, IMEI_MAX_LEN);
	}

	if(at_cmd_write(CMD_GET_IMSI, tmpbuf, sizeof(tmpbuf), NULL) == 0)
	{
	#ifdef NB_DEBUG
		LOGD("imsi:%s", tmpbuf);
	#endif
		strncpy(g_imsi, tmpbuf, IMSI_MAX_LEN);
	}

	if(at_cmd_write(CMD_GET_ICCID, tmpbuf, sizeof(tmpbuf), NULL) == 0)
	{
	#ifdef NB_DEBUG
		LOGD("iccid:%s", &tmpbuf[9]);
	#endif
		strncpy(g_iccid, &tmpbuf[9], ICCID_MAX_LEN);
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
	#ifdef NB_DEBUG
		LOGD("MODEM parameter:%s", &tmpbuf);
	#endif
		DecodeModemMonitor(tmpbuf, strlen(tmpbuf));
	}
}

void SetModemTurnOn(void)
{
	if(at_cmd_write("AT+CFUN=1", NULL, 0, NULL) == 0)
	{
	#ifdef NB_DEBUG
		LOGD("turn on modem success!");
	#endif
	}
	else
	{
	#ifdef NB_DEBUG
		LOGD("Can't turn on modem!");
	#endif
	}
}

void SetModemTurnOff(void)
{
	if(at_cmd_write("AT+CFUN=4", NULL, 0, NULL) == 0)
	{
	#ifdef NB_DEBUG
		LOGD("turn off modem success!");
	#endif
	}
	else
	{
	#ifdef NB_DEBUG
		LOGD("Can't turn off modem!");
	#endif
	}

	nb_connected = false;
}

void GetModemAPN(void)
{
	u8_t tmpbuf[128] = {0};
	
	if(at_cmd_write(CMD_GET_APN, tmpbuf, sizeof(tmpbuf), NULL) == 0)
	{
	#ifdef NB_DEBUG
		LOGD("apn:%s", tmpbuf);	
	#endif
	}
}

void SetModemAPN(void)
{
	u8_t tmpbuf[128] = {0};
	
	if(at_cmd_write("AT+CGDCONT=0,\"IP\",\"arkessalp.com\"", tmpbuf, sizeof(tmpbuf), NULL) != 0)
	{
	#ifdef NB_DEBUG
		LOGD("set apn fail!");
	#endif
	}

	if(at_cmd_write(CMD_GET_APN, tmpbuf, sizeof(tmpbuf), NULL) != 0)
	{
	#ifdef NB_DEBUG
		LOGD("Get apn fail!");
	#endif
		return;
	}
#ifdef NB_DEBUG
	LOGD("apn:%s", tmpbuf);	
#endif
}

static void modem_init(struct k_work *work)
{
	SetModemTurnOn();

	k_delayed_work_submit_to_queue(app_work_q, &nb_link_work, K_SECONDS(2));
}

static bool response_is_valid(const char *response, size_t response_len, const char *check)
{
	if((response == NULL) || (check == NULL))
		return false;

	if((response_len < strlen(check))||(memcmp(response, check, response_len) != 0))
		return false;

	return true;
}

static int parse_nw_reg_status(const char *at_response, enum lte_lc_nw_reg_status *status, size_t reg_status_index)
{
	int err, reg_status;
	struct at_param_list resp_list = {0};
	char response_prefix[sizeof(AT_CEREG_RESPONSE_PREFIX)] = {0};
	size_t response_prefix_len = sizeof(response_prefix);

	if((at_response == NULL) || (status == NULL))
		return -EINVAL;

	err = at_params_list_init(&resp_list, AT_CEREG_PARAMS_COUNT_MAX);
	if(err)
		return err;

	/* Parse CEREG response and populate AT parameter list */
	err = at_parser_max_params_from_str(at_response,
					    NULL,
					    &resp_list,
					    AT_CEREG_PARAMS_COUNT_MAX);
	if(err)
		goto clean_exit;

	/* Check if AT command response starts with +CEREG */
	err = at_params_string_get(&resp_list,
				   AT_RESPONSE_PREFIX_INDEX,
				   response_prefix,
				   &response_prefix_len);
	if(err)
		goto clean_exit;

	if(!response_is_valid(response_prefix, response_prefix_len, AT_CEREG_RESPONSE_PREFIX))
		goto clean_exit;

	/* Get the network registration status parameter from the response */
	err = at_params_int_get(&resp_list, reg_status_index, &reg_status);
	if(err)
		goto clean_exit;

	/* Check if the parsed value maps to a valid registration status */
	switch(reg_status)
	{
	case LTE_LC_NW_REG_NOT_REGISTERED:
	case LTE_LC_NW_REG_REGISTERED_HOME:
	case LTE_LC_NW_REG_SEARCHING:
	case LTE_LC_NW_REG_REGISTRATION_DENIED:
	case LTE_LC_NW_REG_UNKNOWN:
	case LTE_LC_NW_REG_REGISTERED_ROAMING:
	case LTE_LC_NW_REG_REGISTERED_EMERGENCY:
	case LTE_LC_NW_REG_UICC_FAIL:
		*status = reg_status;
		break;
		
	default:
		err = -EIO;
	}

clean_exit:
	at_params_list_free(&resp_list);
	return err;
}

static void at_handler(void *context, const char *response)
{
	ARG_UNUSED(context);

	int err;
	enum lte_lc_nw_reg_status status;

	if(response == NULL)
		return;

	err = parse_nw_reg_status(response, &status, AT_CEREG_REG_STATUS_INDEX);
	if(err)
		return;

	if((status == LTE_LC_NW_REG_REGISTERED_HOME) ||
	    (status == LTE_LC_NW_REG_REGISTERED_ROAMING))
	{
		k_sem_give(&net_link);
	}
}

static int net_connect(void)
{
	int err, rc;
	const char *current_network_mode = nw_mode_preferred;
	bool retry;

	k_sem_init(&net_link, 0, 1);

	rc = at_notif_register_handler(NULL, at_handler);
	if(rc != 0)
		return rc;

	do
	{
		retry = false;

		if(at_cmd_write(current_network_mode, NULL, 0, NULL) != 0)
		{
			err = -EIO;
			goto exit;
		}

		NBGetNetMode(current_network_mode);

		if(at_cmd_write(normal, NULL, 0, NULL) != 0)
		{
			err = -EIO;
			goto exit;
		}

		err = k_sem_take(&net_link, K_SECONDS(CONFIG_LTE_NETWORK_TIMEOUT));
		if(err == -EAGAIN)
		{
		#ifdef NB_DEBUG
			LOGD("Network connection attempt timed out");
		#endif

			if(IS_ENABLED(CONFIG_LTE_NETWORK_USE_FALLBACK)&&(current_network_mode == nw_mode_preferred))
			{
				current_network_mode = nw_mode_fallback;
				retry = true;

				if(at_cmd_write(offline, NULL, 0, NULL) != 0)
				{
					err = -EIO;
					goto exit;
				}
			#ifdef NB_DEBUG
				LOGD("Using fallback network mode");
			#endif
			}
			else
			{
				err = -ETIMEDOUT;
			}
		}
	}while(retry);

exit:
	rc = at_notif_deregister_handler(NULL, at_handler);
	if(rc != 0)
	{
	#ifdef NB_DEBUG
		LOGD("Can't de-register handler rc=%d", rc);
	#endif
	}

	return err;
}

static int net_init_and_link(void)
{
	int ret;

	ret = lte_lc_init();
	if(ret)
	{
		return ret;
	}

	return net_connect();
}

static void nb_link(struct k_work *work)
{
	int err=0;
	u8_t tmpbuf[128] = {0};
	static u32_t retry_count = 0;
	static bool frist_flag = false;

#ifndef NB_SIGNAL_TEST
	if(!frist_flag)
	{
		frist_flag = true;
		GetModemInfor();
		SetModemTurnOff();
		SetNetWorkParaByPlmn(g_imsi);
	}
	else if(strlen(g_imsi) == 0)
	{
		SetModemTurnOn();
		GetModemInfor();
		SetModemTurnOff();
		SetNetWorkParaByPlmn(g_imsi);
	}
#endif

	if(gps_is_working())
	{
	#ifdef NB_DEBUG
		LOGD("gps is working, continue waiting!");
	#endif
	
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
	#ifdef NB_DEBUG
		LOGD("linking");
	#endif
	
		nb_connecting_flag = true;
		configure_low_power();
		err = net_init_and_link();
		if(err)
		{
		#ifdef NB_DEBUG
			LOGD("Can't connected to LTE network. err:%d", err);
		#endif
		
		#ifdef CONFIG_SYNC_SUPPORT
			SyncNetWorkCallBack(SYNC_STATUS_FAIL);
		#endif

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
		#ifdef NB_DEBUG
			LOGD("Connected to LTE network");
		#endif
			
			nb_connected = true;
			retry_count = 0;
		#ifdef CONFIG_MODEM_INFO
			modem_data_init();
		#endif
			GetModemDateTime();
			NBRedrawNetMode();
		}

		GetModemStatus();

	#ifndef NB_SIGNAL_TEST
		if(!nb_connected)
			SetModemTurnOff();
		
		if(!err && !test_nb_flag)
		{
			k_delayed_work_submit_to_queue(app_work_q, &mqtt_link_work, K_SECONDS(2));
		}
	#endif

		nb_connecting_flag = false;
	}
}

void GetNBSignal(void)
{
	u8_t str_rsrp[128] = {0};

	if(at_cmd_write("AT+CFUN?", str_rsrp, sizeof(str_rsrp), NULL) == 0)
	{
	#ifdef NB_DEBUG	
		LOGD("cfun:%s", str_rsrp);
	#endif
	}

	if(at_cmd_write("AT+CPSMS?", str_rsrp, sizeof(str_rsrp), NULL) == 0)
	{
	#ifdef NB_DEBUG
		LOGD("cpsms:%s", str_rsrp);
	#endif
	}
	
	if(at_cmd_write(CMD_GET_CESQ, str_rsrp, sizeof(str_rsrp), NULL) == 0)
	{
	#ifdef NB_DEBUG	
		LOGD("cesq:%s", str_rsrp);
	#endif
	}

	if(at_cmd_write(CMD_GET_APN, str_rsrp, sizeof(str_rsrp), NULL) == 0)
	{
	#ifdef NB_DEBUG	
		LOGD("apn:%s", str_rsrp);
	#endif
	}
	
	if(at_cmd_write(CMD_GET_CSQ, str_rsrp, sizeof(str_rsrp), NULL) == 0)
	{
	#ifdef NB_DEBUG	
		LOGD("csq:%s", str_rsrp);
	#endif
	}
}

bool nb_is_connecting(void)
{
#ifdef NB_DEBUG
	LOGD("nb_connecting_flag:%d", nb_connecting_flag);
#endif
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
	#ifdef NB_DEBUG
		LOGD("Start NB-IoT test!");
	#endif
	
		if(nb_is_connected())
		{
			k_timer_start(&get_nw_rsrp_timer, K_MSEC(1000), K_MSEC(1000));
		}
		else if(nb_is_connecting())
		{
			strcpy(nb_test_info, "LTE Link Connecting ...");
			TestNBUpdateINfor();
		}
		else
		{
			SetModemTurnOff();
			modem_configure();
		}
	}

	if(get_modem_info_flag)
	{	
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
		GetModemSignal();
		get_modem_signal_flag = false;
	}

	if(get_modem_time_flag)
	{	
		GetModemDateTime();
		get_modem_time_flag = false;
	}

	if(get_modem_status_flag)
	{
		GetModemStatus();
		get_modem_status_flag = false;
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
		k_sleep(K_MSEC(5));
	}
}

void NB_init(struct k_work_q *work_q)
{
	int err;

	app_work_q = work_q;

	k_delayed_work_init(&modem_init_work, modem_init);
	k_delayed_work_init(&nb_link_work, nb_link);
	k_delayed_work_init(&mqtt_link_work, mqtt_link);
#ifdef CONFIG_FOTA_DOWNLOAD
	fota_work_init(work_q);
#endif
#ifdef CONFIG_DATA_DOWNLOAD_SUPPORT
	dl_work_init(work_q);
#endif

#ifdef NB_SIGNAL_TEST
	k_delayed_work_submit_to_queue(app_work_q, &nb_link_work, K_SECONDS(5));
#else
	k_delayed_work_submit_to_queue(app_work_q, &modem_init_work, K_SECONDS(5));
#endif
}
