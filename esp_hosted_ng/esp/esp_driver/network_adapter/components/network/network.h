#ifndef __NETWORK_H__
#define __NETWORK_H__

int wifi_init();

int wifi_scan();

int network_prov(void);

int mqtt_init();

int mqtt_publish(const char *topic, const char *data);

#endif