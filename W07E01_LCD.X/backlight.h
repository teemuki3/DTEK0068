#ifndef BACKLIGHT_H
#define	BACKLIGHT_H

/* Makes all required inital configurations for backlight usage. */
void backlight_init(void);

/* Starts to automatically adjust backlight brightness 
 * depending on light level changes */
void backlight_auto_adjust(void* parameter);

/* Deactivates and activates backlight based on user activity. */
void backlight_control(void* parameter);

#endif	/* BACKLIGHT_H */

