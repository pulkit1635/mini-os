#ifndef NETWORK_H
#define NETWORK_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

// Network status
typedef enum {
    NET_STATUS_DISABLED = 0,
    NET_STATUS_DISCONNECTED,
    NET_STATUS_CONNECTING,
    NET_STATUS_CONNECTED,
    NET_STATUS_ERROR
} network_status_t;

// WiFi security types
typedef enum {
    WIFI_SEC_NONE = 0,
    WIFI_SEC_WEP,
    WIFI_SEC_WPA,
    WIFI_SEC_WPA2,
    WIFI_SEC_WPA3
} wifi_security_t;

// WiFi network entry
typedef struct {
    char ssid[33];                 // Network name (max 32 chars + null)
    int8_t signal_strength;        // Signal strength in dBm (-100 to 0)
    wifi_security_t security;      // Security type
    bool saved;                    // Known/saved network
} wifi_network_t;

// Maximum networks in scan results
#define WIFI_MAX_NETWORKS 16

// Network configuration
typedef struct {
    bool enabled;                  // WiFi enabled
    bool auto_connect;             // Auto-connect to known networks
    char connected_ssid[33];       // Currently connected network
    uint8_t ip_address[4];         // IP address
    uint8_t subnet_mask[4];        // Subnet mask
    uint8_t gateway[4];            // Default gateway
    uint8_t dns_server[4];         // DNS server
    bool dhcp_enabled;             // DHCP enabled
    uint8_t mac_address[6];        // MAC address
} network_config_t;

// Network statistics
typedef struct {
    uint64_t bytes_sent;
    uint64_t bytes_received;
    uint32_t packets_sent;
    uint32_t packets_received;
    uint32_t errors;
    uint32_t connection_time;      // Seconds connected
} network_stats_t;

// Network manager state
typedef struct {
    network_status_t status;
    network_config_t config;
    network_stats_t stats;
    wifi_network_t networks[WIFI_MAX_NETWORKS];
    int network_count;
    int selected_network;
    bool scanning;
} network_manager_t;

// Initialize network subsystem
void network_init(void);

// Get network manager state
network_manager_t* network_get_manager(void);

// Enable/disable WiFi
void network_set_enabled(bool enabled);
bool network_is_enabled(void);

// Get current status
network_status_t network_get_status(void);
const char* network_status_string(network_status_t status);

// WiFi scanning
bool network_start_scan(void);
bool network_is_scanning(void);
int network_get_scan_results(wifi_network_t* results, int max_results);

// Connection management
bool network_connect(const char* ssid, const char* password);
bool network_disconnect(void);
bool network_is_connected(void);
const char* network_get_connected_ssid(void);

// Get configuration
network_config_t* network_get_config(void);
void network_set_dhcp(bool enabled);

// Get statistics
void network_get_stats(network_stats_t* stats);

// Get IP address as string
void network_get_ip_string(char* buffer, size_t buffer_size);

// Signal strength helpers
const char* network_signal_bars(int8_t strength);
const char* network_security_string(wifi_security_t security);

// Simulate network activity (for demo purposes)
void network_simulate_activity(void);

#endif // NETWORK_H
