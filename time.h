#ifndef LSC_TIME_H
#define LSC_TIME_H

uint32_t current_time(void);

void reltime(fb_t *out, uint32_t now, uint32_t then);

#define ts_to_u32(ts) ((ts).tv_sec)

#endif
