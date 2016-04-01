/*
 * WINC_mega168.c
 *
 * Created: 13.03.2015 14:33:29
 *  Author: mebskjoensfjell
 */ 
#include "main.h"
#include <avr/eeprom.h>

/** Sockets for TCP and UDP communication */
static SOCKET tcp_server_socket = -1;
static SOCKET tcp_client_socket = -1;
static SOCKET udp_socket = -1;

/** Wi-Fi connection state */
static volatile uint8_t wifi_connected;

/** Variable for main loop to see if a toggle request is received */
volatile bool toggle = false;

/** Receive buffer definition. */
static char gau8SocketTestBuffer[MAIN_WIFI_M2M_BUFFER_SIZE];

/** Buffer for wifi password */
uint8_t password[MAX_LEN];
/** Buffer for wifi SSID*/
uint8_t ssid[MAX_LEN];
/** Buffer for wifi security */
uint8_t security[2];
/** Variable to store status of provisioning mode  */
uint8_t provision = true;

/**
 * \brief Callback to get the Data from socket.
 *
 * \param[in] sock socket handler.
 * \param[in] u8Msg socket event type. Possible values are:
 *  - SOCKET_MSG_BIND
 *  - SOCKET_MSG_LISTEN
 *  - SOCKET_MSG_ACCEPT
 *  - SOCKET_MSG_CONNECT
 *  - SOCKET_MSG_RECV
 *  - SOCKET_MSG_SEND
 *  - SOCKET_MSG_SENDTO
 *  - SOCKET_MSG_RECVFROM
 * \param[in] pvMsg is a pointer to message structure. Existing types are:
 *  - tstrSocketBindMsg
 *  - tstrSocketListenMsg
 *  - tstrSocketAcceptMsg
 *  - tstrSocketConnectMsg
 *  - tstrSocketRecvMsg
 */
void m2m_tcp_socket_handler(SOCKET sock, uint8_t u8Msg, void *pvMsg)
{
	switch (u8Msg) {
	/* Socket bind */
	case SOCKET_MSG_BIND:
	{
		tstrSocketBindMsg *pstrBind = (tstrSocketBindMsg *)pvMsg;
		if (pstrBind && pstrBind->status == 0) {
			listen(tcp_server_socket, 0);
		}
	}
	break;

	/* Connect accept */
	case SOCKET_MSG_ACCEPT:
	{
		memset(gau8SocketTestBuffer, 0, sizeof(gau8SocketTestBuffer));
		tstrSocketAcceptMsg *pstrAccept = (tstrSocketAcceptMsg *)pvMsg;
		if (pstrAccept) {
			tcp_client_socket = pstrAccept->sock;
			recv(tcp_client_socket, gau8SocketTestBuffer, sizeof(gau8SocketTestBuffer), 0);			
		}
	}
	break;

	/* Message sent */
	case SOCKET_MSG_SEND:
		memset(gau8SocketTestBuffer, 0, sizeof(gau8SocketTestBuffer));
		close(tcp_client_socket);
		break;
	/* Message receive */
	case SOCKET_MSG_RECV:
		/* Check command */
		if (strstr(gau8SocketTestBuffer, "toggle")) {
			toggle = true;
		}
		break;
	default:
		break;
	}
}

/**
 * \brief Callback to get the Wi-Fi status update.
 *
 * \param[in] u8MsgType type of Wi-Fi notification. Possible types are:
 *  - [M2M_WIFI_RESP_CURRENT_RSSI](@ref M2M_WIFI_RESP_CURRENT_RSSI)
 *  - [M2M_WIFI_RESP_CON_STATE_CHANGED](@ref M2M_WIFI_RESP_CON_STATE_CHANGED)
 *  - [M2M_WIFI_RESP_CONNTION_STATE](@ref M2M_WIFI_RESP_CONNTION_STATE)
 *  - [M2M_WIFI_RESP_SCAN_DONE](@ref M2M_WIFI_RESP_SCAN_DONE)
 *  - [M2M_WIFI_RESP_SCAN_RESULT](@ref M2M_WIFI_RESP_SCAN_RESULT)
 *  - [M2M_WIFI_REQ_WPS](@ref M2M_WIFI_REQ_WPS)
 *  - [M2M_WIFI_RESP_IP_CONFIGURED](@ref M2M_WIFI_RESP_IP_CONFIGURED)
 *  - [M2M_WIFI_RESP_IP_CONFLICT](@ref M2M_WIFI_RESP_IP_CONFLICT)
 *  - [M2M_WIFI_RESP_P2P](@ref M2M_WIFI_RESP_P2P)
 *  - [M2M_WIFI_RESP_AP](@ref M2M_WIFI_RESP_AP)
 *  - [M2M_WIFI_RESP_CLIENT_INFO](@ref M2M_WIFI_RESP_CLIENT_INFO)
 * \param[in] pvMsg A pointer to a buffer containing the notification parameters
 * (if any). It should be casted to the correct data type corresponding to the
 * notification type. Existing types are:
 *  - tstrM2mWifiStateChanged
 *  - tstrM2MWPSInfo
 *  - tstrM2MP2pResp
 *  - tstrM2MAPResp
 *  - tstrM2mScanDone
 *  - tstrM2mWifiscanResult
 */
static void wifi_cb(uint8_t u8MsgType, void *pvMsg)
{
	switch (u8MsgType) {
	case M2M_WIFI_RESP_CON_STATE_CHANGED:
	{
		tstrM2mWifiStateChanged *pstrWifiState = (tstrM2mWifiStateChanged *)pvMsg;
		if (pstrWifiState->u8CurrState == M2M_WIFI_DISCONNECTED) {
			wifi_connected = 0;
			/* Reconnect */
			m2m_wifi_connect((char*)ssid, strlen((char*)ssid), atoi((char*)security), password, M2M_WIFI_CH_ALL);
		}
	}
	break;
	case M2M_WIFI_REQ_DHCP_CONF:
	{
		/* Connected and gotten IP address*/
		wifi_connected = M2M_WIFI_CONNECTED;
	}
	break;
	
	case M2M_WIFI_RESP_PROVISION_INFO:
	{
		tstrM2MProvisionInfo *pstrProvInfo = (tstrM2MProvisionInfo*)pvMsg;
		if (pstrProvInfo->u8Status == M2M_SUCCESS) {
			memcpy(ssid, pstrProvInfo->au8SSID, strlen((char*)(pstrProvInfo->au8SSID)));
			memcpy(password, pstrProvInfo->au8Password, strlen((char*)(pstrProvInfo->au8Password)));
			itoa(pstrProvInfo->u8SecType, (char*)security, 10);
			provision = true;
			wifi_connected = false;			
		}
	}
	default:
		break;
	}
}

/**
 * \brief Timer1 Compare A interrupt routine
 */
ISR(TIMER1_COMPA_vect)
{
	timer_int = true;
}

/**
 * \brief Initialize timer to trigger every 4 seconds
 */
void init_timer(void)
{
	sysclk_enable_module(POWER_RED_REG0, PRTIM1_bm);	
	/* Clock prescaled 1024, TOP OCR1A, no buffering */
	TCCR1B = (1 << CS12) | (0 << CS11) | (1 << CS10) | (0 << WGM13) | (1 << WGM12);
	/* Set top value 4 s */
	OCR1A = 0x7A10;
	/* Restart timer */
	TCNT1 = 0;
	/* Clear any interrupt flags */
	TIFR1 = 0xff;
	/* Enable interrupt */
	TIMSK1 = (1 << OCIE1A);
}

/**
 * \brief Read WiFi SSID and password from EEPROM
 *
 * \param ssidbuf     buffer for SSID
 * \param passwordbuf buffer for password
 * \param sec         pointer to security type
 */
static void get_wifi_credentials_from_eeprom(uint8_t *ssidbuf, uint8_t *passwordbuf, uint8_t *sec)
{
	uint8_t i = MAX_LEN;
	uint8_t *addr = EEPROM_SSID;
	while (i--) {
		*ssidbuf++ = eeprom_read_byte((uint8_t *)addr++);
	}
	i = MAX_LEN;
	addr = (uint8_t *)EEPROM_PSW;
	while (i--) {
		*passwordbuf++ = eeprom_read_byte((uint8_t *)addr++);
	}	
	*sec = eeprom_read_byte((uint8_t *)EEPROM_SEC);
}

/**
 * \brief Store WiFi SSID and password to EEPROM
 *
 * \param ssidbuf     buffer with SSID
 * \param passwordbuf buffer with password
 * \param sec         pointer to security type
 */
static void save_wifi_credentials_to_eeprom(uint8_t *ssidbuf, uint8_t *passwordbuf, uint8_t *sec)
{
	uint8_t i = MAX_LEN;
	uint8_t *addr = EEPROM_SSID;
	while (*ssidbuf && i--) {
		eeprom_write_byte((uint8_t *)addr++, *ssidbuf++);
	}
	if (i) {
		while (i--) eeprom_write_byte((uint8_t *)addr++, 0);
	}
	i = MAX_LEN;
	addr = (uint8_t *)EEPROM_PSW;
	while (*passwordbuf && i--) {
		eeprom_write_byte((uint8_t *)addr++, (uint8_t)*passwordbuf++);
	}
	if (i) {
		while (i--) eeprom_write_byte((uint8_t *)addr++, 0);
	}	
	eeprom_write_byte((uint8_t *)EEPROM_SEC, (uint8_t)*sec);
}

/**
 * \brief Initializes pin to toggle, LED and button
 */
static void init_pins(void)
{
	ioport_configure_pin(TOGGLE_PIN, IOPORT_INIT_HIGH | IOPORT_DIR_OUTPUT);
	ioport_configure_pin(LED_PIN, IOPORT_INIT_HIGH | IOPORT_DIR_OUTPUT);
	ioport_configure_pin(BUTTON_PIN, IOPORT_DIR_INPUT);
}

/**
 * \brief Main function of demo application
 */
int main(void)
{
	struct sockaddr_in addr;
	tstrWifiInitParam param;
	/* Callback for wifi */
	param.pfAppWifiCb = wifi_cb;
	/** Key buffer - must match with Android app */
	uint8_t key_buf[] = KEY_BUFFER;

	sysclk_init();
	cpu_irq_enable();

	init_pins();

	/* Initialize the BSP. */
	nm_bsp_init();
	
	/* Initialize WINC1500 Wi-Fi driver with data and status callbacks. */
	if (M2M_SUCCESS != m2m_wifi_init(&param)) {
		while (1);
	}

	if (ioport_get_pin_level(BUTTON_PIN)) {
		/* Read credentials from eeprom	*/
		get_wifi_credentials_from_eeprom(ssid, password, security);
		provision = false;
		if (ssid[0] == 0xff) {
			/* SSID is empty */
			memset(ssid, 0, sizeof(ssid));
			memset(password, 0, sizeof(password));
			memset(security, 0, sizeof(security));
			provision = true;	
		}
	}
	if (provision) {
		provision = false;
		/* Configure and enable access point mode with provisioning page. */
		tstrM2MAPConfig ap_config = {
			DEMO_WLAN_AP_NAME, // Access Point Name.
			DEMO_WLAN_AP_CHANNEL, // Channel to use.
			DEMO_WLAN_AP_WEP_INDEX, // Wep key index.
			DEMO_WLAN_AP_WEP_SIZE, // Wep key size.
			DEMO_WLAN_AP_WEP_KEY, // Wep key.
			DEMO_WLAN_AP_SECURITY, // Security mode.
			DEMO_WLAN_AP_MODE, // SSID visible.
			DEMO_WLAN_AP_IP_ADDRESS // DHCP server IP
		};
		static CONST char gacHttpProvDomainName[] = DEMO_WLAN_AP_DOMAIN_NAME;
		m2m_wifi_start_provision_mode((tstrM2MAPConfig *)&ap_config, (char*)gacHttpProvDomainName, 1);
		ioport_set_pin_low(LED_PIN);

		while(!provision) {
			/* Handle pending events from network controller while waiting for credentials */
			m2m_wifi_handle_events(NULL);
		}
		save_wifi_credentials_to_eeprom(ssid, password, security);
	}
	ioport_set_pin_high(LED_PIN);
	
	init_timer();

	/* Initialize socket module */
	socketInit();
	registerSocketCallback(m2m_tcp_socket_handler, NULL);

	/* Connect to router. */
	m2m_wifi_connect((char*)ssid, strlen((char*)ssid), atoi((char*)security),(char*)password, M2M_WIFI_CH_ALL);

	while (1) {
		/* Handle pending events from network controller. */
		m2m_wifi_handle_events(NULL);

		if (wifi_connected == M2M_WIFI_CONNECTED) {
			if (tcp_server_socket < 0) {
					/* Open TCP server socket */
					if ((tcp_server_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
						continue;
					}
					/* Initialize socket address structure. */
					addr.sin_family = AF_INET;
					addr.sin_port = _htons(TCP_PORT);
					addr.sin_addr.s_addr = 0;
					/* Bind service*/
					bind(tcp_server_socket, (struct sockaddr *)&addr, sizeof(struct sockaddr_in));
					continue;
			}

			if (timer_int) {
				/* Time to send UDP broadcast */
				if (udp_socket < 0) {
					udp_socket = socket(AF_INET, SOCK_DGRAM, 0);
					continue;
				}
				timer_int = false;
				/* Initialize socket address structure. */
				addr.sin_family	= AF_INET;
				addr.sin_port = _htons(UDP_PORT);
				/* Broadcast address 255.255.255.255 */
				addr.sin_addr.s_addr = 0xFFFFFFFF;
				sendto(udp_socket, (void*)key_buf, sizeof(key_buf), 0,
						(struct sockaddr *)&addr, sizeof(addr));
				continue;
			}

			/* Toggle command received  */
			if (toggle) {
				toggle = false;
				ioport_toggle_pin_level(TOGGLE_PIN);
				ioport_toggle_pin_level(LED_PIN);					
				/* Sending key buffer back as ack of toggling pin */
				send(tcp_client_socket, (void*)key_buf, sizeof(key_buf), 0);
			}
		}
	}
}