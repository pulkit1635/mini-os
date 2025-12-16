#ifndef SYSMON_H
#define SYSMON_H

#include <stdint.h>
#include <stdbool.h>

// Initialize system monitor
void sysmon_init(void);

// Run system monitor
void sysmon_run(void);

// Redraw system monitor
void sysmon_redraw(void);

#endif // SYSMON_H
