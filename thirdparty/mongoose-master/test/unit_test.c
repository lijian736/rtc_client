#include "mongoose.h"

static int s_num_tests = 0;

#define ASSERT(expr)                                            \
  do {                                                          \
    s_num_tests++;                                              \
    if (!(expr)) {                                              \
      printf("FAILURE %s:%d: %s\n", __FILE__, __LINE__, #expr); \
      exit(EXIT_FAILURE);                                       \
    }                                                           \
  } while (0)

#define FETCH_BUF_SIZE (256 * 1024)

// Important: we use different port numbers for the Windows bug workaround. See
// https://support.microsoft.com/en-ae/help/3039044/error-10013-wsaeacces-is-returned-when-a-second-bind-to-a-excluded-por

static void test_globmatch(void) {
  ASSERT(mg_globmatch("", 0, "", 0) == 1);
  ASSERT(mg_globmatch("*", 1, "a", 1) == 1);
  ASSERT(mg_globmatch("*", 1, "ab", 2) == 1);
  ASSERT(mg_globmatch("", 0, "a", 1) == 0);
  ASSERT(mg_globmatch("/", 1, "/foo", 4) == 0);
  ASSERT(mg_globmatch("/*/foo", 6, "/x/bar", 6) == 0);
  ASSERT(mg_globmatch("/*/foo", 6, "/x/foo", 6) == 1);
  ASSERT(mg_globmatch("/*/foo", 6, "/x/foox", 7) == 0);
  ASSERT(mg_globmatch("/*/foo*", 7, "/x/foox", 7) == 1);
  ASSERT(mg_globmatch("/*", 2, "/abc", 4) == 1);
  ASSERT(mg_globmatch("/*", 2, "/ab/", 4) == 0);
  ASSERT(mg_globmatch("/*", 2, "/", 1) == 1);
  ASSERT(mg_globmatch("/x/*", 4, "/x/2", 4) == 1);
  ASSERT(mg_globmatch("/x/*", 4, "/x/2/foo", 8) == 0);
  ASSERT(mg_globmatch("/x/*/*", 6, "/x/2/foo", 8) == 1);
  ASSERT(mg_globmatch("#", 1, "///", 3) == 1);
  ASSERT(mg_globmatch("/api/*", 6, "/api/foo", 8) == 1);
  ASSERT(mg_globmatch("/api/*", 6, "/api/log/static", 15) == 0);
  ASSERT(mg_globmatch("/api/#", 6, "/api/log/static", 15) == 1);
  ASSERT(mg_globmatch("#.shtml", 7, "/ssi/index.shtml", 16) == 1);
  ASSERT(mg_globmatch("#.c", 3, ".c", 2) == 1);
  ASSERT(mg_globmatch("abc", 3, "ab", 2) == 0);
  ASSERT(mg_globmatch("#.c", 3, "a.c", 3) == 1);
  ASSERT(mg_globmatch("#.c", 3, "..c", 3) == 1);
  ASSERT(mg_globmatch("#.c", 3, "/.c", 3) == 1);
  ASSERT(mg_globmatch("#.c", 3, "//a.c", 5) == 1);
  ASSERT(mg_globmatch("#.c", 3, "x/a.c", 5) == 1);
  ASSERT(mg_globmatch("#.c", 3, "./a.c", 5) == 1);
  ASSERT(mg_globmatch("#.shtml", 7, "./ssi/index.shtml", 17) == 1);
  ASSERT(mg_globmatch("#aa#bb#", 7, "caabba", 6) == 1);
  ASSERT(mg_globmatch("#aa#bb#", 7, "caabxa", 6) == 0);
  ASSERT(mg_globmatch("a*b*c", 5, "a__b_c", 6) == 1);

  {
    struct mg_str caps[3];
    ASSERT(mg_match(mg_str("//a.c"), mg_str("#.c"), NULL) == true);
    ASSERT(mg_match(mg_str("a"), mg_str("#"), caps) == true);
    ASSERT(mg_strcmp(caps[0], mg_str("a")) == 0);
    ASSERT(mg_match(mg_str("//a.c"), mg_str("#.c"), caps) == true);
    ASSERT(mg_match(mg_str("a_b_c_"), mg_str("a*b*c"), caps) == false);
    ASSERT(mg_match(mg_str("a__b_c"), mg_str("a*b*c"), caps) == true);
    ASSERT(mg_strcmp(caps[0], mg_str("__")) == 0);
    ASSERT(mg_strcmp(caps[1], mg_str("_")) == 0);
    ASSERT(mg_match(mg_str("a_b_c__c"), mg_str("a*b*c"), caps) == true);
    ASSERT(mg_strcmp(caps[0], mg_str("_")) == 0);
    ASSERT(mg_strcmp(caps[1], mg_str("_c__")) == 0);
    ASSERT(mg_match(mg_str("a_xb_.c__c"), mg_str("a*b*c"), caps) == true);
    ASSERT(mg_strcmp(caps[0], mg_str("_x")) == 0);
    ASSERT(mg_strcmp(caps[1], mg_str("_.c__")) == 0);
    ASSERT(mg_match(mg_str("a"), mg_str("#a"), caps) == true);
    ASSERT(mg_strcmp(caps[0], mg_str("")) == 0);

    ASSERT(mg_match(mg_str(".aa..b...b"), mg_str("#a#b"), caps) == true);
    ASSERT(mg_strcmp(caps[0], mg_str(".")) == 0);
    ASSERT(mg_strcmp(caps[1], mg_str("a..b...")) == 0);
    ASSERT(mg_strcmp(caps[2], mg_str("")) == 0);

    ASSERT(mg_match(mg_str("/foo/bar"), mg_str("/*/*"), caps) == true);
    ASSERT(mg_strcmp(caps[0], mg_str("foo")) == 0);
    ASSERT(mg_strcmp(caps[1], mg_str("bar")) == 0);
    ASSERT(mg_strcmp(caps[2], mg_str("")) == 0);

    ASSERT(mg_match(mg_str("/foo/"), mg_str("/*/*"), caps) == true);
    ASSERT(mg_strcmp(caps[0], mg_str("foo")) == 0);
    ASSERT(mg_strcmp(caps[1], mg_str("")) == 0);
    ASSERT(mg_strcmp(caps[2], mg_str("")) == 0);

    ASSERT(mg_match(mg_str("abc"), mg_str("?#"), caps) == true);
    ASSERT(mg_strcmp(caps[0], mg_str("a")) == 0);
    ASSERT(mg_strcmp(caps[1], mg_str("bc")) == 0);
    ASSERT(mg_strcmp(caps[2], mg_str("")) == 0);
  }
}

static void test_commalist(void) {
  struct mg_str k, v, s1 = mg_str(""), s2 = mg_str("a"), s3 = mg_str("a,b");
  struct mg_str s4 = mg_str("a=123"), s5 = mg_str("a,b=123");
  ASSERT(mg_commalist(&s1, &k, &v) == false);

  ASSERT(mg_commalist(&s2, &k, &v) == true);
  ASSERT(v.len == 0 && mg_vcmp(&k, "a") == 0);
  ASSERT(mg_commalist(&s2, &k, &v) == false);

  ASSERT(mg_commalist(&s3, &k, &v) == true);
  ASSERT(v.len == 0 && mg_vcmp(&k, "a") == 0);
  ASSERT(mg_commalist(&s3, &k, &v) == true);
  ASSERT(v.len == 0 && mg_vcmp(&k, "b") == 0);
  ASSERT(mg_commalist(&s3, &k, &v) == false);

  ASSERT(mg_commalist(&s4, &k, &v) == true);
  ASSERT(mg_vcmp(&k, "a") == 0 && mg_vcmp(&v, "123") == 0);
  ASSERT(mg_commalist(&s4, &k, &v) == false);
  ASSERT(mg_commalist(&s4, &k, &v) == false);

  ASSERT(mg_commalist(&s5, &k, &v) == true);
  ASSERT(v.len == 0 && mg_vcmp(&k, "a") == 0);
  ASSERT(mg_commalist(&s5, &k, &v) == true);
  ASSERT(mg_vcmp(&k, "b") == 0 && mg_vcmp(&v, "123") == 0);
  ASSERT(mg_commalist(&s4, &k, &v) == false);
}

static void test_http_get_var(void) {
  char buf[256];
  struct mg_str body;
  body = mg_str("key1=value1&key2=value2&key3=value%203&key4=value+4");
  ASSERT(mg_http_get_var(&body, "key1", buf, sizeof(buf)) == 6);
  ASSERT(strcmp(buf, "value1") == 0);
  ASSERT(mg_http_get_var(&body, "KEY1", buf, sizeof(buf)) == 6);
  ASSERT(strcmp(buf, "value1") == 0);
  ASSERT(mg_http_get_var(&body, "key2", buf, sizeof(buf)) == 6);
  ASSERT(strcmp(buf, "value2") == 0);
  ASSERT(mg_http_get_var(&body, "key3", buf, sizeof(buf)) == 7);
  ASSERT(strcmp(buf, "value 3") == 0);
  ASSERT(mg_http_get_var(&body, "key4", buf, sizeof(buf)) == 7);
  ASSERT(strcmp(buf, "value 4") == 0);

  ASSERT(mg_http_get_var(&body, "key", buf, sizeof(buf)) == -4);
  ASSERT(mg_http_get_var(&body, "key1", NULL, sizeof(buf)) == -2);
  ASSERT(mg_http_get_var(&body, "key1", buf, 0) == -2);
  ASSERT(mg_http_get_var(&body, NULL, buf, sizeof(buf)) == -1);
  ASSERT(mg_http_get_var(&body, "key1", buf, 1) == -3);

  body = mg_str("key=broken%2");
  ASSERT(mg_http_get_var(&body, "key", buf, sizeof(buf)) == -3);

  body = mg_str("key=broken%2x");
  ASSERT(mg_http_get_var(&body, "key", buf, sizeof(buf)) == -3);
  ASSERT(mg_http_get_var(&body, "inexistent", buf, sizeof(buf)) == -4);
  body = mg_str("key=%");
  ASSERT(mg_http_get_var(&body, "key", buf, sizeof(buf)) == -3);
  body = mg_str("&&&kEy=%");
  ASSERT(mg_http_get_var(&body, "kEy", buf, sizeof(buf)) == -3);
}

static int vcmp(struct mg_str s1, const char *s2) {
  // MG_INFO(("->%.*s<->%s<- %d %d %d", (int) s1.len, s1.ptr, s2,
  //(int) s1.len, strncmp(s1.ptr, s2, s1.len), mg_vcmp(&s1, s2)));
  return mg_vcmp(&s1, s2) == 0;
}

static void test_url(void) {
  // Host
  ASSERT(vcmp(mg_url_host("foo"), "foo"));
  ASSERT(vcmp(mg_url_host("//foo"), "foo"));
  ASSERT(vcmp(mg_url_host("foo:1234"), "foo"));
  ASSERT(vcmp(mg_url_host(":1234"), ""));
  ASSERT(vcmp(mg_url_host("//foo:1234"), "foo"));
  ASSERT(vcmp(mg_url_host("p://foo"), "foo"));
  ASSERT(vcmp(mg_url_host("p://foo/"), "foo"));
  ASSERT(vcmp(mg_url_host("p://foo/x"), "foo"));
  ASSERT(vcmp(mg_url_host("p://foo/x/"), "foo"));
  ASSERT(vcmp(mg_url_host("p://foo/x//"), "foo"));
  ASSERT(vcmp(mg_url_host("p://foo//x"), "foo"));
  ASSERT(vcmp(mg_url_host("p://foo///x"), "foo"));
  ASSERT(vcmp(mg_url_host("p://foo///x//"), "foo"));
  ASSERT(vcmp(mg_url_host("p://bar:1234"), "bar"));
  ASSERT(vcmp(mg_url_host("p://bar:1234/"), "bar"));
  ASSERT(vcmp(mg_url_host("p://bar:1234/a"), "bar"));
  ASSERT(vcmp(mg_url_host("p://u@bar:1234/a"), "bar"));
  ASSERT(vcmp(mg_url_host("p://u:p@bar:1234/a"), "bar"));
  ASSERT(vcmp(mg_url_host("p://u:p@[::1]:1234/a"), "[::1]"));
  ASSERT(vcmp(mg_url_host("p://u:p@[1:2::3]:1234/a"), "[1:2::3]"));
  ASSERT(vcmp(mg_url_host("p://foo/x:y/z"), "foo"));

  // Port
  ASSERT(mg_url_port("foo:1234") == 1234);
  ASSERT(mg_url_port(":1234") == 1234);
  ASSERT(mg_url_port("x://foo:1234") == 1234);
  ASSERT(mg_url_port("x://foo:1234/") == 1234);
  ASSERT(mg_url_port("x://foo:1234/xx") == 1234);
  ASSERT(mg_url_port("x://foo:1234") == 1234);
  ASSERT(mg_url_port("p://bar:1234/a") == 1234);
  ASSERT(mg_url_port("p://bar:1234/a:b") == 1234);
  ASSERT(mg_url_port("http://bar") == 80);
  ASSERT(mg_url_port("http://localhost:1234") == 1234);
  ASSERT(mg_url_port("https://bar") == 443);
  ASSERT(mg_url_port("wss://bar") == 443);
  ASSERT(mg_url_port("wss://u:p@bar") == 443);
  ASSERT(mg_url_port("wss://u:p@bar:123") == 123);
  ASSERT(mg_url_port("wss://u:p@bar:123/") == 123);
  ASSERT(mg_url_port("wss://u:p@bar:123/abc") == 123);
  ASSERT(mg_url_port("http://u:p@[::1]/abc") == 80);
  ASSERT(mg_url_port("http://u:p@[::1]:2121/abc") == 2121);
  ASSERT(mg_url_port("http://u:p@[::1]:2121/abc/cd:ef") == 2121);

  // User / pass
  ASSERT(vcmp(mg_url_user("p://foo"), ""));
  ASSERT(vcmp(mg_url_pass("p://foo"), ""));
  ASSERT(vcmp(mg_url_user("p://:@foo"), ""));
  ASSERT(vcmp(mg_url_pass("p://:@foo"), ""));
  ASSERT(vcmp(mg_url_user("p://u@foo"), "u"));
  ASSERT(vcmp(mg_url_pass("p://u@foo"), ""));
  ASSERT(vcmp(mg_url_user("p://u:@foo"), "u"));
  ASSERT(vcmp(mg_url_pass("p://u:@foo"), ""));
  ASSERT(vcmp(mg_url_user("p://:p@foo"), ""));
  ASSERT(vcmp(mg_url_pass("p://:p@foo"), "p"));
  ASSERT(vcmp(mg_url_user("p://u:p@foo"), "u"));
  ASSERT(vcmp(mg_url_pass("p://u:p@foo"), "p"));
  ASSERT(vcmp(mg_url_pass("p://u:p@foo//a@b"), "p"));

  // URI
  ASSERT(strcmp(mg_url_uri("p://foo"), "/") == 0);
  ASSERT(strcmp(mg_url_uri("p://foo/"), "/") == 0);
  ASSERT(strcmp(mg_url_uri("p://foo:12/"), "/") == 0);
  ASSERT(strcmp(mg_url_uri("p://foo:12/abc"), "/abc") == 0);
  ASSERT(strcmp(mg_url_uri("p://foo:12/a/b/c"), "/a/b/c") == 0);
  ASSERT(strcmp(mg_url_uri("p://[::1]:12/a/b/c"), "/a/b/c") == 0);
  ASSERT(strcmp(mg_url_uri("p://[ab::1]:12/a/b/c"), "/a/b/c") == 0);
}

static void test_base64(void) {
  char buf[128];

  ASSERT(mg_base64_encode((uint8_t *) "", 0, buf) == 0);
  ASSERT(strcmp(buf, "") == 0);
  ASSERT(mg_base64_encode((uint8_t *) "x", 1, buf) == 4);
  ASSERT(strcmp(buf, "eA==") == 0);
  ASSERT(mg_base64_encode((uint8_t *) "xyz", 3, buf) == 4);
  ASSERT(strcmp(buf, "eHl6") == 0);
  ASSERT(mg_base64_encode((uint8_t *) "abcdef", 6, buf) == 8);
  ASSERT(strcmp(buf, "YWJjZGVm") == 0);
  ASSERT(mg_base64_encode((uint8_t *) "ы", 2, buf) == 4);
  ASSERT(strcmp(buf, "0Ys=") == 0);
  ASSERT(mg_base64_encode((uint8_t *) "xy", 3, buf) == 4);
  ASSERT(strcmp(buf, "eHkA") == 0);
  ASSERT(mg_base64_encode((uint8_t *) "test", 4, buf) == 8);
  ASSERT(strcmp(buf, "dGVzdA==") == 0);
  ASSERT(mg_base64_encode((uint8_t *) "abcde", 5, buf) == 8);
  ASSERT(strcmp(buf, "YWJjZGU=") == 0);

  ASSERT(mg_base64_decode("кю", 4, buf) == 0);
  ASSERT(mg_base64_decode("A", 1, buf) == 0);
  ASSERT(mg_base64_decode("A=", 2, buf) == 0);
  ASSERT(mg_base64_decode("AA=", 3, buf) == 0);
  ASSERT(mg_base64_decode("AAA=", 4, buf) == 2);
  ASSERT(mg_base64_decode("AAAA====", 8, buf) == 0);
  ASSERT(mg_base64_decode("AAAA----", 8, buf) == 0);
  ASSERT(mg_base64_decode("Q2VzYW50YQ==", 12, buf) == 7);
  ASSERT(strcmp(buf, "Cesanta") == 0);
}

static void test_iobuf(void) {
  struct mg_iobuf io = {0, 0, 0};
  ASSERT(io.buf == NULL && io.size == 0 && io.len == 0);
  mg_iobuf_resize(&io, 1);
  ASSERT(io.buf != NULL && io.size == 1 && io.len == 0);
  ASSERT(memcmp(io.buf, "\x00", 1) == 0);
  mg_iobuf_add(&io, 3, "hi", 2, 10);
  ASSERT(io.buf != NULL && io.size == 10 && io.len == 5);
  ASSERT(memcmp(io.buf, "\x00\x00\x00hi", 5) == 0);
  mg_iobuf_add(&io, io.len, "!", 1, 10);
  ASSERT(io.buf != NULL && io.size == 10 && io.len == 6);
  ASSERT(memcmp(io.buf, "\x00\x00\x00hi!", 6) == 0);
  mg_iobuf_add(&io, 0, "x", 1, 10);
  ASSERT(memcmp(io.buf, "x\x00\x00\x00hi!", 7) == 0);
  ASSERT(io.buf != NULL && io.size == 10 && io.len == 7);
  mg_iobuf_del(&io, 1, 3);
  ASSERT(io.buf != NULL && io.size == 10 && io.len == 4);
  ASSERT(memcmp(io.buf, "xhi!", 3) == 0);
  mg_iobuf_del(&io, 10, 100);
  ASSERT(io.buf != NULL && io.size == 10 && io.len == 4);
  ASSERT(memcmp(io.buf, "xhi!", 3) == 0);
  free(io.buf);
}

static void sntp_cb(struct mg_connection *c, int ev, void *evd, void *fnd) {
  if (ev == MG_EV_SNTP_TIME) {
    *(int64_t *) fnd = *(int64_t *) evd;
  }
  (void) c;
}

static void test_sntp(void) {
  int64_t ms = 0;
  struct mg_mgr mgr;
  struct mg_connection *c = NULL;
  int i;

  mg_mgr_init(&mgr);
  c = mg_sntp_connect(&mgr, NULL, sntp_cb, &ms);
  ASSERT(c != NULL);
  ASSERT(c->is_udp == 1);
  mg_sntp_send(c, (unsigned long) time(NULL));
  for (i = 0; i < 300 && ms == 0; i++) mg_mgr_poll(&mgr, 10);
  ASSERT(ms > 0);
  mg_mgr_free(&mgr);

  {
    const unsigned char sntp_good[] =
        "\x24\x02\x00\xeb\x00\x00\x00\x1e\x00\x00\x07\xb6\x3e"
        "\xc9\xd6\xa2\xdb\xde\xea\x30\x91\x86\xb7\x10\xdb\xde"
        "\xed\x98\x00\x00\x00\xde\xdb\xde\xed\x99\x0a\xe2\xc7"
        "\x96\xdb\xde\xed\x99\x0a\xe4\x6b\xda";
    const unsigned char bad_good[] =
        "\x55\x02\x00\xeb\x00\x00\x00\x1e\x00\x00\x07\xb6\x3e"
        "\xc9\xd6\xa2\xdb\xde\xea\x30\x91\x86\xb7\x10\xdb\xde"
        "\xed\x98\x00\x00\x00\xde\xdb\xde\xed\x99\x0a\xe2\xc7"
        "\x96\xdb\xde\xed\x99\x0a\xe4\x6b\xda";
    ASSERT((ms = mg_sntp_parse(sntp_good, sizeof(sntp_good))) > 0);
#if MG_ARCH == MG_ARCH_UNIX
    time_t t = (time_t) (ms / 1000);
    struct tm tm;
    gmtime_r(&t, &tm);
    ASSERT(tm.tm_year == 116);
    ASSERT(tm.tm_mon == 10);
    ASSERT(tm.tm_mday == 22);
    ASSERT(tm.tm_hour == 16);
    ASSERT(tm.tm_min == 15);
    ASSERT(tm.tm_sec == 21);
#endif
    ASSERT(mg_sntp_parse(bad_good, sizeof(bad_good)) < 0);
  }

  ASSERT(mg_sntp_parse(NULL, 0) == -1);
}

static void mqtt_cb(struct mg_connection *c, int ev, void *evd, void *fnd) {
  char *buf = (char *) fnd;
  if (ev == MG_EV_MQTT_OPEN) {
    buf[0] = *(int *) evd == 0 ? 'X' : 'Y';
  } else if (ev == MG_EV_MQTT_MSG) {
    struct mg_mqtt_message *mm = (struct mg_mqtt_message *) evd;
    sprintf(buf + 1, "%.*s/%.*s", (int) mm->topic.len, mm->topic.ptr,
            (int) mm->data.len, mm->data.ptr);
  }
  (void) c;
}

static void test_mqtt(void) {
  char buf[50] = {0};
  struct mg_mgr mgr;
  struct mg_str topic = mg_str("x/f12"), data = mg_str("hi");
  struct mg_connection *c;
  struct mg_mqtt_opts opts;
  // const char *url = "mqtt://mqtt.eclipse.org:1883";
  const char *url = "mqtt://broker.hivemq.com:1883";
  int i;
  mg_mgr_init(&mgr);

  {
    uint8_t bad[] = " \xff\xff\xff\xff ";
    struct mg_mqtt_message mm;
    mg_mqtt_parse(bad, sizeof(bad), &mm);
  }

  // Connect with empty client ID
  c = mg_mqtt_connect(&mgr, url, NULL, mqtt_cb, buf);
  for (i = 0; i < 200 && buf[0] == 0; i++) mg_mgr_poll(&mgr, 10);
  ASSERT(buf[0] == 'X');
  mg_mqtt_sub(c, topic, 1);
  mg_mqtt_pub(c, topic, data, 1, false);
  for (i = 0; i < 300 && buf[1] == 0; i++) mg_mgr_poll(&mgr, 10);
  // MG_INFO(("[%s]", buf));
  ASSERT(strcmp(buf, "Xx/f12/hi") == 0);

  // Set params
  memset(buf, 0, sizeof(buf));
  memset(&opts, 0, sizeof(opts));
  opts.clean = true;
  opts.will_qos = 1;
  opts.will_retain = true;
  opts.keepalive = 20;
  opts.client_id = mg_str("mg_client");
  opts.will_topic = mg_str("mg_will_topic");
  opts.will_message = mg_str("mg_will_messsage");
  c = mg_mqtt_connect(&mgr, url, &opts, mqtt_cb, buf);
  for (i = 0; i < 300 && buf[0] == 0; i++) mg_mgr_poll(&mgr, 10);
  ASSERT(buf[0] == 'X');
  mg_mqtt_sub(c, topic, 1);
  mg_mqtt_pub(c, topic, data, 1, false);
  for (i = 0; i < 500 && buf[1] == 0; i++) mg_mgr_poll(&mgr, 10);
  ASSERT(strcmp(buf, "Xx/f12/hi") == 0);

  mg_mgr_free(&mgr);
  ASSERT(mgr.conns == NULL);
}

static void eh1(struct mg_connection *c, int ev, void *ev_data, void *fn_data) {
  struct mg_tls_opts *topts = (struct mg_tls_opts *) fn_data;
  if (ev == MG_EV_ACCEPT && topts != NULL) mg_tls_init(c, topts);
  if (ev == MG_EV_HTTP_MSG) {
    struct mg_http_message *hm = (struct mg_http_message *) ev_data;
    MG_INFO(("[%.*s %.*s] message len %d", (int) hm->method.len, hm->method.ptr,
             (int) hm->uri.len, hm->uri.ptr, (int) hm->message.len));
    if (mg_http_match_uri(hm, "/foo/*")) {
      mg_http_reply(c, 200, "", "uri: %.*s", hm->uri.len - 5, hm->uri.ptr + 5);
    } else if (mg_http_match_uri(hm, "/ws")) {
      mg_ws_upgrade(c, hm, NULL);
    } else if (mg_http_match_uri(hm, "/body")) {
      mg_http_reply(c, 200, "", "%.*s", (int) hm->body.len, hm->body.ptr);
    } else if (mg_http_match_uri(hm, "/bar")) {
      mg_http_reply(c, 404, "", "not found");
    } else if (mg_http_match_uri(hm, "/no_reason")) {
      mg_printf(c, "%s", "HTTP/1.0 200\r\nContent-Length: 2\r\n\r\nok");
    } else if (mg_http_match_uri(hm, "/badroot")) {
      struct mg_http_serve_opts sopts;
      memset(&sopts, 0, sizeof(sopts));
      sopts.root_dir = "/BAAADDD!";
      mg_http_serve_dir(c, hm, &sopts);
    } else if (mg_http_match_uri(hm, "/creds")) {
      char user[100], pass[100];
      mg_http_creds(hm, user, sizeof(user), pass, sizeof(pass));
      mg_http_reply(c, 200, "", "[%s]:[%s]", user, pass);
    } else if (mg_http_match_uri(hm, "/upload")) {
      mg_http_upload(c, hm, &mg_fs_posix, ".");
    } else if (mg_http_match_uri(hm, "/test/")) {
      struct mg_http_serve_opts sopts;
      memset(&sopts, 0, sizeof(sopts));
      sopts.root_dir = ".";
      sopts.extra_headers = "A: B\r\nC: D\r\n";
      mg_http_serve_dir(c, hm, &sopts);
    } else if (mg_http_match_uri(hm, "/servefile")) {
      struct mg_http_serve_opts sopts;
      memset(&sopts, 0, sizeof(sopts));
      sopts.mime_types = "foo=a/b,txt=c/d";
      mg_http_serve_file(c, hm, "test/data/a.txt", &sopts);
    } else {
      struct mg_http_serve_opts sopts;
      memset(&sopts, 0, sizeof(sopts));
      sopts.root_dir = "./test/data";
      sopts.ssi_pattern = "#.shtml";
      sopts.extra_headers = "C: D\r\n";
      mg_http_serve_dir(c, hm, &sopts);
    }
  } else if (ev == MG_EV_WS_OPEN) {
    struct mg_http_message *hm = (struct mg_http_message *) ev_data;
    ASSERT(mg_strcmp(hm->uri, mg_str("/ws")) == 0);
    mg_ws_send(c, "opened", 6, WEBSOCKET_OP_BINARY);
  } else if (ev == MG_EV_WS_MSG) {
    struct mg_ws_message *wm = (struct mg_ws_message *) ev_data;
    mg_ws_send(c, wm->data.ptr, wm->data.len, WEBSOCKET_OP_BINARY);
  }
}

struct fetch_data {
  char *buf;
  int code, closed;
};

static void fcb(struct mg_connection *c, int ev, void *ev_data, void *fn_data) {
  struct fetch_data *fd = (struct fetch_data *) fn_data;
  if (ev == MG_EV_HTTP_MSG) {
    struct mg_http_message *hm = (struct mg_http_message *) ev_data;
    snprintf(fd->buf, FETCH_BUF_SIZE, "%.*s", (int) hm->message.len,
             hm->message.ptr);
    fd->code = atoi(hm->uri.ptr);
    fd->closed = 1;
    c->is_closing = 1;
    (void) c;
  }
}

static int fetch(struct mg_mgr *mgr, char *buf, const char *url,
                 const char *fmt, ...) {
  struct fetch_data fd = {buf, 0, 0};
  int i;
  struct mg_connection *c = mg_http_connect(mgr, url, fcb, &fd);
  va_list ap;
  ASSERT(c != NULL);
  if (mg_url_is_ssl(url)) {
    struct mg_tls_opts opts;
    struct mg_str host = mg_url_host(url);
    memset(&opts, 0, sizeof(opts));
    opts.ca = "./test/data/ca.pem";
    if (strstr(url, "127.0.0.1") != NULL) {
      // Local connection, use self-signed certificates
      opts.ca = "./test/data/ss_ca.pem";
      opts.cert = "./test/data/ss_client.pem";
    } else {
      opts.srvname = host;
    }
    mg_tls_init(c, &opts);
    if (c->tls == NULL) fd.closed = 1;
    // c->is_hexdumping = 1;
  }
  va_start(ap, fmt);
  mg_vprintf(c, fmt, ap);
  va_end(ap);
  buf[0] = '\0';
  for (i = 0; i < 250 && buf[0] == '\0'; i++) mg_mgr_poll(mgr, 1);
  if (!fd.closed) c->is_closing = 1;
  mg_mgr_poll(mgr, 1);
  return fd.code;
}

static int cmpbody(const char *buf, const char *str) {
  struct mg_http_message hm;
  struct mg_str s = mg_str(str);
  size_t len = strlen(buf);
  mg_http_parse(buf, len, &hm);
  if (hm.body.len > len) hm.body.len = len - (size_t) (hm.body.ptr - buf);
  return mg_strcmp(hm.body, s);
}

static bool cmpheader(const char *buf, const char *name, const char *value) {
  struct mg_http_message hm;
  struct mg_str *h;
  size_t len = strlen(buf);
  mg_http_parse(buf, len, &hm);
  h = mg_http_get_header(&hm, name);
  return h != NULL && mg_strcmp(*h, mg_str(value)) == 0;
}

static void wcb(struct mg_connection *c, int ev, void *ev_data, void *fn_data) {
  if (ev == MG_EV_WS_OPEN) {
    struct mg_http_message *hm = (struct mg_http_message *) ev_data;
    struct mg_str *wsproto = mg_http_get_header(hm, "Sec-WebSocket-Protocol");
    ASSERT(wsproto != NULL);
    mg_ws_send(c, "boo", 3, WEBSOCKET_OP_BINARY);
    mg_ws_send(c, "", 0, WEBSOCKET_OP_PING);
    ((int *) fn_data)[0] += 100;
  } else if (ev == MG_EV_WS_MSG) {
    struct mg_ws_message *wm = (struct mg_ws_message *) ev_data;
    if (mg_strstr(wm->data, mg_str("boo")))
      mg_ws_send(c, "", 0, WEBSOCKET_OP_CLOSE);
    ((int *) fn_data)[0]++;
  } else if (ev == MG_EV_CLOSE) {
    ((int *) fn_data)[0] += 10;
  }
}

static void test_ws(void) {
  char buf[FETCH_BUF_SIZE];
  const char *url = "ws://LOCALHOST:12343/ws";
  struct mg_mgr mgr;
  int i, done = 0;

  mg_mgr_init(&mgr);
  ASSERT(mg_http_listen(&mgr, url, eh1, NULL) != NULL);
  mg_ws_connect(&mgr, url, wcb, &done, "%s", "Sec-WebSocket-Protocol: meh\r\n");
  for (i = 0; i < 30; i++) mg_mgr_poll(&mgr, 1);
  // MG_INFO(("--> %d", done));
  ASSERT(done == 112);

  // Test that non-WS requests fail
  ASSERT(fetch(&mgr, buf, url, "GET /ws HTTP/1.0\r\n\n") == 426);

  mg_mgr_free(&mgr);
  ASSERT(mgr.conns == NULL);
}

static void eh9(struct mg_connection *c, int ev, void *ev_data, void *fn_data) {
  if (ev == MG_EV_ERROR) {
    ASSERT(!strcmp((char *) ev_data, "error connecting to 127.0.0.1:55117"));
    *(int *) fn_data = 7;
  }
  (void) c;
}

static void test_http_server(void) {
  struct mg_mgr mgr;
  const char *url = "http://127.0.0.1:12346";
  char buf[FETCH_BUF_SIZE];

  mg_mgr_init(&mgr);
  mg_http_listen(&mgr, url, eh1, NULL);

  ASSERT(fetch(&mgr, buf, url, "GET /a.txt HTTP/1.0\n\n") == 200);
  ASSERT(cmpbody(buf, "hello\n") == 0);

  ASSERT(fetch(&mgr, buf, url, "GET /%%61.txt HTTP/1.0\n\n") == 200);
  ASSERT(cmpbody(buf, "hello\n") == 0);

  // Responses with missing reason phrase must also work
  ASSERT(fetch(&mgr, buf, url, "GET /no_reason HTTP/1.0\n\n") == 200);
  ASSERT(cmpbody(buf, "ok") == 0);

  // Fetch file with unicode chars in filename
  ASSERT(fetch(&mgr, buf, url, "GET /київ.txt HTTP/1.0\n\n") == 200);
  ASSERT(cmpbody(buf, "є\n") == 0);

  ASSERT(fetch(&mgr, buf, url, "GET /../fuzz.c HTTP/1.0\n\n") == 404);
  ASSERT(fetch(&mgr, buf, url, "GET /.%%2e/fuzz.c HTTP/1.0\n\n") == 404);
  ASSERT(fetch(&mgr, buf, url, "GET /.%%2e%%2ffuzz.c HTTP/1.0\n\n") == 404);
  ASSERT(fetch(&mgr, buf, url, "GET /..%%2f%%20fuzz.c HTTP/1.0\n\n") == 404);
  ASSERT(fetch(&mgr, buf, url, "GET /..%%2ffuzz.c%%20 HTTP/1.0\n\n") == 404);

  ASSERT(fetch(&mgr, buf, url, "GET /dredir HTTP/1.0\n\n") == 301);
  ASSERT(cmpheader(buf, "Location", "/dredir/"));

  ASSERT(fetch(&mgr, buf, url, "GET /dredir/ HTTP/1.0\n\n") == 200);
  ASSERT(cmpbody(buf, "hi\n") == 0);

  ASSERT(fetch(&mgr, buf, url, "GET /..ddot HTTP/1.0\n\n") == 301);
  ASSERT(fetch(&mgr, buf, url, "GET /..ddot/ HTTP/1.0\n\n") == 200);
  ASSERT(cmpbody(buf, "hi\n") == 0);

  {
    extern char *mg_http_etag(char *, size_t, size_t, time_t);
    char etag[100];
    size_t size = 0;
    time_t mtime = 0;
    ASSERT(mg_fs_posix.st("./test/data/a.txt", &size, &mtime) != 0);
    ASSERT(mg_http_etag(etag, sizeof(etag), size, mtime) == etag);
    ASSERT(fetch(&mgr, buf, url, "GET /a.txt HTTP/1.0\nIf-None-Match: %s\n\n",
                 etag) == 304);
  }

  // Text mime type override
  ASSERT(fetch(&mgr, buf, url, "GET /servefile HTTP/1.0\n\n") == 200);
  ASSERT(cmpbody(buf, "hello\n") == 0);
  {
    struct mg_http_message hm;
    mg_http_parse(buf, strlen(buf), &hm);
    ASSERT(mg_http_get_header(&hm, "Content-Type") != NULL);
    ASSERT(mg_strcmp(*mg_http_get_header(&hm, "Content-Type"), mg_str("c/d")) ==
           0);
  }

  ASSERT(fetch(&mgr, buf, url, "GET /foo/1 HTTP/1.0\r\n\n") == 200);
  // MG_INFO(("%d %.*s", (int) hm.len, (int) hm.len, hm.buf));
  ASSERT(cmpbody(buf, "uri: 1") == 0);

  ASSERT(fetch(&mgr, buf, url, "%s",
               "POST /body HTTP/1.1\r\n"
               "Content-Length: 4\r\n\r\nkuku") == 200);
  ASSERT(cmpbody(buf, "kuku") == 0);

  ASSERT(fetch(&mgr, buf, url, "GET /ssi HTTP/1.1\r\n\r\n") == 301);
  ASSERT(fetch(&mgr, buf, url, "GET /ssi/ HTTP/1.1\r\n\r\n") == 200);
  ASSERT(cmpbody(buf,
                 "this is index\n"
                 "this is nested\n\n"
                 "this is f1\n\n\n\n"
                 "recurse\n\n"
                 "recurse\n\n"
                 "recurse\n\n"
                 "recurse\n\n"
                 "recurse\n\n") == 0);
  {
    struct mg_http_message hm;
    mg_http_parse(buf, strlen(buf), &hm);
    ASSERT(mg_http_get_header(&hm, "Content-Length") != NULL);
    ASSERT(mg_http_get_header(&hm, "Content-Type") != NULL);
    ASSERT(mg_strcmp(*mg_http_get_header(&hm, "Content-Type"),
                     mg_str("text/html; charset=utf-8")) == 0);
  }

  ASSERT(fetch(&mgr, buf, url, "GET /badroot HTTP/1.0\r\n\n") == 400);
  ASSERT(cmpbody(buf, "Invalid web root [/BAAADDD!]\n") == 0);

  {
    char *data = mg_file_read(&mg_fs_posix, "./test/data/ca.pem", NULL);
    ASSERT(fetch(&mgr, buf, url, "GET /ca.pem HTTP/1.0\r\n\n") == 200);
    ASSERT(cmpbody(buf, data) == 0);
    free(data);
  }

  {
    // Test mime type
    struct mg_http_message hm;
    ASSERT(fetch(&mgr, buf, url, "GET /empty.js HTTP/1.0\r\n\n") == 200);
    mg_http_parse(buf, strlen(buf), &hm);
    ASSERT(mg_http_get_header(&hm, "Content-Type") != NULL);
    ASSERT(mg_strcmp(*mg_http_get_header(&hm, "Content-Type"),
                     mg_str("text/javascript; charset=utf-8")) == 0);
  }

  {
    // Test connection refused
    int i, errored = 0;
    mg_connect(&mgr, "tcp://127.0.0.1:55117", eh9, &errored);
    for (i = 0; i < 10 && errored == 0; i++) mg_mgr_poll(&mgr, 1);
    ASSERT(errored == 7);
  }

  // Directory listing
  fetch(&mgr, buf, url, "GET /test/ HTTP/1.0\n\n");
  ASSERT(fetch(&mgr, buf, url, "GET /test/ HTTP/1.0\n\n") == 200);
  ASSERT(mg_strstr(mg_str(buf), mg_str(">Index of /test/<")) != NULL);
  ASSERT(mg_strstr(mg_str(buf), mg_str(">fuzz.c<")) != NULL);

  {
    // Credentials
    struct mg_http_message hm;
    ASSERT(fetch(&mgr, buf, url, "%s",
                 "GET /creds?access_token=x HTTP/1.0\r\n\r\n") == 200);
    mg_http_parse(buf, strlen(buf), &hm);
    ASSERT(mg_strcmp(hm.body, mg_str("[]:[x]")) == 0);

    ASSERT(fetch(&mgr, buf, url, "%s",
                 "GET /creds HTTP/1.0\r\n"
                 "Authorization: Bearer x\r\n\r\n") == 200);
    mg_http_parse(buf, strlen(buf), &hm);
    ASSERT(mg_strcmp(hm.body, mg_str("[]:[x]")) == 0);

    ASSERT(fetch(&mgr, buf, url, "%s",
                 "GET /creds HTTP/1.0\r\n"
                 "Authorization: Basic Zm9vOmJhcg==\r\n\r\n") == 200);
    mg_http_parse(buf, strlen(buf), &hm);
    ASSERT(mg_strcmp(hm.body, mg_str("[foo]:[bar]")) == 0);

    ASSERT(fetch(&mgr, buf, url, "%s",
                 "GET /creds HTTP/1.0\r\n"
                 "Cookie: blah; access_token=hello\r\n\r\n") == 200);
    mg_http_parse(buf, strlen(buf), &hm);
    ASSERT(mg_strcmp(hm.body, mg_str("[]:[hello]")) == 0);
  }

  {
    // Test upload
    char *p;
    remove("uploaded.txt");
    ASSERT((p = mg_file_read(&mg_fs_posix, "uploaded.txt", NULL)) == NULL);
    ASSERT(fetch(&mgr, buf, url,
                 "POST /upload HTTP/1.0\n"
                 "Content-Length: 1\n\nx") == 400);

    ASSERT(fetch(&mgr, buf, url,
                 "POST /upload?name=uploaded.txt HTTP/1.0\r\n"
                 "Content-Length: 5\r\n"
                 "\r\nhello") == 200);
    ASSERT(fetch(&mgr, buf, url,
                 "POST /upload?name=uploaded.txt&offset=5 HTTP/1.0\r\n"
                 "Content-Length: 6\r\n"
                 "\r\n\nworld") == 200);
    ASSERT((p = mg_file_read(&mg_fs_posix, "uploaded.txt", NULL)) != NULL);
    ASSERT(strcmp(p, "hello\nworld") == 0);
    free(p);
    remove("uploaded.txt");
  }

  {
    // Test upload directory traversal
    char *p;
    remove("uploaded.txt");
    ASSERT((p = mg_file_read(&mg_fs_posix, "uploaded.txt", NULL)) == NULL);
    ASSERT(fetch(&mgr, buf, url,
                 "POST /upload?name=../uploaded.txt HTTP/1.0\r\n"
                 "Content-Length: 5\r\n"
                 "\r\nhello") == 200);
    ASSERT((p = mg_file_read(&mg_fs_posix, "uploaded.txt", NULL)) != NULL);
    ASSERT(strcmp(p, "hello") == 0);
    free(p);
    remove("uploaded.txt");
  }

  // HEAD request
  ASSERT(fetch(&mgr, buf, url, "GET /a.txt HTTP/1.0\n\n") == 200);
  ASSERT(fetch(&mgr, buf, url, "HEAD /a.txt HTTP/1.0\n\n") == 200);

#if MG_ENABLE_IPV6
  {
    const char *url6 = "http://[::1]:12366";
    ASSERT(mg_http_listen(&mgr, url6, eh1, NULL) != NULL);
    ASSERT(fetch(&mgr, buf, url6, "GET /a.txt HTTP/1.0\n\n") == 200);
    ASSERT(cmpbody(buf, "hello\n") == 0);
  }
#endif

  mg_mgr_free(&mgr);
  ASSERT(mgr.conns == NULL);
}

static void test_tls(void) {
#if MG_ENABLE_MBEDTLS || MG_ENABLE_OPENSSL
  struct mg_tls_opts opts = {.ca = "./test/data/ss_ca.pem",
                             .cert = "./test/data/ss_server.pem",
                             .certkey = "./test/data/ss_server.pem"};
  struct mg_mgr mgr;
  struct mg_connection *c;
  const char *url = "https://127.0.0.1:12347";
  char buf[FETCH_BUF_SIZE];
  mg_mgr_init(&mgr);
  c = mg_http_listen(&mgr, url, eh1, (void *) &opts);
  ASSERT(c != NULL);
  ASSERT(fetch(&mgr, buf, url, "GET /a.txt HTTP/1.0\n\n") == 200);
  // MG_INFO(("%s", buf));
  ASSERT(cmpbody(buf, "hello\n") == 0);
  mg_mgr_free(&mgr);
  ASSERT(mgr.conns == NULL);
#endif
}

static void f3(struct mg_connection *c, int ev, void *ev_data, void *fn_data) {
  int *ok = (int *) fn_data;
  // MG_INFO(("%d", ev));
  if (ev == MG_EV_CONNECT) {
    // c->is_hexdumping = 1;
    mg_printf(c, "GET / HTTP/1.0\r\nHost: %s\r\n\r\n",
              c->rem.is_ip6 ? "ipv6.google.com" : "cesanta.com");
  } else if (ev == MG_EV_HTTP_MSG) {
    struct mg_http_message *hm = (struct mg_http_message *) ev_data;
    // MG_INFO(("-->[%.*s]", (int) hm->message.len, hm->message.ptr));
    // ASSERT(mg_vcmp(&hm->method, "HTTP/1.1") == 0);
    // ASSERT(mg_vcmp(&hm->uri, "301") == 0);
    *ok = mg_http_status(hm);
  } else if (ev == MG_EV_CLOSE) {
    if (*ok == 0) *ok = 888;
  } else if (ev == MG_EV_ERROR) {
    if (*ok == 0) *ok = 777;
  }
}

static void test_http_client(void) {
  struct mg_mgr mgr;
  struct mg_connection *c;
  int i, ok = 0;
  mg_mgr_init(&mgr);
  c = mg_http_connect(&mgr, "http://cesanta.com", f3, &ok);
  ASSERT(c != NULL);
  for (i = 0; i < 500 && ok <= 0; i++) mg_mgr_poll(&mgr, 10);
  ASSERT(ok == 301);
  c->is_closing = 1;
  mg_mgr_poll(&mgr, 0);
  ok = 0;
#if MG_ENABLE_MBEDTLS || MG_ENABLE_OPENSSL
  {
    struct mg_tls_opts opts = {.ca = "./test/data/ca.pem"};
    c = mg_http_connect(&mgr, "https://cesanta.com", f3, &ok);
    ASSERT(c != NULL);
    mg_tls_init(c, &opts);
    for (i = 0; i < 500 && ok <= 0; i++) mg_mgr_poll(&mgr, 10);
    ASSERT(ok == 200);
  }
#endif

#if MG_ENABLE_IPV6
  ok = 0;
  // ipv6.google.com does not have IPv4 address, only IPv6, therefore
  // it is guaranteed to hit IPv6 resolution path.
  c = mg_http_connect(&mgr, "http://ipv6.google.com", f3, &ok);
  ASSERT(c != NULL);
  for (i = 0; i < 500 && ok <= 0; i++) mg_mgr_poll(&mgr, 10);
  ASSERT(ok == 200);
#endif

  mg_mgr_free(&mgr);
  ASSERT(mgr.conns == NULL);
}

static void f4(struct mg_connection *c, int ev, void *ev_data, void *fn_data) {
  if (ev == MG_EV_HTTP_MSG) {
    struct mg_http_message *hm = (struct mg_http_message *) ev_data;
    mg_printf(c, "HTTP/1.0 200 OK\n\n%.*s/%s", (int) hm->uri.len, hm->uri.ptr,
              fn_data);
    c->is_draining = 1;
  }
}

static void test_http_no_content_length(void) {
  struct mg_mgr mgr;
  const char *url = "http://127.0.0.1:12348";
  char buf[FETCH_BUF_SIZE];
  mg_mgr_init(&mgr);
  mg_http_listen(&mgr, url, f4, (void *) "baz");
  ASSERT(fetch(&mgr, buf, url, "GET /foo/bar HTTP/1.0\r\n\n") == 200);
  ASSERT(cmpbody(buf, "/foo/bar/baz") == 0);
  mg_mgr_free(&mgr);
  ASSERT(mgr.conns == NULL);
}

static void f5(struct mg_connection *c, int ev, void *ev_data, void *fn_data) {
  if (ev == MG_EV_HTTP_MSG) {
    struct mg_http_message *hm = (struct mg_http_message *) ev_data;
    mg_printf(c, "HTTP/1.0 200 OK\n\n%.*s", (int) hm->uri.len, hm->uri.ptr);
    (*(int *) fn_data)++;
  }
}

static void test_http_pipeline(void) {
  struct mg_mgr mgr;
  const char *url = "http://127.0.0.1:12377";
  struct mg_connection *c;
  int i, ok = 0;
  mg_mgr_init(&mgr);
  mg_http_listen(&mgr, url, f5, (void *) &ok);
  c = mg_http_connect(&mgr, url, NULL, NULL);
  mg_printf(c, "POST / HTTP/1.0\nContent-Length: 5\n\n12345GET / HTTP/1.0\n\n");
  for (i = 0; i < 20; i++) mg_mgr_poll(&mgr, 1);
  // MG_INFO(("-----> [%d]", ok));
  ASSERT(ok == 2);
  mg_mgr_free(&mgr);
  ASSERT(mgr.conns == NULL);
}

static void test_http_parse(void) {
  struct mg_str *v;
  struct mg_http_message req;

  {
    const char *s = "GET / HTTP/1.0\n\n";
    ASSERT(mg_http_parse("\b23", 3, &req) == -1);
    ASSERT(mg_http_parse("get\n\n", 5, &req) == -1);
    ASSERT(mg_http_parse(s, strlen(s) - 1, &req) == 0);
    ASSERT(mg_http_parse(s, strlen(s), &req) == (int) strlen(s));
    ASSERT(req.message.len == strlen(s));
    ASSERT(req.body.len == 0);
  }

  {
    const char *s = "GET /blah HTTP/1.0\r\nFoo:  bar  \r\n\r\n";
    size_t idx, len = strlen(s);
    ASSERT(mg_http_parse(s, strlen(s), &req) == (int) len);
    ASSERT(mg_vcmp(&req.headers[0].name, "Foo") == 0);
    ASSERT(mg_vcmp(&req.headers[0].value, "bar") == 0);
    ASSERT(req.headers[1].name.len == 0);
    ASSERT(req.headers[1].name.ptr == NULL);
    ASSERT(req.query.len == 0);
    ASSERT(req.message.len == len);
    ASSERT(req.body.len == 0);
    for (idx = 0; idx < len; idx++) ASSERT(mg_http_parse(s, idx, &req) == 0);
  }

  {
    static const char *s = "get b c\nz :  k \nb: t\nvvv\n\n xx";
    ASSERT(mg_http_parse(s, strlen(s), &req) == (int) strlen(s) - 3);
    ASSERT(req.headers[2].name.len == 0);
    ASSERT(mg_vcmp(&req.headers[0].value, "k") == 0);
    ASSERT(mg_vcmp(&req.headers[1].value, "t") == 0);
    ASSERT(req.body.len == 0);
  }

  {
    const char *s = "a b c\r\nContent-Length: 21 \r\nb: t\r\nvvv\r\n\r\nabc";
    ASSERT(mg_http_parse(s, strlen(s), &req) == (int) strlen(s) - 3);
    ASSERT(req.body.len == 21);
    ASSERT(req.message.len == 21 - 3 + strlen(s));
    ASSERT(mg_http_get_header(&req, "foo") == NULL);
    ASSERT((v = mg_http_get_header(&req, "contENT-Length")) != NULL);
    ASSERT(mg_vcmp(v, "21") == 0);
    ASSERT((v = mg_http_get_header(&req, "B")) != NULL);
    ASSERT(mg_vcmp(v, "t") == 0);
  }

  {
    const char *s = "GET /foo?a=b&c=d HTTP/1.0\n\n";
    ASSERT(mg_http_parse(s, strlen(s), &req) == (int) strlen(s));
    ASSERT(mg_vcmp(&req.uri, "/foo") == 0);
    ASSERT(mg_vcmp(&req.query, "a=b&c=d") == 0);
  }

  {
    const char *s = "POST /x HTTP/1.0\n\n";
    ASSERT(mg_http_parse(s, strlen(s), &req) == (int) strlen(s));
    ASSERT(req.body.len == (size_t) ~0);
  }

  {
    const char *s = "WOHOO /x HTTP/1.0\n\n";
    ASSERT(mg_http_parse(s, strlen(s), &req) == (int) strlen(s));
    ASSERT(req.body.len == 0);
  }

  {
    const char *s = "HTTP/1.0 200 OK\n\n";
    ASSERT(mg_http_parse(s, strlen(s), &req) == (int) strlen(s));
    ASSERT(mg_vcmp(&req.method, "HTTP/1.0") == 0);
    ASSERT(mg_vcmp(&req.uri, "200") == 0);
    ASSERT(mg_vcmp(&req.proto, "OK") == 0);
    ASSERT(req.body.len == (size_t) ~0);
  }

  {
    static const char *s = "HTTP/1.0 999 OMGWTFBBQ\n\n";
    ASSERT(mg_http_parse(s, strlen(s), &req) == (int) strlen(s));
  }

  {
    const char *s =
        "GET / HTTP/1.0\r\nhost:127.0.0.1:18888\r\nCookie:\r\nX-PlayID: "
        "45455\r\nRange:  0-1 \r\n\r\n";
    ASSERT(mg_http_parse(s, strlen(s), &req) == (int) strlen(s));
    ASSERT((v = mg_http_get_header(&req, "Host")) != NULL);
    ASSERT(mg_vcmp(v, "127.0.0.1:18888") == 0);
    ASSERT((v = mg_http_get_header(&req, "Cookie")) != NULL);
    ASSERT(v->len == 0);
    ASSERT((v = mg_http_get_header(&req, "X-PlayID")) != NULL);
    ASSERT(mg_vcmp(v, "45455") == 0);
    ASSERT((v = mg_http_get_header(&req, "Range")) != NULL);
    ASSERT(mg_vcmp(v, "0-1") == 0);
  }

  {
    static const char *s = "a b c\na:1\nb:2\nc:3\nd:4\ne:5\nf:6\ng:7\nh:8\n\n";
    ASSERT(mg_http_parse(s, strlen(s), &req) == (int) strlen(s));
    ASSERT((v = mg_http_get_header(&req, "e")) != NULL);
    ASSERT(mg_vcmp(v, "5") == 0);
    ASSERT((v = mg_http_get_header(&req, "h")) == NULL);
  }

  {
    struct mg_connection c;
    struct mg_str s,
        res = mg_str("GET /\r\nAuthorization: Basic Zm9vOmJhcg==\r\n\r\n");
    memset(&c, 0, sizeof(c));
    mg_printf(&c, "%s", "GET /\r\n");
    mg_http_bauth(&c, "foo", "bar");
    mg_printf(&c, "%s", "\r\n");
    s = mg_str_n((char *) c.send.buf, c.send.len);
    ASSERT(mg_strcmp(s, res) == 0);
    mg_iobuf_free(&c.send);
  }

  {
    struct mg_http_message hm;
    const char *s = "GET /foo?bar=baz HTTP/1.0\n\n ";
    ASSERT(mg_http_parse(s, strlen(s), &hm) == (int) strlen(s) - 1);
    ASSERT(mg_strcmp(hm.uri, mg_str("/foo")) == 0);
    ASSERT(mg_strcmp(hm.query, mg_str("bar=baz")) == 0);
  }

  {
    struct mg_http_message hm;
    const char *s = "a b c\n\n";
    ASSERT(mg_http_parse(s, strlen(s), &hm) == (int) strlen(s));
    s = "a b\nc\n\n";
    ASSERT(mg_http_parse(s, strlen(s), &hm) == (int) strlen(s));
    s = "a\nb\nc\n\n";
    ASSERT(mg_http_parse(s, strlen(s), &hm) < 0);
  }
}

static void ehr(struct mg_connection *c, int ev, void *ev_data, void *fn_data) {
  if (ev == MG_EV_HTTP_MSG) {
    struct mg_http_message *hm = (struct mg_http_message *) ev_data;
    struct mg_http_serve_opts opts;
    memset(&opts, 0, sizeof(opts));
    opts.root_dir = "./test/data";
    mg_http_serve_dir(c, hm, &opts);
  }
  (void) fn_data;
}

static void test_http_range(void) {
  struct mg_mgr mgr;
  const char *url = "http://127.0.0.1:12349";
  struct mg_http_message hm;
  char buf[FETCH_BUF_SIZE];

  mg_mgr_init(&mgr);
  mg_http_listen(&mgr, url, ehr, NULL);

  ASSERT(fetch(&mgr, buf, url, "GET /range.txt HTTP/1.0\n\n") == 200);
  ASSERT(mg_http_parse(buf, strlen(buf), &hm) > 0);
  ASSERT(hm.body.len == 312);

  fetch(&mgr, buf, url, "%s", "GET /range.txt HTTP/1.0\nRange: bytes=5-10\n\n");
  ASSERT(mg_http_parse(buf, strlen(buf), &hm) > 0);
  ASSERT(mg_strcmp(hm.uri, mg_str("206")) == 0);
  ASSERT(mg_strcmp(hm.proto, mg_str("Partial Content")) == 0);
  ASSERT(mg_strcmp(hm.body, mg_str(" of co")) == 0);
  ASSERT(mg_strcmp(*mg_http_get_header(&hm, "Content-Range"),
                   mg_str("bytes 5-10/312")) == 0);

  // Fetch till EOF
  fetch(&mgr, buf, url, "%s", "GET /range.txt HTTP/1.0\nRange: bytes=300-\n\n");
  ASSERT(mg_http_parse(buf, strlen(buf), &hm) > 0);
  ASSERT(mg_strcmp(hm.uri, mg_str("206")) == 0);
  ASSERT(mg_strcmp(hm.body, mg_str("is disease.\n")) == 0);
  // MG_INFO(("----%d\n[%s]", (int) hm.body.len, buf));

  // Fetch past EOF, must trigger 416 response
  fetch(&mgr, buf, url, "%s", "GET /range.txt HTTP/1.0\nRange: bytes=999-\n\n");
  ASSERT(mg_http_parse(buf, strlen(buf), &hm) > 0);
  ASSERT(mg_strcmp(hm.uri, mg_str("416")) == 0);
  ASSERT(hm.body.len == 0);
  ASSERT(mg_strcmp(*mg_http_get_header(&hm, "Content-Range"),
                   mg_str("bytes */312")) == 0);

  fetch(&mgr, buf, url, "%s",
        "GET /range.txt HTTP/1.0\nRange: bytes=0-312\n\n");
  ASSERT(mg_http_parse(buf, strlen(buf), &hm) > 0);
  ASSERT(mg_strcmp(hm.uri, mg_str("416")) == 0);

  mg_mgr_free(&mgr);
  ASSERT(mgr.conns == NULL);
}

static void f1(void *arg) {
  (*(int *) arg)++;
}

static void test_timer(void) {
  int v1 = 0, v2 = 0, v3 = 0;
  struct mg_timer t1, t2, t3;

  MG_INFO(("g_timers: %p", g_timers));
  ASSERT(g_timers == NULL);

  mg_timer_init(&t1, 5, MG_TIMER_REPEAT, f1, &v1);
  mg_timer_init(&t2, 15, 0, f1, &v2);
  mg_timer_init(&t3, 10, MG_TIMER_REPEAT | MG_TIMER_RUN_NOW, f1, &v3);

  ASSERT(g_timers == &t3);
  ASSERT(g_timers->next == &t2);

  mg_timer_poll(0);
  mg_timer_poll(1);
  ASSERT(v1 == 0);
  ASSERT(v2 == 0);
  ASSERT(v3 == 1);

  mg_timer_poll(5);
  ASSERT(v1 == 1);
  ASSERT(v2 == 0);
  ASSERT(v3 == 1);

  ASSERT(g_timers == &t3);
  ASSERT(g_timers->next == &t2);

  // Simulate long delay - timers must invalidate expiration times
  mg_timer_poll(100);
  ASSERT(v1 == 2);
  ASSERT(v2 == 1);
  ASSERT(v3 == 2);

  ASSERT(g_timers == &t3);
  ASSERT(g_timers->next == &t1);  // t2 should be removed
  ASSERT(g_timers->next->next == NULL);

  mg_timer_poll(107);
  ASSERT(v1 == 3);
  ASSERT(v2 == 1);
  ASSERT(v3 == 2);

  mg_timer_poll(114);
  ASSERT(v1 == 4);
  ASSERT(v2 == 1);
  ASSERT(v3 == 3);

  mg_timer_poll(115);
  ASSERT(v1 == 5);
  ASSERT(v2 == 1);
  ASSERT(v3 == 3);

  mg_timer_init(&t2, 3, 0, f1, &v2);
  ASSERT(g_timers == &t2);
  ASSERT(g_timers->next == &t3);
  ASSERT(g_timers->next->next == &t1);
  ASSERT(g_timers->next->next->next == NULL);

  mg_timer_poll(120);
  ASSERT(v1 == 6);
  ASSERT(v2 == 1);
  ASSERT(v3 == 4);

  mg_timer_poll(125);
  ASSERT(v1 == 7);
  ASSERT(v2 == 2);
  ASSERT(v3 == 4);

  // Test millisecond counter wrap - when time goes back.
  mg_timer_poll(0);
  ASSERT(v1 == 7);
  ASSERT(v2 == 2);
  ASSERT(v3 == 4);

  ASSERT(g_timers == &t3);
  ASSERT(g_timers->next == &t1);
  ASSERT(g_timers->next->next == NULL);

  mg_timer_poll(7);
  ASSERT(v1 == 8);
  ASSERT(v2 == 2);
  ASSERT(v3 == 4);

  mg_timer_poll(11);
  ASSERT(v1 == 9);
  ASSERT(v2 == 2);
  ASSERT(v3 == 5);

  mg_timer_free(&t1);
  ASSERT(g_timers == &t3);
  ASSERT(g_timers->next == NULL);

  mg_timer_free(&t2);
  ASSERT(g_timers == &t3);
  ASSERT(g_timers->next == NULL);

  mg_timer_free(&t3);
  ASSERT(g_timers == NULL);
}

static bool sn(const char *fmt, ...) {
  char buf[100], tmp[1] = {0}, buf2[sizeof(buf)];
  size_t n, n2, n1;
  va_list ap;
  bool result;
  va_start(ap, fmt);
  n = (size_t) vsnprintf(buf2, sizeof(buf2), fmt, ap);
  va_end(ap);
  va_start(ap, fmt);
  n1 = mg_vsnprintf(buf, sizeof(buf), fmt, ap);
  va_end(ap);
  va_start(ap, fmt);
  n2 = mg_vsnprintf(tmp, 0, fmt, ap);
  va_end(ap);
  result = n1 == n2 && n1 == n && strcmp(buf, buf2) == 0;
  if (!result)
    MG_ERROR(("[%s] -> [%s] != [%s] %d %d %d\n", fmt, buf, buf2, (int) n1,
              (int) n2, (int) n));
  return result;
}

static bool sccmp(const char *s1, const char *s2) {
  int n1 = mg_casecmp(s1, s2);
#if MG_ARCH == MG_ARCH_UNIX
  int n2 = strcasecmp(s1, s2);
#else
  int n2 = mg_casecmp(s1, s2);  // On MSVC98, _stricmp() is buggy
#endif
  MG_INFO(("[%s] [%s] %d %d", s1, s2, n1, n2));
  return n1 == n2;
}

static void test_str(void) {
  struct mg_str s = mg_strdup(mg_str("a"));
  ASSERT(mg_strcmp(s, mg_str("a")) == 0);
  free((void *) s.ptr);
  ASSERT(mg_strcmp(mg_str(""), mg_str(NULL)) == 0);
  ASSERT(mg_strcmp(mg_str("a"), mg_str("b")) < 0);
  ASSERT(mg_strcmp(mg_str("b"), mg_str("a")) > 0);
  ASSERT(mg_strstr(mg_str("abc"), mg_str("d")) == NULL);
  ASSERT(mg_strstr(mg_str("abc"), mg_str("b")) != NULL);
  ASSERT(mg_strcmp(mg_str("hi"), mg_strstrip(mg_str(" \thi\r\n"))) == 0);

  ASSERT(sccmp("", ""));
  ASSERT(sccmp("", "1"));
  ASSERT(sccmp("a", "A"));
  ASSERT(sccmp("a1", "A"));
  ASSERT(sccmp("a", "A1"));

  ASSERT(sn("%d", 0));
  ASSERT(sn("%d", 1));
  ASSERT(sn("%d", -1));
  ASSERT(sn("%.*s", 1, "ab"));
  ASSERT(sn("%.1s", "ab"));
  ASSERT(sn("%.99s", "a"));
  ASSERT(sn("%11s", "a"));
  ASSERT(sn("%s", "a\0b"));
  ASSERT(sn("%2s", "a"));
  ASSERT(sn("%.*s", 3, "a\0b"));
  ASSERT(sn("%d", 7));
  ASSERT(sn("%d", 123));
#if MG_ARCH == MG_ARCH_UNIX
  ASSERT(sn("%lld", (uint64_t) 0xffffffffff));
  ASSERT(sn("%lld", (uint64_t) -1));
  ASSERT(sn("%llu", (uint64_t) -1));
  ASSERT(sn("%llx", (uint64_t) 0xffffffffff));
  ASSERT(sn("%p", (void *) (size_t) 7));
#endif
  ASSERT(sn("%lx", (unsigned long) 0x6204d754));
  ASSERT(sn("ab"));
  ASSERT(sn("%dx", 1));
  ASSERT(sn("%sx", "a"));
  ASSERT(sn("%cx", 32));
  ASSERT(sn("%x", 15));
  ASSERT(sn("%2x", 15));
  ASSERT(sn("%02x", 15));
  ASSERT(sn("%hx:%hhx", (short) 1, (char) 2));
  ASSERT(sn("%hx:%hhx", (short) 1, (char) 2));
  ASSERT(sn("%%"));
  ASSERT(sn("%x", 15));
  ASSERT(sn("%#x", 15));
  ASSERT(sn("%#6x", 15));
  ASSERT(sn("%#06x", 15));
  ASSERT(sn("%#-6x", 15));
  ASSERT(sn("%-2s!", "a"));
  ASSERT(sn("%s %s", "a", "b"));
  ASSERT(sn("%s %s", "a", "b"));
  ASSERT(sn("ab%dc", 123));
  ASSERT(sn("%s ", "a"));
  ASSERT(sn("%s %s", "a", "b"));
  ASSERT(sn("%2s %s", "a", "b"));
}

static void fn1(struct mg_connection *c, int ev, void *ev_data, void *fn_data) {
  if (ev == MG_EV_ERROR) sprintf((char *) fn_data, "%s", (char *) ev_data);
  (void) c;
}

static void test_dns_timeout(const char *dns_server_url, const char *errstr) {
  // Test timeout
  struct mg_mgr mgr;
  char buf[100] = "";
  int i;
  mg_mgr_init(&mgr);
  mgr.dns4.url = dns_server_url;
  mgr.dnstimeout = 10;
  MG_DEBUG(("opening dummy DNS listener..."));
  mg_listen(&mgr, mgr.dns4.url, NULL, NULL);  // Just discard our queries
  mg_http_connect(&mgr, "http://google.com", fn1, buf);
  for (i = 0; i < 50 && buf[0] == '\0'; i++) mg_mgr_poll(&mgr, 1);
  mg_mgr_free(&mgr);
  MG_DEBUG(("buf: [%s]", buf));
  ASSERT(strcmp(buf, errstr) == 0);
}

static void test_dns(void) {
  struct mg_dns_message dm;
  //       txid  flags numQ  numA  numAP numOP
  // 0000  00 01 81 80 00 01 00 01 00 00 00 00 07 63 65 73  .............ces
  // 0010  61 6e 74 61 03 63 6f 6d 00 00 01 00 01 c0 0c 00  anta.com........
  // 0020  01 00 01 00 00 02 57 00 04 94 fb 36 ec           ......W....6.
  uint8_t data[] = {0,    1,    0x81, 0x80, 0,    1,    0,    1,    0,
                    0,    0,    0,    7,    0x63, 0x65, 0x73, 0x61, 0x6e,
                    0x74, 0x61, 0x03, 0x63, 0x6f, 0x6d, 0,    0,    1,
                    0,    1,    0xc0, 0x0c, 0,    1,    0,    1,    0,
                    0,    2,    0x57, 0,    4,    0x94, 0xfb, 0x36, 0xec};
  ASSERT(mg_dns_parse(NULL, 0, &dm) == 0);
  ASSERT(mg_dns_parse(data, sizeof(data), &dm) == 1);
  ASSERT(strcmp(dm.name, "cesanta.com") == 0);
  data[30] = 29;  // Point a pointer to itself
  memset(&dm, 0, sizeof(dm));
  ASSERT(mg_dns_parse(data, sizeof(data), &dm) == 1);
  ASSERT(strcmp(dm.name, "") == 0);

  test_dns_timeout("udp://127.0.0.1:12345", "DNS timeout");
  test_dns_timeout("", "resolver");
  test_dns_timeout("tcp://0.0.0.0:0", "DNS error");
}

static void test_util(void) {
  char buf[100], *s = mg_hexdump("abc", 3), *p;
  struct mg_addr a;
  ASSERT(s != NULL);
  free(s);
  memset(&a, 0, sizeof(a));
  ASSERT(mg_file_printf(&mg_fs_posix, "data.txt", "%s", "hi") == true);
  if (system("ls -l") != 0) (void) 0;
  ASSERT((p = mg_file_read(&mg_fs_posix, "data.txt", NULL)) != NULL);
  ASSERT(strcmp(p, "hi") == 0);
  free(p);
  remove("data.txt");
  ASSERT(mg_aton(mg_str("0"), &a) == false);
  ASSERT(mg_aton(mg_str("0.0.0."), &a) == false);
  ASSERT(mg_aton(mg_str("0.0.0.256"), &a) == false);
  ASSERT(mg_aton(mg_str("0.0.0.-1"), &a) == false);
  ASSERT(mg_aton(mg_str("127.0.0.1"), &a) == true);
  ASSERT(a.is_ip6 == false);
  ASSERT(a.ip == 0x100007f);
  ASSERT(strcmp(mg_ntoa(&a, buf, sizeof(buf)), "127.0.0.1") == 0);

  ASSERT(mg_aton(mg_str("1:2:3:4:5:6:7:8"), &a) == true);
  ASSERT(a.is_ip6 == true);
  ASSERT(
      memcmp(a.ip6,
             "\x00\x01\x00\x02\x00\x03\x00\x04\x00\x05\x00\x06\x00\x07\x00\x08",
             sizeof(a.ip6)) == 0);

  memset(a.ip6, 0xaa, sizeof(a.ip6));
  ASSERT(mg_aton(mg_str("1::1"), &a) == true);
  ASSERT(a.is_ip6 == true);
  ASSERT(
      memcmp(a.ip6,
             "\x00\x01\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x01",
             sizeof(a.ip6)) == 0);

  memset(a.ip6, 0xaa, sizeof(a.ip6));
  ASSERT(mg_aton(mg_str("::fFff:1.2.3.4"), &a) == true);
  ASSERT(a.is_ip6 == true);
  ASSERT(memcmp(a.ip6,
                "\x00\x00\x00\x00\x00\x00\x00\x00"
                "\x00\x00\xff\xff\x01\x02\x03\x04",
                sizeof(a.ip6)) == 0);

  memset(a.ip6, 0xaa, sizeof(a.ip6));
  ASSERT(mg_aton(mg_str("::1"), &a) == true);
  ASSERT(a.is_ip6 == true);
  ASSERT(
      memcmp(a.ip6,
             "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x01",
             sizeof(a.ip6)) == 0);

  memset(a.ip6, 0xaa, sizeof(a.ip6));
  ASSERT(mg_aton(mg_str("1::"), &a) == true);
  ASSERT(a.is_ip6 == true);
  ASSERT(
      memcmp(a.ip6,
             "\x00\x01\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00",
             sizeof(a.ip6)) == 0);

  memset(a.ip6, 0xaa, sizeof(a.ip6));
  ASSERT(mg_aton(mg_str("2001:4860:4860::8888"), &a) == true);
  ASSERT(a.is_ip6 == true);
  ASSERT(
      memcmp(a.ip6,
             "\x20\x01\x48\x60\x48\x60\x00\x00\x00\x00\x00\x00\x00\x00\x88\x88",
             sizeof(a.ip6)) == 0);

  ASSERT(strcmp(mg_hex("abc", 3, buf), "616263") == 0);
  ASSERT(mg_url_decode("a=%", 3, buf, sizeof(buf), 0) < 0);
  ASSERT(mg_url_decode("&&&a=%", 6, buf, sizeof(buf), 0) < 0);

  {
    size_t n;
    ASSERT((n = mg_url_encode("", 0, buf, sizeof(buf))) == 0);
    ASSERT((n = mg_url_encode("a", 1, buf, 0)) == 0);
    ASSERT((n = mg_url_encode("a", 1, buf, sizeof(buf))) == 1);
    ASSERT(strncmp(buf, "a", n) == 0);
    ASSERT((n = mg_url_encode("._-~", 4, buf, sizeof(buf))) == 4);
    ASSERT(strncmp(buf, "._-~", n) == 0);
    ASSERT((n = mg_url_encode("a@%>", 4, buf, sizeof(buf))) == 10);
    ASSERT(strncmp(buf, "a%40%25%3e", n) == 0);
    ASSERT((n = mg_url_encode("a@b.c", 5, buf, sizeof(buf))) == 7);
    ASSERT(strncmp(buf, "a%40b.c", n) == 0);
  }

  {
    s = buf;
    mg_asprintf(&s, sizeof(buf), "%3d", 123);
    ASSERT(s == buf);
    ASSERT(strcmp(buf, "123") == 0);
    mg_asprintf(&s, sizeof(buf), "%.*s", 7, "a%40b.c");
    ASSERT(s == buf);
    ASSERT(strcmp(buf, "a%40b.c") == 0);
  }

  ASSERT(mg_to64(mg_str("-9223372036854775809")) == 0);
  ASSERT(mg_to64(mg_str("9223372036854775800")) == 0);
  ASSERT(mg_to64(mg_str("9223372036854775700")) > 0);
}

static void test_crc32(void) {
  //  echo -n aaa | cksum -o3
  ASSERT(mg_crc32(0, 0, 0) == 0);
  ASSERT(mg_crc32(0, "a", 1) == 3904355907);
  ASSERT(mg_crc32(0, "abc", 3) == 891568578);
  ASSERT(mg_crc32(mg_crc32(0, "ab", 2), "c", 1) == 891568578);
}

#define LONG_CHUNK "chunk with length taking up more than two hex digits"

// Streaming server event handler.
// Send one chunk immediately, then drain, then send two chunks in one go
static void eh2(struct mg_connection *c, int ev, void *ev_data, void *fn_data) {
  if (ev == MG_EV_HTTP_MSG) {
    mg_printf(c, "HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n");
    mg_http_printf_chunk(c, LONG_CHUNK);
    c->label[0] = 1;
  } else if (ev == MG_EV_POLL) {
    if (c->label[0] > 0 && c->send.len == 0) c->label[0]++;
    if (c->label[0] > 10 && c->label[0] != 'x') {
      mg_http_printf_chunk(c, "chunk 1");
      mg_http_printf_chunk(c, "chunk 2");
      mg_http_printf_chunk(c, "");
      c->label[0] = 'x';
    }
  }
  (void) ev_data;
  (void) fn_data;
}

// Non-streaming client event handler. Make sure we've got full body
static void eh3(struct mg_connection *c, int ev, void *ev_data, void *fn_data) {
  if (ev == MG_EV_CONNECT) {
    mg_printf(c, "GET / HTTP/1.0\n\n");
  } else if (ev == MG_EV_HTTP_MSG) {
    struct mg_http_message *hm = (struct mg_http_message *) ev_data;
    // MG_INFO(("----> [%.*s]", (int) hm->body.len, hm->body.ptr));
    c->is_closing = 1;
    *(uint32_t *) fn_data = mg_crc32(0, hm->body.ptr, hm->body.len);
  }
}

// Streaming client event handler. Make sure we've got all chunks
static void eh4(struct mg_connection *c, int ev, void *ev_data, void *fn_data) {
  uint32_t *crc = (uint32_t *) c->label;
  if (ev == MG_EV_CONNECT) {
    mg_printf(c, "GET / HTTP/1.0\n\n");
  } else if (ev == MG_EV_HTTP_CHUNK) {
    struct mg_http_message *hm = (struct mg_http_message *) ev_data;
    *crc = mg_crc32(*crc, hm->chunk.ptr, hm->chunk.len);
  } else if (ev == MG_EV_HTTP_MSG) {
    struct mg_http_message *hm = (struct mg_http_message *) ev_data;
    *crc = mg_crc32(*crc, hm->body.ptr, hm->body.len);
    // MG_INFO(("MSG [%.*s]", (int) hm->body.len, hm->body.ptr));
    c->is_closing = 1;
    *(uint32_t *) fn_data = *crc;
  }
  (void) ev_data;
}

// Streaming client event handler. Delete chunks as they arrive
static void eh5(struct mg_connection *c, int ev, void *ev_data, void *fn_data) {
  uint32_t *crc = (uint32_t *) c->label;
  if (ev == MG_EV_CONNECT) {
    mg_printf(c, "GET / HTTP/1.0\n\n");
  } else if (ev == MG_EV_HTTP_CHUNK) {
    struct mg_http_message *hm = (struct mg_http_message *) ev_data;
    *crc = mg_crc32(*crc, hm->chunk.ptr, hm->chunk.len);
    // MG_INFO(("CHUNK [%.*s]", (int) hm->chunk.len, hm->chunk.ptr));
    mg_http_delete_chunk(c, hm);
  } else if (ev == MG_EV_HTTP_MSG) {
    struct mg_http_message *hm = (struct mg_http_message *) ev_data;
    *crc = mg_crc32(*crc, hm->chunk.ptr, hm->chunk.len);
    c->is_closing = 1;
    *(uint32_t *) fn_data = *crc;
    // MG_INFO(("MSG [%.*s]", (int) hm->body.len, hm->body.ptr));
  }
  (void) ev_data;
}

static void test_http_chunked(void) {
  struct mg_mgr mgr;
  const char *data, *url = "http://127.0.0.1:12344";
  uint32_t i, done = 0;
  mg_mgr_init(&mgr);
  mg_http_listen(&mgr, url, eh2, NULL);

  mg_http_connect(&mgr, url, eh3, &done);
  for (i = 0; i < 50 && done == 0; i++) mg_mgr_poll(&mgr, 1);
  ASSERT(i < 50);
  data = LONG_CHUNK "chunk 1chunk 2";
  ASSERT(done == mg_crc32(0, data, strlen(data)));

  done = 0;
  mg_http_connect(&mgr, url, eh4, &done);
  for (i = 0; i < 50 && done == 0; i++) mg_mgr_poll(&mgr, 1);
  data = LONG_CHUNK LONG_CHUNK "chunk 1chunk 2" LONG_CHUNK "chunk 1chunk 2";
  ASSERT(done == mg_crc32(0, data, strlen(data)));

  done = 0;
  mg_http_connect(&mgr, url, eh5, &done);
  for (i = 0; i < 50 && done == 0; i++) mg_mgr_poll(&mgr, 1);
  data = LONG_CHUNK "chunk 1chunk 2";
  ASSERT(done == mg_crc32(0, data, strlen(data)));

  mg_mgr_free(&mgr);
  ASSERT(mgr.conns == NULL);
}

#undef LONG_CHUNK

static void test_multipart(void) {
  struct mg_http_part part;
  size_t ofs;
  const char *s =
      "--xyz\r\n"
      "Content-Disposition: form-data; name=\"val\"\r\n"
      "\r\n"
      "abc\r\ndef\r\n"
      "--xyz\r\n"
      "Content-Disposition: form-data; name=\"foo\"; filename=\"a b.txt\"\r\n"
      "Content-Type: text/plain\r\n"
      "\r\n"
      "hello world\r\n"
      "\r\n"
      "--xyz--\r\n";
  ASSERT(mg_http_next_multipart(mg_str(""), 0, NULL) == 0);
  ASSERT((ofs = mg_http_next_multipart(mg_str(s), 0, &part)) > 0);
  ASSERT(mg_strcmp(part.name, mg_str("val")) == 0);
  // MG_INFO(("--> [%.*s]", (int) part.body.len, part.body.ptr));
  ASSERT(mg_strcmp(part.body, mg_str("abc\r\ndef")) == 0);
  ASSERT(part.filename.len == 0);
  ASSERT((ofs = mg_http_next_multipart(mg_str(s), ofs, &part)) > 0);
  ASSERT(mg_strcmp(part.name, mg_str("foo")) == 0);
  // MG_INFO(("--> [%.*s]", (int) part.filename.len, part.filename.ptr));
  ASSERT(mg_strcmp(part.filename, mg_str("a b.txt")) == 0);
  ASSERT(mg_strcmp(part.body, mg_str("hello world\r\n")) == 0);
  ASSERT(mg_http_next_multipart(mg_str(s), ofs, &part) == 0);
}

static void eh7(struct mg_connection *c, int ev, void *ev_data, void *fn_data) {
  if (ev == MG_EV_HTTP_MSG) {
    struct mg_http_message *hm = (struct mg_http_message *) ev_data;
    struct mg_http_serve_opts sopts;
    memset(&sopts, 0, sizeof(sopts));
    sopts.root_dir = "/";
    sopts.fs = &mg_fs_packed;
    mg_http_serve_dir(c, hm, &sopts);
  }
  (void) ev_data, (void) fn_data;
}

static void test_packed(void) {
  struct mg_mgr mgr;
  const char *url = "http://127.0.0.1:12351";
  char buf[FETCH_BUF_SIZE] = "",
       *data = mg_file_read(&mg_fs_posix, "Makefile", NULL);
  mg_mgr_init(&mgr);
  mg_http_listen(&mgr, url, eh7, NULL);

  // Load top level file directly
  ASSERT(fetch(&mgr, buf, url, "GET /Makefile HTTP/1.0\n\n") == 200);
  ASSERT(cmpbody(buf, data) == 0);
  free(data);

  // Load file deeper in the FS tree directly
  data = mg_file_read(&mg_fs_posix, "src/ssi.h", NULL);
  ASSERT(fetch(&mgr, buf, url, "GET /src/ssi.h HTTP/1.0\n\n") == 200);
  ASSERT(cmpbody(buf, data) == 0);
  free(data);

  // List root dir
  ASSERT(fetch(&mgr, buf, url, "GET / HTTP/1.0\n\n") == 200);
  // printf("--------\n%s\n", buf);

  // List nested dir
  ASSERT(fetch(&mgr, buf, url, "GET /test HTTP/1.0\n\n") == 301);
  ASSERT(fetch(&mgr, buf, url, "GET /test/ HTTP/1.0\n\n") == 200);
  // printf("--------\n%s\n", buf);

  mg_mgr_free(&mgr);
  ASSERT(mgr.conns == NULL);
}

static void eh6(struct mg_connection *c, int ev, void *ev_data, void *fn_data) {
  if (ev == MG_EV_READ) *(int *) fn_data = 1;
  (void) c, (void) ev_data;
}

static void test_pipe(void) {
  struct mg_mgr mgr;
  struct mg_connection *c;
  int i, done = 0;
  mg_mgr_init(&mgr);
  ASSERT((c = mg_mkpipe(&mgr, eh6, (void *) &done)) != NULL);
  mg_mgr_wakeup(c, "", 1);
  for (i = 0; i < 10 && done == 0; i++) mg_mgr_poll(&mgr, 1);
  ASSERT(done == 1);
  mg_mgr_free(&mgr);
  ASSERT(mgr.conns == NULL);
}

static void u1(struct mg_connection *c, int ev, void *ev_data, void *fn_data) {
  if (ev == MG_EV_CONNECT) {
    ((int *) fn_data)[0] += 1;
    mg_send(c, "hi", 2);
  } else if (ev == MG_EV_WRITE) {
    ((int *) fn_data)[0] += 100;
  } else if (ev == MG_EV_READ) {
    ((int *) fn_data)[0] += 10;
    mg_iobuf_free(&c->recv);
  }
  (void) ev_data;
}

static void test_udp(void) {
  struct mg_mgr mgr;
  const char *url = "udp://127.0.0.1:12353";
  int i, done = 0;
  mg_mgr_init(&mgr);
  mg_listen(&mgr, url, u1, (void *) &done);
  mg_connect(&mgr, url, u1, (void *) &done);
  for (i = 0; i < 5; i++) mg_mgr_poll(&mgr, 1);
  // MG_INFO(("%d", done));
  ASSERT(done == 111);
  mg_mgr_free(&mgr);
  ASSERT(mgr.conns == NULL);
}

static void test_check_ip_acl(void) {
  uint32_t ip = mg_htonl(0x01020304);
  ASSERT(mg_check_ip_acl(mg_str(NULL), ip) == 1);
  ASSERT(mg_check_ip_acl(mg_str(""), ip) == 1);
  ASSERT(mg_check_ip_acl(mg_str("invalid"), ip) == -1);
  ASSERT(mg_check_ip_acl(mg_str("+hi"), ip) == -2);
  ASSERT(mg_check_ip_acl(mg_str("+//"), ip) == -2);
  ASSERT(mg_check_ip_acl(mg_str("-0.0.0.0/0"), ip) == 0);
  ASSERT(mg_check_ip_acl(mg_str("-0.0.0.0/0,+1.0.0.0/8"), ip) == 1);
  ASSERT(mg_check_ip_acl(mg_str("-0.0.0.0/0,+1.2.3.4"), ip) == 1);
  ASSERT(mg_check_ip_acl(mg_str("-0.0.0.0/0,+1.0.0.0/16"), ip) == 0);
}

static void w3(struct mg_connection *c, int ev, void *ev_data, void *fn_data) {
  // MG_INFO(("ev %d", ev));
  if (ev == MG_EV_WS_OPEN) {
    mg_ws_send(c, "hi there!", 9, WEBSOCKET_OP_TEXT);
  } else if (ev == MG_EV_WS_MSG) {
    struct mg_ws_message *wm = (struct mg_ws_message *) ev_data;
    ASSERT(mg_strcmp(wm->data, mg_str("lebowski")) == 0);
    ((int *) fn_data)[0]++;
  } else if (ev == MG_EV_CLOSE) {
    ((int *) fn_data)[0] += 10;
  }
}

static void w2(struct mg_connection *c, int ev, void *ev_data, void *fn_data) {
  struct mg_str msg = mg_str_n("lebowski", 8);
  if (ev == MG_EV_HTTP_MSG) {
    mg_ws_upgrade(c, (struct mg_http_message *) ev_data, NULL);
  } else if (ev == MG_EV_WS_OPEN) {
    mg_ws_send(c, "x", 1, WEBSOCKET_OP_PONG);
  } else if (ev == MG_EV_POLL && c->is_websocket) {
    size_t ofs, n = (size_t) fn_data;
    if (n < msg.len) {
      // Send "msg" char by char using fragmented frames
      // mg_ws_send() sets the FIN flag in the WS header. Clean it
      // to send fragmented packet. Insert PONG messages between frames
      uint8_t op = n == 0 ? WEBSOCKET_OP_TEXT : WEBSOCKET_OP_CONTINUE;
      mg_ws_send(c, ":->", 3, WEBSOCKET_OP_PING);
      ofs = c->send.len;
      mg_ws_send(c, &msg.ptr[n], 1, op);
      if (n < msg.len - 1) c->send.buf[ofs] = op;  // Clear FIN flag
      c->fn_data = (void *) (n + 1);               // Point to the next char
    } else {
      mg_ws_send(c, "", 0, WEBSOCKET_OP_CLOSE);
    }
  } else if (ev == MG_EV_WS_MSG) {
    struct mg_ws_message *wm = (struct mg_ws_message *) ev_data;
    ASSERT(mg_strcmp(wm->data, mg_str("hi there!")) == 0);
  }
}

static void test_ws_fragmentation(void) {
  const char *url = "ws://localhost:12357/ws";
  struct mg_mgr mgr;
  int i, done = 0;

  mg_mgr_init(&mgr);
  ASSERT(mg_http_listen(&mgr, url, w2, NULL) != NULL);
  mg_ws_connect(&mgr, url, w3, &done, "%s", "Sec-WebSocket-Protocol: echo\r\n");
  for (i = 0; i < 25; i++) mg_mgr_poll(&mgr, 1);
  // MG_INFO(("--> %d", done));
  ASSERT(done == 11);

  mg_mgr_free(&mgr);
  ASSERT(mgr.conns == NULL);
}

static void h7(struct mg_connection *c, int ev, void *ev_data, void *fn_data) {
  if (ev == MG_EV_HTTP_MSG) {
    struct mg_http_message *hm = (struct mg_http_message *) ev_data;
    struct mg_http_serve_opts opts;
    memset(&opts, 0, sizeof(opts));
    opts.root_dir = "./test/data,/foo=./src";
    mg_http_serve_dir(c, hm, &opts);
  }
  (void) fn_data;
}

static void test_rewrites(void) {
  char buf[FETCH_BUF_SIZE];
  const char *url = "http://LOCALHOST:12358";
  const char *expected = "#define MG_VERSION \"" MG_VERSION "\"\n";
  struct mg_mgr mgr;
  mg_mgr_init(&mgr);
  ASSERT(mg_http_listen(&mgr, url, h7, NULL) != NULL);
  ASSERT(fetch(&mgr, buf, url, "GET /a.txt HTTP/1.0\n\n") == 200);
  ASSERT(cmpbody(buf, "hello\n") == 0);
  ASSERT(fetch(&mgr, buf, url, "GET /foo/version.h HTTP/1.0\n\n") == 200);
  ASSERT(cmpbody(buf, expected) == 0);
  ASSERT(fetch(&mgr, buf, url, "GET /foo HTTP/1.0\n\n") == 301);
  ASSERT(fetch(&mgr, buf, url, "GET /foo/ HTTP/1.0\n\n") == 200);
  // printf("-->[%s]\n", buf);
  // exit(0);
  mg_mgr_free(&mgr);
  ASSERT(mgr.conns == NULL);
}

static void test_get_header_var(void) {
  struct mg_str empty = mg_str(""), bar = mg_str("bar"), baz = mg_str("baz");
  struct mg_str header = mg_str("Digest foo=\"bar\", blah,boo=baz, x=\"yy\"");
  struct mg_str yy = mg_str("yy");
  // struct mg_str x = mg_http_get_header_var(header, mg_str("x"));
  // MG_INFO(("--> [%d] [%d]", (int) x.len, yy.len));
  ASSERT(mg_strcmp(empty, mg_http_get_header_var(empty, empty)) == 0);
  ASSERT(mg_strcmp(empty, mg_http_get_header_var(header, empty)) == 0);
  ASSERT(mg_strcmp(empty, mg_http_get_header_var(header, mg_str("fooo"))) == 0);
  ASSERT(mg_strcmp(empty, mg_http_get_header_var(header, mg_str("fo"))) == 0);
  ASSERT(mg_strcmp(empty, mg_http_get_header_var(header, mg_str("blah"))) == 0);
  ASSERT(mg_strcmp(bar, mg_http_get_header_var(header, mg_str("foo"))) == 0);
  ASSERT(mg_strcmp(baz, mg_http_get_header_var(header, mg_str("boo"))) == 0);
  ASSERT(mg_strcmp(yy, mg_http_get_header_var(header, mg_str("x"))) == 0);
}

int main(void) {
  const char *debug_level = getenv("V");
  if (debug_level == NULL) debug_level = "3";
  mg_log_set(debug_level);

  test_str();
  test_globmatch();
  test_get_header_var();
  test_rewrites();
  test_check_ip_acl();
  test_udp();
  test_pipe();
  test_packed();
  test_crc32();
  test_multipart();
  test_http_chunked();
  test_http_parse();
  test_util();
  test_sntp();
  test_dns();
  test_timer();
  test_url();
  test_iobuf();
  test_commalist();
  test_base64();
  test_http_get_var();
  test_tls();
  test_ws();
  test_ws_fragmentation();
  test_http_client();
  test_http_server();
  test_http_no_content_length();
  test_http_pipeline();
  test_http_range();
  test_mqtt();
  printf("SUCCESS. Total tests: %d\n", s_num_tests);
  return EXIT_SUCCESS;
}
