#ifndef DISKMGR_H
#define DISKMGR_H

#include <stdint.h>
#include <stdbool.h>

// Initialize disk manager app
void diskmgr_init(void);

// Run disk manager app
void diskmgr_run(void);

// Refresh disk list
void diskmgr_refresh(void);

// Redraw disk manager
void diskmgr_redraw(void);

#endif // DISKMGR_H
