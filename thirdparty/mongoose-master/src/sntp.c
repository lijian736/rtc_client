#include "sntp.h"
#include "arch.h"
#include "event.h"
#include "log.h"
#include "util.h"

#define SNTP_INTERVAL_SEC 3600
#define SNTP_TIME_OFFSET 2208988800UL

int64_t mg_sntp_parse(const unsigned char *buf, size_t len) {
  int64_t res = -1;
  int mode = len > 0 ? buf[0] & 7 : 0;
  if (len < 48) {
    MG_ERROR(("%s", "corrupt packet"));
  } else if ((buf[0] & 0x38) >> 3 != 4) {
    MG_ERROR(("%s", "wrong version"));
  } else if (mode != 4 && mode != 5) {
    MG_ERROR(("%s", "not a server reply"));
  } else if (buf[1] == 0) {
    MG_ERROR(("%s", "server sent a kiss of death"));
  } else {
    uint32_t *data = (uint32_t *) &buf[40];
    unsigned long seconds = mg_ntohl(data[0]) - SNTP_TIME_OFFSET;
    unsigned long useconds = mg_ntohl(data[1]);
    // MG_DEBUG(("%lu %lu %lu", time(0), seconds, useconds));
    res = ((int64_t) seconds) * 1000 + (int64_t) ((useconds / 1000) % 1000);
  }
  return res;
}

static void sntp_cb(struct mg_connection *c, int ev, void *evd, void *fnd) {
  if (ev == MG_EV_READ) {
    int64_t milliseconds = mg_sntp_parse(c->recv.buf, c->recv.len);
    if (milliseconds > 0) {
      mg_call(c, MG_EV_SNTP_TIME, &milliseconds);
      MG_DEBUG(("%u.%u", (unsigned) (milliseconds / 1000),
                (unsigned) (milliseconds % 1000)));
    }
    c->recv.len = 0;  // Clear receive buffer
  } else if (ev == MG_EV_CONNECT) {
    mg_sntp_send(c, (unsigned long) 0);
  } else if (ev == MG_EV_CLOSE) {
  }
  (void) fnd;
  (void) evd;
}

void mg_sntp_send(struct mg_connection *c, unsigned long utc) {
  if (c->is_resolving) {
    MG_ERROR(("%lu wait until resolved", c->id));
  } else {
    uint8_t buf[48] = {0};
    buf[0] = (0 << 6) | (4 << 3) | 3;
    mg_send(c, buf, sizeof(buf));
    MG_DEBUG(("%lu ct %lu", c->id, utc));
  }
}

struct mg_connection *mg_sntp_connect(struct mg_mgr *mgr, const char *url,
                                      mg_event_handler_t fn, void *fnd) {
  struct mg_connection *c = NULL;
  if (url == NULL) url = "udp://time.google.com:123";
  if ((c = mg_connect(mgr, url, fn, fnd)) != NULL) c->pfn = sntp_cb;
  return c;
}
