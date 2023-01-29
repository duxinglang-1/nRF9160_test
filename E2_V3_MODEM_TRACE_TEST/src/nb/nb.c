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
#include <modem/at_cmd_parser.h>
#include <modem/at_params.h>
#include <modem/lte_lc.h>
#include <modem/lte_lc_trace.h>
#include <modem/modem_info.h>
#include <modem/nrf_modem_lib.h>
#include <modem/at_monitor.h>
#include <nrf_modem_at.h>
#if defined(CONFIG_LWM2M_CARRIER)
#include <lwm2m_carrier.h>
#endif
#if defined(CONFIG_BSD_LIBRARY)
#include <modem/bsdlib.h>
#include <bsd.h>
#endif /* CONFIG_BSD_LIBRARY */
#include <random/rand32.h>
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
#ifdef CONFIG_PPG_SUPPORT
#include "Max32674.h"
#endif
#include "transfer_cache.h"
#ifdef CONFIG_SYNC_SUPPORT
#include "sync.h"
#endif
#include "logger.h"

//#define NB_DEBUG

#define LTE_TAU_WAKEUP_EARLY_TIME	(30)
#define MQTT_CONNECTED_KEEP_TIME	(30)

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
static bool server_has_timed_flag = false;

#if defined(CONFIG_MQTT_LIB_TLS)
static sec_tag_t sec_tag_list[] = { CONFIG_SEC_TAG };
#endif/*CONFIG_MQTT_LIB_TLS*/ 

/* Buffers for MQTT client. */
static uint8_t rx_buffer[CONFIG_MQTT_MESSAGE_BUFFER_SIZE];
static uint8_t tx_buffer[CONFIG_MQTT_MESSAGE_BUFFER_SIZE];
static uint8_t payload_buf[CONFIG_MQTT_PAYLOAD_BUFFER_SIZE];

/* The mqtt client struct */
static struct mqtt_client client;

/* MQTT Broker details. */
static uint8_t broker_hostname[128] = CONFIG_MQTT_DOMESTIC_BROKER_HOSTNAME;
static uint32_t broker_port = CONFIG_MQTT_DOMESTIC_BROKER_PORT;
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

uint8_t g_imsi[IMSI_MAX_LEN+1] = {0};
uint8_t g_imei[IMEI_MAX_LEN+1] = {0};
uint8_t g_iccid[ICCID_MAX_LEN+1] = {0};
uint8_t g_modem[MODEM_MAX_LEN+1] = {0};

uint8_t g_prj_dir[128] = {0};
uint8_t g_new_fw_ver[64] = {0};
uint8_t g_new_modem_ver[64] = {0};
uint8_t g_new_ble_ver[64] = {0};
uint8_t g_new_wifi_ver[64] = {0};
uint8_t g_new_ui_ver[16] = {0};
uint8_t g_new_font_ver[16] = {0};
uint8_t g_new_ppg_ver[16] = {0};
uint8_t g_timezone[5] = {0};

uint8_t g_rsrp = 0;
uint16_t g_tau_time = 0;
uint16_t g_act_time = 0;
uint32_t g_tau_ext_time = 0;

uint8_t nb_test_info[256] = {0};

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
static void MqttReceData(uint8_t *data, uint32_t datalen);
static void MqttDicConnectStart(void);
static void MqttDicConnectStop(void);

//AT_MONITOR(at_cereg_info_mon, "CEREG", at_handler, PAUSED);
//AT_MONITOR(ltelc_atmon_cereg_info, "+CEREG", at_handler_cereg, PAUSED);
AT_MONITOR(ltelc_atmon_modem_info, "%XMODEMSLEEP", at_handler_modem_notify, PAUSED);

static void at_handler_modem_notify(const char *response)
{
	int err;
	struct lte_lc_evt evt = {0};

	__ASSERT_NO_MSG(response != NULL);

#ifdef NB_DEBUG
	LOGD("%%XMODEMSLEEP notification");
#endif

	err = parse_xmodemsleep(response, &evt.modem_sleep);
	if(err)
	{
	#ifdef NB_DEBUG
		LOGD("Can't parse modem sleep pre-warning notification, error: %d", err);
	#endif
		return;
	}

	/* Link controller only supports PSM, RF inactivity and flight mode
	 * modem sleep types.
	 */
	if((evt.modem_sleep.type != LTE_LC_MODEM_SLEEP_PSM) &&
		(evt.modem_sleep.type != LTE_LC_MODEM_SLEEP_RF_INACTIVITY) &&
		(evt.modem_sleep.type != LTE_LC_MODEM_SLEEP_FLIGHT_MODE))
	{
		return;
	}

	/* Propagate the appropriate event depending on the parsed time parameter. */
	if(evt.modem_sleep.time == CONFIG_LTE_LC_MODEM_SLEEP_PRE_WARNING_TIME_MS)
	{
		evt.type = LTE_LC_EVT_MODEM_SLEEP_EXIT_PRE_WARNING;
	}
	else if(evt.modem_sleep.time == 0)
	{
		LTE_LC_TRACE(LTE_LC_TRACE_MODEM_SLEEP_EXIT);
		evt.type = LTE_LC_EVT_MODEM_SLEEP_EXIT;
	}
	else
	{
		LTE_LC_TRACE(LTE_LC_TRACE_MODEM_SLEEP_ENTER);
		evt.type = LTE_LC_EVT_MODEM_SLEEP_ENTER;
	}

	event_handler_list_dispatch(&evt);
}

/**@brief Function to publish data on the configured topic
 */
static int data_publish(struct mqtt_client *c, enum mqtt_qos qos,
	uint8_t *data, size_t len)
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
			.utf8 = (uint8_t *)get_mqtt_topic(),
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
	uint8_t *buf = payload_buf;
	uint8_t *end = buf + length;

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
	
	client->client_id.utf8 = (uint8_t *)g_imei;	//xb add 2021-03-24 CONFIG_MQTT_CLIENT_ID;
	client->client_id.size = strlen(g_imei);

	password.utf8 = (uint8_t *)CONFIG_MQTT_PASSWORD;
	password.size = strlen(CONFIG_MQTT_PASSWORD);
	client->password = &password;

	username.utf8 = (uint8_t *)CONFIG_MQTT_USER_NAME;
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
		goto link_over;
	}

	err = fds_init(&client);
	if(err != 0)
	{
	#ifdef NB_DEBUG
		LOGD("ERROR: fds_init %d", err);
	#endif
		goto link_over;
	}

	mqtt_connecting_flag = false;
	
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

link_over:
	mqtt_connecting_flag = false;
}

static void SendDataCallBack(struct k_timer *timer)
{
	send_data_flag = true;
}

static void NbSendDataStart(void)
{
	k_timer_start(&send_data_timer, K_MSEC(500), K_NO_WAIT);
}

static void NbSendDataStop(void)
{
	k_timer_stop(&send_data_timer);
}

static void NbSendData(void)
{
	uint8_t *p_data;
	uint32_t data_len;
	int ret;

	ret = get_data_from_send_cache(&p_data, &data_len);
	if(ret)
	{
		ret = data_publish(&client, MQTT_QOS_1_AT_LEAST_ONCE, p_data, data_len);
		if(!ret)
		{
			delete_data_from_send_cache();
		}

		k_timer_start(&send_data_timer, K_MSEC(500), K_NO_WAIT);
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
	k_timer_start(&mqtt_disconnect_timer, K_SECONDS(MQTT_CONNECTED_KEEP_TIME), K_NO_WAIT);
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
	uint8_t buf[128] = {0};
	
	if(test_nb_flag)
	{
		strcpy(nb_test_info, "modem_configure");
		TestNBUpdateINfor();
	}

	if(nrf_modem_at_cmd(buf, sizeof(buf), "AT%CESQ=1") != 0)
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
			k_timer_start(&get_nw_rsrp_timer, K_MSEC(1000), K_NO_WAIT);
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

void NBGetNetMode(uint8_t *at_mode_set)
{
	uint8_t *p1,*p2;
	uint8_t tmpbuf[8] = {0};
	
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
	if((screen_id == SCREEN_ID_IDLE)
		||(screen_id == SCREEN_ID_HR)
		||(screen_id == SCREEN_ID_SPO2)
		||(screen_id == SCREEN_ID_BP)
		||(screen_id == SCREEN_ID_TEMP)
		)
	{
		scr_msg[screen_id].para |= SCREEN_EVENT_UPDATE_NET_MODE;
		scr_msg[screen_id].act = SCREEN_ACTION_UPDATE;
	}
}

void NBRedrawSignal(void)
{
	uint8_t *ptr;
	bool flag=false;
	uint8_t strbuf[128] = {0};
	uint8_t tmpbuf[128] = {0};

	if(nrf_modem_at_cmd(strbuf, sizeof(strbuf), CMD_GET_REG_STATUS) == 0)
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
		uint8_t *ptr;
		uint8_t reg_status;
		
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
					k_timer_start(&nb_reconnect_timer, K_SECONDS(10), K_NO_WAIT);
			}
		}
	}

	if(nrf_modem_at_cmd(strbuf, sizeof(strbuf), "AT+CSCON?") == 0)
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
			uint8_t mode;
			uint16_t len;

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

	if((screen_id == SCREEN_ID_IDLE)
		||(screen_id == SCREEN_ID_HR)
		||(screen_id == SCREEN_ID_SPO2)
		||(screen_id == SCREEN_ID_BP)
		||(screen_id == SCREEN_ID_TEMP)
		)
	{
		scr_msg[screen_id].para |= SCREEN_EVENT_UPDATE_SIG;
		scr_msg[screen_id].act = SCREEN_ACTION_UPDATE;
	}
}

void GetStringInforBySepa(uint8_t *sour_buf, uint8_t *key_buf, uint8_t pos_index, uint8_t *outbuf)
{
	uint8_t *ptr1,*ptr2;
	uint8_t count=0;
	
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
	uint8_t timebuf[128] = {0};
	uint8_t tmpbuf[10] = {0};
	uint8_t tz_dir[3] = {0};
	uint8_t tz_count,daylight;
	static uint8_t retry = 5;

	if(nrf_modem_at_cmd(timebuf, sizeof(timebuf), "AT+CCLK?") != 0)
	{
	#ifdef NB_DEBUG
		LOGD("Get CCLK fail!");
	#endif

	#ifndef NB_SIGNAL_TEST
		if(nb_connected && (retry > 0))
		{
			retry--;
			k_timer_start(&get_nw_time_timer, K_MSEC(1000), K_NO_WAIT);
		}
	#endif
		return;
	}

	retry = 5;

#ifdef NB_DEBUG	
	LOGD("%s", timebuf);
#endif
	//%CCLK: "21/02/26,08:31:33+32",0
	//+CCLK: "22/08/16,10:30:41+32"
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

		RedrawSystemTime();
		SaveSystemDateTime();
	}

#ifdef NB_DEBUG	
	LOGD("real time:%04d/%02d/%02d,%02d:%02d:%02d,%02d", 
					date_time.year,date_time.month,date_time.day,
					date_time.hour,date_time.minute,date_time.second,
					date_time.week);
#endif
}

static void MqttSendData(uint8_t *data, uint32_t datalen)
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

			k_timer_stop(&nb_reconnect_timer);
			k_timer_start(&nb_reconnect_timer, K_SECONDS(10), K_NO_WAIT);
		}
	}
}

void NBSendSettingReply(uint8_t *data, uint32_t datalen)
{
	uint8_t buf[256] = {0};
	uint8_t tmpbuf[32] = {0};

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

void NBSendSosAlarmData(uint8_t *data, uint32_t datalen)
{
	uint8_t buf[256] = {0};
	uint8_t tmpbuf[32] = {0};
	
	strcpy(buf, "{1:1:0:0:");
	strcat(buf, g_imei);
	strcat(buf, ":T0:");
	strcat(buf, data);
	strcat(buf, ",");
	GetSystemTimeSecString(tmpbuf);
	strcat(buf, tmpbuf);
	strcat(buf, "}");
#ifdef NB_DEBUG	
	LOGD("sos alarm data:%s", buf);
#endif
	MqttSendData(buf, strlen(buf));
}

void NBSendSosWifiData(uint8_t *data, uint32_t datalen)
{
	uint8_t buf[256] = {0};
	uint8_t tmpbuf[32] = {0};
	
	strcpy(buf, "{1:1:0:0:");
	strcat(buf, g_imei);
	strcat(buf, ":T1:");
	strcat(buf, data);
	strcat(buf, ",");
	GetSystemTimeSecString(tmpbuf);
	strcat(buf, tmpbuf);
	strcat(buf, "}");
#ifdef NB_DEBUG	
	LOGD("sos wifi data:%s", buf);
#endif
	MqttSendData(buf, strlen(buf));
}

void NBSendSosGpsData(uint8_t *data, uint32_t datalen)
{
	uint8_t buf[256] = {0};
	uint8_t tmpbuf[32] = {0};
	
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
void NBSendFallWifiData(uint8_t *data, uint32_t datalen)
{
	uint8_t buf[256] = {0};
	
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

void NBSendFallGpsData(uint8_t *data, uint32_t datalen)
{
	uint8_t buf[256] = {0};
	uint8_t tmpbuf[32] = {0};
	
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

void NBSendSingleHealthData(uint8_t *data, uint32_t datalen)
{
	uint8_t buf[256] = {0};
	uint8_t tmpbuf[32] = {0};
	
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

void NBSendTimelyHealthData(uint8_t *data, uint32_t datalen)
{
	uint8_t buf[1024] = {0};
	uint8_t tmpbuf[32] = {0};
	
	strcpy(buf, "{1:1:0:0:");
	strcat(buf, g_imei);
	strcat(buf, ":T14:");
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

void NBSendLocationData(uint8_t *data, uint32_t datalen)
{
	uint8_t buf[256] = {0};
	uint8_t tmpbuf[32] = {0};
	
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

void NBSendPowerOnInfor(uint8_t *data, uint32_t datalen)
{
	uint8_t buf[256] = {0};
	uint8_t tmpbuf[20] = {0};
	
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

void NBSendPowerOffInfor(uint8_t *data, uint32_t datalen)
{
	uint8_t buf[256] = {0};
	uint8_t tmpbuf[20] = {0};
	
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

bool GetParaFromString(uint8_t *rece, uint32_t rece_len, uint8_t *cmd, uint8_t *data)
{
	uint8_t *p1,*p2;
	uint32_t i=0;
	
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

void ParseData(uint8_t *data, uint32_t datalen)
{
	bool ret = false;
	uint8_t strcmd[10] = {0};
	uint8_t strdata[256] = {0};
	
	if(data == NULL || datalen == 0)
		return;

	ret = GetParaFromString(data, datalen, strcmd, strdata);
#ifdef NB_DEBUG
	LOGD("ret:%d, cmd:%s, data:%s", ret,strcmd,strdata);
#endif
	if(ret)
	{
		bool flag = false;
		
		if(strcmp(strcmd, "S7") == 0)
		{
			uint8_t *ptr,*ptr1;
			uint8_t strtmp[128] = {0};

			//后台下发定位上报间隔
			ptr = strstr(strdata, ",");
			if(ptr != NULL)
			{
				memcpy(strtmp, strdata, (ptr-strdata));
				global_settings.dot_interval.time = atoi(strtmp);

				ptr++;
				ptr1 = strstr(ptr, ",");
				if(ptr1 != NULL)
				{
					memset(strtmp, 0, sizeof(strtmp));
					memcpy(strtmp, ptr, ptr1-ptr);
					global_settings.dot_interval.steps = atoi(strtmp);

					ptr = ptr1+1;
					memset(strtmp, 0, sizeof(strtmp));
					strcpy(strtmp, ptr);
					global_settings.location_type = atoi(strtmp);
				}
			#ifdef NB_DEBUG
				LOGD("time:%d, steps:%d, location_type:%d", global_settings.dot_interval.time,global_settings.dot_interval.steps,global_settings.location_type);
			#endif	
			}

			flag = true;
		}
		else if(strcmp(strcmd, "S8") == 0)
		{
			//后台下发健康检测间隔
			global_settings.health_interval = atoi(strdata);

			flag = true;
		}
		else if(strcmp(strcmd, "S9") == 0)
		{
			//后台下发抬腕亮屏设置
			global_settings.wake_screen_by_wrist = atoi(strdata);
			
			flag = true;
		}
		else if(strcmp(strcmd, "S10") == 0)
		{
			//后台下发脱腕检测设置
			global_settings.wrist_off_check = atoi(strdata);

			flag = true;			
		}
		else if(strcmp(strcmd, "S11") == 0)
		{
			uint8_t *ptr;
			uint8_t strtmp[128] = {0};
			
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

			#ifdef CONFIG_PPG_SUPPORT
				sh_clear_bpt_cal_data();
				ppg_bpt_is_calbraed = false;
				ppg_bpt_cal_need_update = true;
			#endif
			}

			flag = true;
		}
		else if(strcmp(strcmd, "S12") == 0)
		{
			uint8_t *ptr,*ptr1;
			uint8_t strtmp[128] = {0};
			uint32_t copylen = 0;

			//后台下发最新版本信息
			//project dir
			ptr = strstr(strdata, ",");
			if(ptr == NULL)
				return;
			copylen = (ptr-strdata) < sizeof(g_prj_dir) ? (ptr-strdata) : sizeof(g_prj_dir);
			memcpy(g_prj_dir, strdata, copylen);

			//9160 fw ver
			ptr++;
			ptr1 = strstr(ptr, ",");
			if(ptr1 == NULL)
				return;
			copylen = (ptr1-ptr) < sizeof(g_new_fw_ver) ? (ptr1-ptr) : sizeof(g_new_fw_ver);
			memcpy(g_new_fw_ver, ptr, copylen);

			//9160 modem ver
			ptr = ptr1+1;
			ptr1 = strstr(ptr, ",");
			if(ptr1 == NULL)
				return;
			copylen = (ptr1-ptr) < sizeof(g_new_modem_ver) ? (ptr1-ptr) : sizeof(g_new_modem_ver);
			memcpy(g_new_modem_ver, ptr, copylen);

			//52810 fw ver
			ptr = ptr1+1;
			ptr1 = strstr(ptr, ",");
			if(ptr1 == NULL)
				return;
			copylen = (ptr1-ptr) < sizeof(g_new_ble_ver) ? (ptr1-ptr) : sizeof(g_new_ble_ver);
			memcpy(g_new_ble_ver, ptr, copylen);

			//ppg ver
			ptr = ptr1+1;
			ptr1 = strstr(ptr, ",");
			if(ptr1 == NULL)
				return;
			copylen = (ptr1-ptr) < sizeof(g_new_ppg_ver) ? (ptr1-ptr) : sizeof(g_new_ppg_ver);
			memcpy(g_new_ppg_ver, ptr, copylen);

			//wifi ver
			ptr = ptr1+1;
			ptr1 = strstr(ptr, ",");
			if(ptr1 == NULL)
				return;
			copylen = (ptr1-ptr) < sizeof(g_new_wifi_ver) ? (ptr1-ptr) : sizeof(g_new_wifi_ver);
			memcpy(g_new_wifi_ver, ptr, copylen);

			//ui ver
			ptr = ptr1+1;
			ptr1 = strstr(ptr, ",");
			if(ptr1 == NULL)
				return;
			copylen = (ptr1-ptr) < sizeof(g_new_ui_ver) ? (ptr1-ptr) : sizeof(g_new_ui_ver);
			memcpy(g_new_ui_ver, ptr, copylen);

			//font ver
			ptr = ptr1+1;
			ptr1 = strstr(ptr, ",");
			if(ptr1 == NULL)
				copylen = (datalen-(ptr-strdata)) < sizeof(g_new_font_ver) ? (datalen-(ptr-strdata)) : sizeof(g_new_font_ver);
			else
				copylen = (ptr1-ptr) < sizeof(g_new_font_ver) ? (ptr1-ptr) : sizeof(g_new_font_ver);
			memcpy(g_new_font_ver, ptr, copylen);
		}
		else if(strcmp(strcmd, "S15") == 0)
		{
			uint8_t *ptr,*ptr1;
			uint8_t tz_count,tz_dir,tz_buf[4] = {0};
			uint8_t date_buf[8] = {0};
			uint8_t time_buf[6] = {0};
			uint8_t tmpbuf[16] = {0};
			sys_date_timer_t tmp_dt = {0};
			uint32_t copylen = 0;

			//后台下发校时指令
			//timezone
			ptr = strstr(strdata, ",");
			if(ptr == NULL)
				return;
			copylen = (ptr-strdata) < sizeof(tz_buf) ? (ptr-strdata) : sizeof(tz_buf);
			memcpy(tz_buf, strdata, copylen);
			if(tz_buf[0] == '+' || tz_buf[0] == '-')
			{
				if(tz_buf[0] == '+')
					tz_dir = 1;
				else
					tz_dir = 0;

				tz_count = atoi(&tz_buf[1]);
			}
			else
			{
				tz_dir = 1;
				tz_count = atoi(tz_buf);
			}

		#ifdef NB_DEBUG
			LOGD("timezone:%c%d",  (tz_dir == 1 ? '+':'-'), tz_count);
		#endif

			//date
			ptr++;
			ptr1 = strstr(ptr, ",");
			if(ptr1 == NULL)
				return;
			copylen = (ptr1-ptr) < sizeof(date_buf) ? (ptr1-ptr) : sizeof(date_buf);
			memcpy(date_buf, ptr, copylen);
			memset(tmpbuf, 0, sizeof(tmpbuf));
			memcpy(tmpbuf, &date_buf[0], 4);
			tmp_dt.year = atoi(tmpbuf);
			memset(tmpbuf, 0, sizeof(tmpbuf));
			memcpy(tmpbuf, &date_buf[4], 2);
			tmp_dt.month = atoi(tmpbuf);
			memset(tmpbuf, 0, sizeof(tmpbuf));
			memcpy(tmpbuf, &date_buf[6], 2);
			tmp_dt.day = atoi(tmpbuf);

			//time
			ptr = ptr1+1;
			copylen = (datalen-(ptr-strdata)) < sizeof(time_buf) ? (datalen-(ptr-strdata)) : sizeof(time_buf);
			memcpy(time_buf, ptr, copylen);
			memset(tmpbuf, 0, sizeof(tmpbuf));
			memcpy(tmpbuf, &time_buf[0], 2);
			tmp_dt.hour = atoi(tmpbuf);
			memset(tmpbuf, 0, sizeof(tmpbuf));
			memcpy(tmpbuf, &time_buf[2], 2);
			tmp_dt.minute = atoi(tmpbuf);
			memset(tmpbuf, 0, sizeof(tmpbuf));
			memcpy(tmpbuf, &time_buf[4], 2);
			tmp_dt.second = atoi(tmpbuf);

			//if(tz_dir == 1)
			//{
			//	TimeIncrease(&tmp_dt, tz_count*15);
			//}
			//else
			//{
			//	TimeDecrease(&tmp_dt, tz_count*15);
			//}
			
		#ifdef NB_DEBUG
			LOGD("%04d%02d%02d %02d:%02d:%02d", tmp_dt.year,tmp_dt.month,tmp_dt.day,tmp_dt.hour,tmp_dt.minute,tmp_dt.second);
		#endif

			if(CheckSystemDateTimeIsValid(tmp_dt))
			{
				tmp_dt.week = GetWeekDayByDate(tmp_dt);
				memcpy(&date_time, &tmp_dt, sizeof(sys_date_timer_t));
				RedrawSystemTime();
				SaveSystemDateTime();

				server_has_timed_flag = true;
			}

			flag = true;			
		}
		else if(strcmp(strcmd, "S29") == 0)
		{
			//后台下发位置信息
		#ifdef NB_DEBUG
			LOGD("%s", strdata);
		#endif

			SOSRecLocatNotify(strdata);
		}

		SaveSystemSettings();

		if(flag)
		{			
			strcmd[0] = 'T';
			NBSendSettingReply(strcmd, strlen(strcmd));
		}
	}
}

static void ParseDataCallBack(struct k_timer *timer_id)
{
	parse_data_flag = true;
}

static void ParseReceData(void)
{
	uint8_t *p_data;
	uint32_t data_len;
	int ret;

	ret = get_data_from_rece_cache(&p_data, &data_len);
	if(ret)
	{
		ParseData(p_data, data_len);
		delete_data_from_rece_cache();
		
		k_timer_start(&parse_data_timer, K_MSEC(100), K_NO_WAIT);
	}
}

static void ParseReceDataStart(void)
{
	k_timer_start(&parse_data_timer, K_MSEC(500), K_NO_WAIT);
}

static void MqttReceData(uint8_t *data, uint32_t datalen)
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
	uint8_t buf[128] = {0};
	bool enable = IS_ENABLED(CONFIG_MODEM_AUTO_REQUEST_POWER_SAVING_FEATURES);

	err = lte_lc_psm_req(enable);
	if(err)
	{
	#ifdef NB_DEBUG
		LOGD("lte_lc_psm_req, error: %d", err);
	#endif
		return err;
	}

	if(enable)
	{
	#ifdef NB_DEBUG
		LOGD("PSM requested");
	#endif
	}
	else
	{
	#ifdef NB_DEBUG
		LOGD("PSM disabled");
	#endif
	}

	if(nrf_modem_at_cmd(buf, sizeof(buf), "AT+CPSMS?") == 0)
	{
	#ifdef NB_DEBUG
		LOGD("PSM:%s", buf);
	#endif
	}

	return 0;
}

void GetModemSignal(void)
{
	char *ptr1,*ptr2;
	int i=0,len;
	uint8_t strbuf[64] = {0};
	uint8_t tmpbuf[64] = {0};
	int32_t rsrq=0,rsrp,snr;
	static int32_t rsrpbk = 0;
	
	if(nrf_modem_at_cmd(tmpbuf, sizeof(tmpbuf), CMD_GET_CESQ) == 0)
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

	if(nrf_modem_at_cmd(tmpbuf, sizeof(tmpbuf), CMD_GET_SNR) == 0)
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

void DecodeModemMonitor(uint8_t *buf, uint32_t len)
{
	uint8_t reg_status = 0;
	uint8_t *ptr;
	uint8_t tmpbuf[128] = {0};
	uint8_t strbuf[128] = {0};

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
			uint8_t tau = 0;
			uint8_t act = 0;
			uint16_t flag;

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

void SetNetWorkApn(uint8_t *imsi_buf)
{
	uint32_t i;
	uint8_t tmpbuf[256] = {0};

	for(i=0;i<ARRAY_SIZE(nb_apn_table);i++)
	{
		if(strncmp(imsi_buf, nb_apn_table[i].plmn, strlen(nb_apn_table[i].plmn)) == 0)
		{
			uint8_t cmdbuf[128] = {0};
			
			sprintf(cmdbuf, "AT+CGDCONT=0,\"IP\",\"%s\"", nb_apn_table[i].apn);
		#ifdef NB_DEBUG
			LOGD("cmdbuf:%s", cmdbuf); 
		#endif
			if(nrf_modem_at_cmd(tmpbuf, sizeof(tmpbuf), cmdbuf) != 0)
			{
			#ifdef NB_DEBUG
				LOGD("set apn fail!");
			#endif
			}

			break;
		}
	}

	if(nrf_modem_at_cmd(tmpbuf, sizeof(tmpbuf), CMD_GET_APN) == 0)
	{
	#ifdef NB_DEBUG
		LOGD("apn:%s", tmpbuf); 
	#endif
	}
}

void SetNwtWorkMqttBroker(uint8_t *imsi_buf)
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

void SetNetWorkParaByPlmn(uint8_t *imsi)
{
	SetNetWorkApn(imsi);
	SetNwtWorkMqttBroker(imsi);
}

void GetModemInfor(void)
{
	uint8_t tmpbuf[256] = {0};

	if(nrf_modem_at_cmd(tmpbuf, sizeof(tmpbuf), CMD_GET_MODEM_V) == 0)
	{
		strncpy(g_modem, &tmpbuf, MODEM_MAX_LEN);
	#ifdef NB_DEBUG
		LOGD("MODEM version:%s", g_modem);
	#endif	
	}

#if 0
	if(nrf_modem_at_cmd(tmpbuf, sizeof(tmpbuf), CMD_GET_SUPPORT_BAND) == 0)
	{
	#ifdef NB_DEBUG
		LOGD("support band:%s", tmpbuf);
	#endif
	}

	if(nrf_modem_at_cmd(tmpbuf, sizeof(tmpbuf), CMD_GET_CUR_BAND) == 0)
	{
	#ifdef NB_DEBUG
		LOGD("current band:%s", tmpbuf);
	#endif
	}

	if(nrf_modem_at_cmd(tmpbuf, sizeof(tmpbuf), CMD_GET_LOCKED_BAND) == 0)
	{
	#ifdef NB_DEBUG
		LOGD("locked band:%s", tmpbuf);
	#endif
	}
#endif

	if(nrf_modem_at_cmd(tmpbuf, sizeof(tmpbuf), CMD_GET_IMEI) == 0)
	{
		strncpy(g_imei, tmpbuf, IMEI_MAX_LEN);
	#ifdef NB_DEBUG
		LOGD("imei:%s", g_imei);
	#endif	
	}

	if(nrf_modem_at_cmd(tmpbuf, sizeof(tmpbuf), CMD_GET_IMSI) == 0)
	{
		strncpy(g_imsi, tmpbuf, IMSI_MAX_LEN);
	#ifdef NB_DEBUG
		LOGD("imsi:%s", g_imsi);
	#endif
	}

	if(nrf_modem_at_cmd(tmpbuf, sizeof(tmpbuf), CMD_GET_ICCID) == 0)
	{
		strncpy(g_iccid, &tmpbuf[9], ICCID_MAX_LEN);
	#ifdef NB_DEBUG
		LOGD("iccid:%s", g_iccid);
	#endif
	}
}

void GetModemStatus(void)
{
	char *ptr;
	int i=0,len,err;
	uint8_t strbuf[64] = {0};
	uint8_t tmpbuf[256] = {0};

	if(nrf_modem_at_cmd(tmpbuf, sizeof(tmpbuf), CMD_GET_MODEM_PARA) == 0)
	{
	#ifdef NB_DEBUG
		LOGD("MODEM parameter:%s", &tmpbuf);
	#endif
		DecodeModemMonitor(tmpbuf, strlen(tmpbuf));
	}
}

void SetModemTurnOn(void)
{
	uint8_t buf[128] = {0};
	
	if(nrf_modem_at_cmd(buf, sizeof(buf), "AT+CFUN=1") == 0)
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
	uint8_t buf[128] = {0};

	if(nrf_modem_at_cmd(buf, sizeof(buf), "AT+CFUN=4") == 0)
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

void SetModemGps(void)
{
	uint8_t buf[128] = {0};

	if(nrf_modem_at_cmd(buf, sizeof(buf), CMD_SET_NW_MODE_GPS) == 0)
	{
	#ifdef NB_DEBUG
		LOGD("set modem for gps success!");
	#endif
	}
	else
	{
	#ifdef NB_DEBUG
		LOGD("set modem for gps fail!");
	#endif
	}
}

void GetModemNw(void)
{
	int err;
	enum lte_lc_system_mode mode;

	err = lte_lc_system_mode_get(&mode, NULL);
	if(err)
	{
		return;
	}

	switch(mode)
	{
	case LTE_LC_SYSTEM_MODE_LTEM:
	case LTE_LC_SYSTEM_MODE_LTEM_GPS:
		g_net_mode = NET_MODE_LTE_M;
		break;
		
	case LTE_LC_SYSTEM_MODE_NBIOT:
	case LTE_LC_SYSTEM_MODE_NBIOT_GPS:
		g_net_mode = NET_MODE_NB;
		break;
		
	default:
		g_net_mode = NET_MODE_MAX;
		break;
	}
}

void SetModemNw(void)
{
	uint8_t buf[128] = {0};

#if defined(CONFIG_LTE_NETWORK_MODE_NBIOT)
	if(nrf_modem_at_cmd(buf, sizeof(buf), CMD_SET_NW_MODE_NB) == 0)
#elif defined(CONFIG_LTE_NETWORK_MODE_NBIOT_GPS)
	if(nrf_modem_at_cmd(buf, sizeof(buf), CMD_SET_NW_MODE_NB_GPS) == 0)
#elif defined(CONFIG_LTE_NETWORK_MODE_LTE_M)
	if(nrf_modem_at_cmd(buf, sizeof(buf), CMD_SET_NW_MODE_LTE_M) == 0)
#elif defined(CONFIG_LTE_NETWORK_MODE_LTE_M_GPS)
	if(nrf_modem_at_cmd(buf, sizeof(buf), CMD_SET_NW_MODE_LTE_M_GPS) == 0)
#endif
	{
	#ifdef NB_DEBUG
		LOGD("set modem for nw success!");
	#endif
	}
	else
	{
	#ifdef NB_DEBUG
		LOGD("set modem for nw fail!");
	#endif
	}
}

void GetModemAPN(void)
{
	uint8_t tmpbuf[128] = {0};
	
	if(nrf_modem_at_cmd(tmpbuf, sizeof(tmpbuf), CMD_GET_APN) == 0)
	{
	#ifdef NB_DEBUG
		LOGD("apn:%s", tmpbuf);	
	#endif
	}
}

void SetModemAPN(void)
{
	uint8_t tmpbuf[128] = {0};
	
	if(nrf_modem_at_cmd(tmpbuf, sizeof(tmpbuf), "AT+CGDCONT=0,\"IP\",\"arkessalp.com\"") != 0)
	{
	#ifdef NB_DEBUG
		LOGD("set apn fail!");
	#endif
	}

	if(nrf_modem_at_cmd(tmpbuf, sizeof(tmpbuf), CMD_GET_APN) != 0)
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

	k_delayed_work_submit_to_queue(app_work_q, &nb_link_work, K_SECONDS(1));
}

static void nb_link(struct k_work *work)
{
	int err=0;
	uint8_t tmpbuf[128] = {0};
	static uint32_t retry_count = 0;
	static bool frist_flag = false;

	if(!frist_flag)
	{
		frist_flag = true;
		GetModemInfor();
		SetModemTurnOff();
		SetNetWorkParaByPlmn(g_imsi);
	}
	else
	{
		SetModemTurnOff();
	}

#ifndef NB_SIGNAL_TEST
	if(strlen(g_imsi) == 0)
	{
	#ifdef NB_DEBUG
		LOGD("Can't get sim info, cancel the connecting!");
	#endif
		return;
	}
#endif

	if(gps_is_working())
	{
	#ifdef NB_DEBUG
		LOGD("gps is working, continue waiting!");
	#endif
	
		if(retry_count <= 2)		//2次以内每1分钟重连一次
			k_timer_start(&nb_reconnect_timer, K_SECONDS(60), K_NO_WAIT);
		else if(retry_count <= 4)	//3到4次每5分钟重连一次
			k_timer_start(&nb_reconnect_timer, K_SECONDS(300), K_NO_WAIT);
		else if(retry_count <= 6)	//5到6次每10分钟重连一次
			k_timer_start(&nb_reconnect_timer, K_SECONDS(600), K_NO_WAIT);
		else if(retry_count <= 8)	//7到8次每1小时重连一次
			k_timer_start(&nb_reconnect_timer, K_SECONDS(3600), K_NO_WAIT);
		else						//8次以上每6小时重连一次
			k_timer_start(&nb_reconnect_timer, K_SECONDS(6*3600), K_NO_WAIT);	
	}
	else
	{
	#ifdef NB_DEBUG
		LOGD("linking");
	#endif
	
		nb_connecting_flag = true;
		configure_low_power();
		err = lte_lc_init_and_connect();
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
				k_timer_start(&nb_reconnect_timer, K_SECONDS(60), K_NO_WAIT);
			else if(retry_count <= 4)	//3到4次每5分钟重连一次
				k_timer_start(&nb_reconnect_timer, K_SECONDS(300), K_NO_WAIT);
			else if(retry_count <= 6)	//5到6次每10分钟重连一次
				k_timer_start(&nb_reconnect_timer, K_SECONDS(600), K_NO_WAIT);
			else if(retry_count <= 8)	//7到8次每1小时重连一次
				k_timer_start(&nb_reconnect_timer, K_SECONDS(3600), K_NO_WAIT);
			else						//8次以上每6小时重连一次
				k_timer_start(&nb_reconnect_timer, K_SECONDS(6*3600), K_NO_WAIT);	
		}
		else
		{
		#ifdef NB_DEBUG
			LOGD("Connected to LTE network");
		#endif

			GetModemNw();

			if(test_nb_flag)
			{
				strcpy(nb_test_info, "LTE Link Connected!");
				nb_test_update_flag = true;
				k_timer_start(&get_nw_rsrp_timer, K_MSEC(1000), K_NO_WAIT);
			}
			
			nb_connected = true;
			retry_count = 0;
		#ifdef CONFIG_MODEM_INFO
			modem_data_init();
		#endif

			if(!server_has_timed_flag)
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
	uint8_t str_rsrp[128] = {0};

	if(nrf_modem_at_cmd(str_rsrp, sizeof(str_rsrp), "AT+CFUN?") == 0)
	{
	#ifdef NB_DEBUG	
		LOGD("cfun:%s", str_rsrp);
	#endif
	}

	if(nrf_modem_at_cmd(str_rsrp, sizeof(str_rsrp), "AT+CPSMS?") == 0)
	{
	#ifdef NB_DEBUG
		LOGD("cpsms:%s", str_rsrp);
	#endif
	}
	
	if(nrf_modem_at_cmd(str_rsrp, sizeof(str_rsrp), CMD_GET_CESQ) == 0)
	{
	#ifdef NB_DEBUG	
		LOGD("cesq:%s", str_rsrp);
	#endif
	}

    if(nrf_modem_at_cmd(str_rsrp, sizeof(str_rsrp), CMD_GET_APN) == 0)
	{
	#ifdef NB_DEBUG	
		LOGD("apn:%s", str_rsrp);
	#endif
	}
	
	if(nrf_modem_at_cmd(str_rsrp, sizeof(str_rsrp), CMD_GET_CSQ) == 0)
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

bool nb_reconnect(void)
{
	if(k_timer_remaining_get(&nb_reconnect_timer) > 0)
		k_timer_stop(&nb_reconnect_timer);
	
	nb_reconnect_flag = true;
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
			strcpy(nb_test_info, "LTE Link Connected!");
			TestNBUpdateINfor();
			k_timer_start(&get_nw_rsrp_timer, K_MSEC(1000), K_NO_WAIT);
		}
		else if(nb_is_connecting())
		{
			strcpy(nb_test_info, "LTE Link Connecting ...");
			TestNBUpdateINfor();
		}
		else
		{
			SetModemTurnOff();
			configure_low_power();
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
		if(screen_id == SCREEN_ID_NB_TEST)
			k_timer_start(&get_nw_rsrp_timer, K_MSEC(1000), K_NO_WAIT);
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
		if(send_cache_is_empty())
		{
		#ifdef NB_DEBUG
			LOGD("001");
		#endif
			MqttDisConnect();
		}
		else
		{
		#ifdef NB_DEBUG
			LOGD("002");
		#endif
			k_timer_start(&mqtt_disconnect_timer, K_SECONDS(5), K_NO_WAIT);
		}
		
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
		
		if(test_gps_flag || nb_connected)
			return;
	
		if(!nb_connecting_flag)
		{
			k_delayed_work_submit_to_queue(app_work_q, &nb_link_work, K_SECONDS(2));
		}
		else
		{
			k_timer_start(&nb_reconnect_timer, K_SECONDS(10), K_NO_WAIT);
		}
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

	k_delayed_work_submit_to_queue(app_work_q, &modem_init_work, K_SECONDS(2));
}
