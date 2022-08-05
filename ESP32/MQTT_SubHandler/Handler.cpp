/*
 * Handler.cpp
 *
 *  Created on: 3 Feb 2019
 *      Author: xasin
 */

#include "xasin/mqtt/Handler.h"
#include "xasin/mqtt/Subscription.h"

#include "esp_wpa2.h"

#include "nvs.h"
#include "nvs_flash.h"

#define LOG_LOCAL_LEVEL ESP_LOG_INFO
#include "esp_log.h"

namespace Xasin {
namespace MQTT {

const char *mqtt_tag = "XAQTT";

bool wifi_was_configured = false;
volatile int  wifi_task_conn_counter = 0;
TaskHandle_t wifi_task_handle = nullptr;
void handler_wifi_checkup_task(void *eh) {
	while(true) {
		xTaskNotifyWait(0, 0, nullptr, portMAX_DELAY);

		if(wifi_task_conn_counter == -1)
			continue;

		int delay = 1000;
		if(wifi_task_conn_counter < 2)
			delay = 2000;
		else if(wifi_task_conn_counter < 4)
			delay = 10000;
		else
			delay = 30000;

		vTaskDelay(delay/portTICK_PERIOD_MS);

		if(wifi_task_conn_counter == -1)
			continue;
		ESP_LOGI("XAQTT:WiFi", "Retrying connection...");
		sleep(1);
		esp_wifi_start();
	}
}

bool Handler::start_wifi_from_nvs() {
	nvs_handle_t read_handle;

	char ssid_buffer[64] = {};
	size_t ssid_len = 64;
	char pswd_buffer[64] = {};

	nvs_open("xasin", NVS_READONLY, &read_handle);
	auto ret = nvs_get_str(read_handle, "w_ssid", ssid_buffer, &ssid_len);

	if(ret != ESP_OK) {
		nvs_close(read_handle);
		return false;
	}

	ssid_len = 64;
	ret = nvs_get_str(read_handle, "w_pswd", pswd_buffer, &ssid_len);
	nvs_close(read_handle);

	if(ret != ESP_OK)
		return false;

	sleep(1);
	start_wifi(ssid_buffer, pswd_buffer);

	return true;
}

void Handler::set_nvs_wifi(const char *wifi_ssid, const char *wifi_pswd) {
	nvs_handle_t write_handle;

	nvs_open("xasin", NVS_READWRITE, &write_handle);

	if(wifi_ssid != nullptr && strlen(wifi_ssid) < 100)
		nvs_set_str(write_handle, "w_ssid", wifi_ssid);
	if(wifi_pswd != nullptr && strlen(wifi_pswd) < 100)
		nvs_set_str(write_handle, "w_pswd", wifi_pswd);

	nvs_commit(write_handle);
	nvs_close(write_handle);
}

void Handler::start_wifi(const char *SSID, const char *PSWD, int psMode) {
	if(!wifi_was_configured) {
		wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();

		ESP_ERROR_CHECK( esp_wifi_init(&cfg) );
		ESP_ERROR_CHECK( esp_wifi_set_storage(WIFI_STORAGE_RAM) );
		ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_STA) );

		xTaskCreate(handler_wifi_checkup_task, "XAQTT::Wifi", 2*1024, nullptr, 10, &wifi_task_handle);
	}

	wifi_was_configured = true;

	wifi_config_t wifi_cfg = {};
	wifi_sta_config_t* sta_cfg = &(wifi_cfg.sta);

	memcpy(sta_cfg->password, PSWD, strlen(PSWD));
	memcpy(sta_cfg->ssid, SSID, strlen(SSID));

	if(psMode >= 2) {
			esp_wifi_set_max_tx_power(50);
			sta_cfg->listen_interval = 5;
	}
	ESP_ERROR_CHECK( esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_cfg) );
	if(psMode >= 1) {
		ESP_ERROR_CHECK( esp_wifi_set_ps(WIFI_PS_MAX_MODEM));
	}
	else if(psMode == -1) {
		ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_NONE));
	}
	sleep(1);
	ESP_ERROR_CHECK( esp_wifi_start() );
}

//void Handler::start_wifi_enterprise(const char *SSID, const char *domain, const char *identity, const char *anonymousIdentity, const char *password) {
//	if(!wifi_was_configured) {
//		wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
//
//		ESP_ERROR_CHECK( esp_wifi_init(&cfg) );
//		ESP_ERROR_CHECK( esp_wifi_set_storage(WIFI_STORAGE_RAM) );
//		ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_STA) );
//
//		xTaskCreate(handler_wifi_checkup_task, "XAQTT::Wifi", 2*1024, nullptr, 10, &wifi_task_handle);
//	}
//
//	wifi_was_configured = true;
//
//	wifi_config_t wifi_cfg = {};
//	memcpy(&wifi_cfg.sta.ssid, SSID, strlen(SSID));
//	esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_cfg);
//
//	esp_wpa2_config_t wpaCFG = WPA2_CONFIG_INIT_DEFAULT();
//
//	esp_wifi_sta_wpa2_ent_set_identity(reinterpret_cast<const unsigned char *>(anonymousIdentity)
//			, strlen(anonymousIdentity));
//	esp_wifi_sta_wpa2_ent_set_username(reinterpret_cast<const unsigned char *>(identity)
//			, strlen(identity));
//	esp_wifi_sta_wpa2_ent_set_password(reinterpret_cast<const unsigned char *>(password)
//			, strlen(password));
//
//	esp_log_level_set("wpa", ESP_LOG_DEBUG);
//
//	ESP_ERROR_CHECK(esp_wifi_sta_wpa2_ent_set_ca_cert(TeleSec_crt, TeleSec_crt_end - TeleSec_crt));
//
//	const char *keyPassword = "\0";
//
//	//ESP_ERROR_CHECK(esp_wifi_sta_wpa2_ent_set_cert_key(wifi_cert, wifi_cert_end - wifi_cert,
//	//			wifi_cert_key, wifi_cert_key_end - wifi_cert_key,
//	//			reinterpret_cast<const unsigned char*>(keyPassword), 0));
//
//	esp_wifi_sta_wpa2_ent_enable(&wpaCFG);
//
//	esp_wifi_start();
//}

void Handler::try_wifi_reconnect(system_event_t *event) {
	switch(event->event_id) {
	case SYSTEM_EVENT_STA_CONNECTED:
		ESP_LOGI("XAQTT:WiFi", "Reconnected");
		wifi_task_conn_counter = -1;
		break;
	case SYSTEM_EVENT_STA_START:
		sleep(1);
		esp_wifi_connect();
		break;
	case SYSTEM_EVENT_STA_DISCONNECTED:
		ESP_LOGW("XAQTT::WiFi", "Disconnected");

		sleep(1);
		esp_wifi_stop();
		wifi_task_conn_counter++;
		xTaskNotify(wifi_task_handle, 0, eNoAction);
	break;
	default: break;
	}
}


esp_err_t mqtt_handle_caller(esp_mqtt_event_t *event) {
	reinterpret_cast<Handler*>(event->user_context)->mqtt_handler(event);

	return ESP_OK;
}

Handler::Handler()
	: subscriptions(),
	  mqtt_handle(nullptr),
	  wifi_connected(false),
	  mqtt_started(false), mqtt_connected(false) {

	config_lock = xSemaphoreCreateMutex();
}

void Handler::start(const mqtt_cfg &config) {
	assert(!mqtt_started);

	mqtt_started = true;
	esp_log_level_set("MQTT_CLIENT", ESP_LOG_WARN);

	mqtt_cfg modConfig = config;

	modConfig.event_handle = mqtt_handle_caller;
	modConfig.user_context = this;

	mqtt_handle = esp_mqtt_client_init(&modConfig);
	if(wifi_connected)
		esp_mqtt_client_start(mqtt_handle);
}
void Handler::start(const std::string uri, const std::string &status_topic) {
	mqtt_cfg config = {};
	config.uri = uri.data();

	config.buffer_size = 6*1024;
	config.task_prio = 20;

	config.disable_clean_session = false;

	if(status_topic != "") {
		config.lwt_topic = status_topic.data();
		config.lwt_retain = true;
		config.lwt_qos = 1;
		config.lwt_msg_len = 0;

		config.keepalive = 5;

		this->status_topic = status_topic;
	}

	start(config);
}

bool Handler::start_from_nvs(const std::string &status_topic) {
	if(!wifi_was_configured) {
		if(!Handler::start_wifi_from_nvs())
			return false;
	}

	nvs_handle_t read_handle;

	char uri_buffer[254] = {};
	size_t uri_length = 254;

	nvs_open("xasin", NVS_READONLY, &read_handle);
	auto ret = nvs_get_str(read_handle, "mqtt_uri", uri_buffer, &uri_length);
	nvs_close(read_handle);

	if(ret != ESP_OK)
		return false;

	ESP_LOGI(mqtt_tag, "Starting from NVS with URI %s", uri_buffer);
	start(uri_buffer, status_topic);

	return true;
}

void Handler::set_nvs_uri(const char *new_uri) {
	if(strlen(new_uri) > 500)
		return;

	nvs_handle_t write_handle;

	nvs_open("xasin", NVS_READWRITE, &write_handle);
	nvs_set_str(write_handle, "mqtt_uri", new_uri);

	nvs_commit(write_handle);
	nvs_close(write_handle);
}

void Handler::wifi_handler(system_event_t *event) {
	switch(event->event_id) {
	case SYSTEM_EVENT_STA_GOT_IP:
	case SYSTEM_EVENT_ETH_GOT_IP:
		wifi_connected = true;
		esp_mqtt_client_start(mqtt_handle);
	break;

	case SYSTEM_EVENT_STA_LOST_IP:
		wifi_connected = false;
		mqtt_connected = false;
		esp_mqtt_client_stop(mqtt_handle);
	break;

	default: break;
	}
}

void Handler::mqtt_handler(esp_mqtt_event_t *event) {
	xSemaphoreTake(config_lock, portMAX_DELAY);

	switch(event->event_id) {
	case MQTT_EVENT_CONNECTED:
		mqtt_connected = true;

		if(!event->session_present) {
			for(auto s : subscriptions)
				s->raw_subscribe();
		}
		if(status_topic != "")
			this->publish_to(status_topic, status_msg.data(), status_msg.length(), true);
		ESP_LOGI(mqtt_tag, "Reconnected and subscribed");
	break;

	case MQTT_EVENT_DISCONNECTED:
		ESP_LOGW(mqtt_tag, "Disconnected from broker!");
		mqtt_connected = false;

//		if(wifi_connected)
//			esp_mqtt_client_reconnect(mqtt_handle);
	break;

	case MQTT_EVENT_DATA: {
		const std::string dataString = std::string(event->data, event->data_len);
		const std::string topicString = std::string(event->topic, event->topic_len);

		ESP_LOGD(mqtt_tag, "Data for topic %s:", topicString.data());
		ESP_LOG_BUFFER_HEXDUMP(mqtt_tag, dataString.data(), dataString.length(), ESP_LOG_VERBOSE);

		for(auto s : subscriptions)
			s->feed_data({topicString, dataString});
	}
	break;

	default: break;
	}

	xSemaphoreGive(config_lock);
}

void Handler::set_status(const std::string newStatus) {
	if(status_topic == "")
		return;

	status_msg = newStatus;
	if(mqtt_connected)
		this->publish_to(status_topic, status_msg.data(), status_msg.length(), true);
}

void Handler::publish_to(const std::string &topic, const void *data, size_t length, bool retain, int qos) {
	if(!mqtt_connected) {
		ESP_LOGW(mqtt_tag, "Packet to %s dropped (disconnected)", topic.data());
		return;
	}

	ESP_LOGD(mqtt_tag, "Publishing to %s", topic.data());
	ESP_LOG_BUFFER_HEXDUMP(mqtt_tag, data, length, ESP_LOG_VERBOSE);
	esp_mqtt_client_publish(mqtt_handle, topic.data(), reinterpret_cast<const char*>(data)
				, length, qos, retain);
}

void Handler::publish_int(const std::string &topic, int32_t data, bool retain, int qos) {
	char buffer[10] = {};
	sprintf(buffer, "%d", data);

	publish_to(topic, buffer, strlen(buffer), retain, qos);
}

Subscription * Handler::subscribe_to(const std::string &topic, mqtt_callback cb, int qos) {
	// NO subscribing necessary here, the subscription class already handles this
	auto nSub = new Subscription(*this, topic, qos);
	nSub->on_received = cb;

	return nSub;
}

uint8_t Handler::is_disconnected() {
	if(!mqtt_started)
		return 255;

	if(mqtt_connected)
		return 0;
	if(wifi_connected)
		return 1;

	return 2;
}


} /* namespace MQTT */
} /* namespace Xasin */
