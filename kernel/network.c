#include "network.h"
#include "string.h"
#include "memory.h"

// Global network manager
static network_manager_t g_network_manager;

// Simple pseudo-random number generator for simulation
static uint32_t rand_seed = 12345;
static uint32_t simple_rand(void) {
    rand_seed = rand_seed * 1103515245 + 12345;
    return (rand_seed >> 16) & 0x7FFF;
}

void network_init(void) {
    memset(&g_network_manager, 0, sizeof(network_manager_t));
    
    // Set default configuration
    g_network_manager.status = NET_STATUS_DISABLED;
    g_network_manager.config.enabled = false;
    g_network_manager.config.auto_connect = true;
    g_network_manager.config.dhcp_enabled = true;
    
    // Set a fake MAC address
    g_network_manager.config.mac_address[0] = 0x00;
    g_network_manager.config.mac_address[1] = 0x11;
    g_network_manager.config.mac_address[2] = 0x22;
    g_network_manager.config.mac_address[3] = 0x33;
    g_network_manager.config.mac_address[4] = 0x44;
    g_network_manager.config.mac_address[5] = 0x55;
    
    // Default network settings (when connected)
    g_network_manager.config.ip_address[0] = 192;
    g_network_manager.config.ip_address[1] = 168;
    g_network_manager.config.ip_address[2] = 1;
    g_network_manager.config.ip_address[3] = 100;
    
    g_network_manager.config.subnet_mask[0] = 255;
    g_network_manager.config.subnet_mask[1] = 255;
    g_network_manager.config.subnet_mask[2] = 255;
    g_network_manager.config.subnet_mask[3] = 0;
    
    g_network_manager.config.gateway[0] = 192;
    g_network_manager.config.gateway[1] = 168;
    g_network_manager.config.gateway[2] = 1;
    g_network_manager.config.gateway[3] = 1;
    
    g_network_manager.config.dns_server[0] = 8;
    g_network_manager.config.dns_server[1] = 8;
    g_network_manager.config.dns_server[2] = 8;
    g_network_manager.config.dns_server[3] = 8;
    
    g_network_manager.network_count = 0;
    g_network_manager.selected_network = -1;
    g_network_manager.scanning = false;
}

network_manager_t* network_get_manager(void) {
    return &g_network_manager;
}

void network_set_enabled(bool enabled) {
    g_network_manager.config.enabled = enabled;
    
    if (enabled) {
        g_network_manager.status = NET_STATUS_DISCONNECTED;
        // Automatically start a scan when enabled
        network_start_scan();
    } else {
        g_network_manager.status = NET_STATUS_DISABLED;
        g_network_manager.config.connected_ssid[0] = '\0';
        g_network_manager.network_count = 0;
    }
}

bool network_is_enabled(void) {
    return g_network_manager.config.enabled;
}

network_status_t network_get_status(void) {
    return g_network_manager.status;
}

const char* network_status_string(network_status_t status) {
    switch (status) {
        case NET_STATUS_DISABLED:     return "Disabled";
        case NET_STATUS_DISCONNECTED: return "Disconnected";
        case NET_STATUS_CONNECTING:   return "Connecting...";
        case NET_STATUS_CONNECTED:    return "Connected";
        case NET_STATUS_ERROR:        return "Error";
        default:                      return "Unknown";
    }
}

bool network_start_scan(void) {
    if (!g_network_manager.config.enabled) {
        return false;
    }
    
    g_network_manager.scanning = true;
    g_network_manager.network_count = 0;
    
    // Simulate finding networks (in a real OS, this would query hardware)
    // Add some fake networks for demonstration
    
    // Network 1: Home network
    wifi_network_t* net = &g_network_manager.networks[0];
    strcpy(net->ssid, "MyHomeNetwork");
    net->signal_strength = -45 - (simple_rand() % 10);
    net->security = WIFI_SEC_WPA2;
    net->saved = true;
    
    // Network 2: Neighbor's network
    net = &g_network_manager.networks[1];
    strcpy(net->ssid, "NETGEAR-5G");
    net->signal_strength = -65 - (simple_rand() % 15);
    net->security = WIFI_SEC_WPA2;
    net->saved = false;
    
    // Network 3: Open network
    net = &g_network_manager.networks[2];
    strcpy(net->ssid, "Coffee Shop WiFi");
    net->signal_strength = -70 - (simple_rand() % 10);
    net->security = WIFI_SEC_NONE;
    net->saved = false;
    
    // Network 4: Another network
    net = &g_network_manager.networks[3];
    strcpy(net->ssid, "Office-Guest");
    net->signal_strength = -80 - (simple_rand() % 10);
    net->security = WIFI_SEC_WPA;
    net->saved = false;
    
    // Network 5: Hidden-ish network
    net = &g_network_manager.networks[4];
    strcpy(net->ssid, "xfinitywifi");
    net->signal_strength = -75 - (simple_rand() % 10);
    net->security = WIFI_SEC_NONE;
    net->saved = false;
    
    g_network_manager.network_count = 5;
    g_network_manager.scanning = false;
    
    return true;
}

bool network_is_scanning(void) {
    return g_network_manager.scanning;
}

int network_get_scan_results(wifi_network_t* results, int max_results) {
    if (results == NULL || max_results <= 0) {
        return 0;
    }
    
    int count = g_network_manager.network_count;
    if (count > max_results) {
        count = max_results;
    }
    
    for (int i = 0; i < count; i++) {
        memcpy(&results[i], &g_network_manager.networks[i], sizeof(wifi_network_t));
    }
    
    return count;
}

bool network_connect(const char* ssid, const char* password) {
    if (!g_network_manager.config.enabled || ssid == NULL) {
        return false;
    }
    
    // Simulate connection process
    g_network_manager.status = NET_STATUS_CONNECTING;
    
    // Find the network in our list
    bool found = false;
    wifi_network_t* target = NULL;
    for (int i = 0; i < g_network_manager.network_count; i++) {
        if (strcmp(g_network_manager.networks[i].ssid, ssid) == 0) {
            found = true;
            target = &g_network_manager.networks[i];
            g_network_manager.selected_network = i;
            break;
        }
    }
    
    if (!found) {
        g_network_manager.status = NET_STATUS_ERROR;
        return false;
    }
    
    // Check password for secured networks (simulation)
    if (target->security != WIFI_SEC_NONE) {
        if (password == NULL || strlen(password) < 8) {
            g_network_manager.status = NET_STATUS_ERROR;
            return false;
        }
    }
    
    // Simulate successful connection
    g_network_manager.status = NET_STATUS_CONNECTED;
    strcpy(g_network_manager.config.connected_ssid, ssid);
    target->saved = true;
    
    // Reset statistics
    g_network_manager.stats.bytes_sent = 0;
    g_network_manager.stats.bytes_received = 0;
    g_network_manager.stats.packets_sent = 0;
    g_network_manager.stats.packets_received = 0;
    g_network_manager.stats.errors = 0;
    g_network_manager.stats.connection_time = 0;
    
    return true;
}

bool network_disconnect(void) {
    if (g_network_manager.status != NET_STATUS_CONNECTED) {
        return false;
    }
    
    g_network_manager.status = NET_STATUS_DISCONNECTED;
    g_network_manager.config.connected_ssid[0] = '\0';
    g_network_manager.selected_network = -1;
    
    return true;
}

bool network_is_connected(void) {
    return g_network_manager.status == NET_STATUS_CONNECTED;
}

const char* network_get_connected_ssid(void) {
    if (g_network_manager.status == NET_STATUS_CONNECTED) {
        return g_network_manager.config.connected_ssid;
    }
    return NULL;
}

network_config_t* network_get_config(void) {
    return &g_network_manager.config;
}

void network_set_dhcp(bool enabled) {
    g_network_manager.config.dhcp_enabled = enabled;
}

void network_get_stats(network_stats_t* stats) {
    if (stats == NULL) return;
    memcpy(stats, &g_network_manager.stats, sizeof(network_stats_t));
}

void network_get_ip_string(char* buffer, size_t buffer_size) {
    if (buffer == NULL || buffer_size < 16) return;
    
    if (g_network_manager.status != NET_STATUS_CONNECTED) {
        strcpy(buffer, "0.0.0.0");
        return;
    }
    
    // Format IP address
    char temp[8];
    buffer[0] = '\0';
    
    utoa(g_network_manager.config.ip_address[0], temp, 10);
    strcat(buffer, temp);
    strcat(buffer, ".");
    
    utoa(g_network_manager.config.ip_address[1], temp, 10);
    strcat(buffer, temp);
    strcat(buffer, ".");
    
    utoa(g_network_manager.config.ip_address[2], temp, 10);
    strcat(buffer, temp);
    strcat(buffer, ".");
    
    utoa(g_network_manager.config.ip_address[3], temp, 10);
    strcat(buffer, temp);
}

const char* network_signal_bars(int8_t strength) {
    // Convert dBm to signal bars
    if (strength >= -50) return "[####]";      // Excellent
    if (strength >= -60) return "[### ]";      // Good
    if (strength >= -70) return "[##  ]";      // Fair
    if (strength >= -80) return "[#   ]";      // Weak
    return "[    ]";                            // Very weak
}

const char* network_security_string(wifi_security_t security) {
    switch (security) {
        case WIFI_SEC_NONE: return "Open";
        case WIFI_SEC_WEP:  return "WEP";
        case WIFI_SEC_WPA:  return "WPA";
        case WIFI_SEC_WPA2: return "WPA2";
        case WIFI_SEC_WPA3: return "WPA3";
        default:            return "Unknown";
    }
}

void network_simulate_activity(void) {
    if (g_network_manager.status != NET_STATUS_CONNECTED) {
        return;
    }
    
    // Simulate some network activity
    g_network_manager.stats.bytes_sent += simple_rand() % 1000;
    g_network_manager.stats.bytes_received += simple_rand() % 5000;
    g_network_manager.stats.packets_sent += simple_rand() % 10;
    g_network_manager.stats.packets_received += simple_rand() % 20;
    g_network_manager.stats.connection_time++;
    
    // Occasional errors
    if (simple_rand() % 100 < 2) {
        g_network_manager.stats.errors++;
    }
    
    // Update signal strength slightly
    if (g_network_manager.selected_network >= 0) {
        int8_t* strength = &g_network_manager.networks[g_network_manager.selected_network].signal_strength;
        *strength += (simple_rand() % 5) - 2;  // -2 to +2 variation
        if (*strength > -30) *strength = -30;
        if (*strength < -95) *strength = -95;
    }
}
