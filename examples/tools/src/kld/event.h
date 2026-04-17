#ifndef __EVENT_H__
#define __EVENT_H__

typedef enum {
  EVNT_HDLR_SUCCESS=0,
  EVNT_HDLR_FAILED=-1,
  EVNT_HDLR_EXIT_LOOP=1 } evnthdlrrc_t;
typedef  evnthdlrrc_t (* evnthdlr_t)(void *, uint32_t, int);
typedef struct {
  evnthdlr_t  hdlr;
  void       *obj;
} evntdesc_t;

#endif
