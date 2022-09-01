#ifndef _KCP_DEMO_H

#define _KCP_DEMO_H

#include "ev.h"
#include "ikcp.h"
#include <arpa/inet.h>
#include <bits/types/struct_timespec.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#define __DEBUG 0

const int UDP_CLI_PORT = 54321;
const int UDP_SRV_PORT = 12345;

const int TIMES = 1000;

const int MAXLINE = 1024;

const socklen_t SIZE = sizeof(struct sockaddr);

const int WND_SIZE = 8192;

const int DATA_SIZE = 16;

struct udp_holder {
  ikcpcb *kcp;
  int *send_fd;
  int *recv_fd;
  struct sockaddr_in *send_addr;
  struct sockaddr_in *recv_addr;
  char *name;
  int active;
  ev_io *io_w;
  ev_timer *tm_w;
  void (*kcp_recv_func)(struct ev_loop *loop, struct udp_holder *holder);
  int count;
};

typedef struct udp_holder udp_holder;

struct timestamp {
  IUINT64 sec;
  IUINT64 nsec;
  IUINT32 curr_ms;
};

typedef struct timestamp timestamp;

const int max_fetch_times = 100;

static long long padding1[7];
static long long widx = 0;
static long long padding2[7];
static long long ridx = 0;
static long long padding3[7];

static char *kcp_buf;
static int *kcp_buf_len;

static char *udp_buf;

inline void switch_char(char *p, int idx1, int idx2) {
  char ch = p[idx1];
  p[idx1] = p[idx2];
  p[idx2] = ch;
}

in_addr_t conv(const char *ip) {
  struct in_addr addr;
  inet_aton(ip, &addr);
#if __DEBUG
  printf("remote IP: %s, %x\n", inet_ntoa(*((struct in_addr *)&addr.s_addr)),
         addr.s_addr);
#endif
  in_addr_t ip_addr = addr.s_addr;
  char *p = (char *)&ip_addr;
  switch_char(p, 0, 3);
  switch_char(p, 1, 2);
  return ip_addr;
}

/* get system time */
static inline timestamp iclockX() {
  struct timespec ts;
  timestamp tsx;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  tsx.sec = ts.tv_sec;
  tsx.nsec = ts.tv_nsec;
  IINT64 timestamp = ((IINT64)ts.tv_sec) * 1000 + (ts.tv_nsec / 1000);
  tsx.curr_ms = (IUINT32)(timestamp & 0xfffffffful);
  return tsx;
}

/* encode 64 bits unsigned int (lsb) */
static inline char *encode64u(char *p, IUINT64 l) {
#if IWORDS_BIG_ENDIAN || IWORDS_MUST_ALIGN
  *(unsigned char *)(p + 0) = (unsigned char)((l >> 0) & 0xff);
  *(unsigned char *)(p + 1) = (unsigned char)((l >> 8) & 0xff);
  *(unsigned char *)(p + 2) = (unsigned char)((l >> 16) & 0xff);
  *(unsigned char *)(p + 3) = (unsigned char)((l >> 24) & 0xff);
  *(unsigned char *)(p + 4) = (unsigned char)((l >> 32) & 0xff);
  *(unsigned char *)(p + 5) = (unsigned char)((l >> 40) & 0xff);
  *(unsigned char *)(p + 6) = (unsigned char)((l >> 48) & 0xff);
  *(unsigned char *)(p + 7) = (unsigned char)((l >> 56) & 0xff);
#else
  memcpy(p, &l, 8);
#endif
  p += 8;
  return p;
}

/* decode 64 bits unsigned int (lsb) */
static inline const char *decode64u(const char *p, IUINT64 *l) {
#if IWORDS_BIG_ENDIAN || IWORDS_MUST_ALIGN
  *l = *(const unsigned char *)(p + 7);
  *l = *(const unsigned char *)(p + 6) + (*l << 8);
  *l = *(const unsigned char *)(p + 5) + (*l << 8);
  *l = *(const unsigned char *)(p + 4) + (*l << 8);
  *l = *(const unsigned char *)(p + 3) + (*l << 8);
  *l = *(const unsigned char *)(p + 2) + (*l << 8);
  *l = *(const unsigned char *)(p + 1) + (*l << 8);
  *l = *(const unsigned char *)(p + 0) + (*l << 8);
#else
  memcpy(l, p, 8);
#endif
  p += 8;
  return p;
}

int udp_send(const char *buf, int len, ikcpcb *kcp, void *user) {
  udp_holder *holder = (udp_holder *)user;
#if __DEBUG
  printf("%s try to send var udp: %d bytes\n", holder->name, len);
#endif
  int rt = sendto(*(holder->send_fd), buf, len, 0,
                  (struct sockaddr *)(holder->send_addr), SIZE);
  if (rt < 0) {
#if __DEBUG
    printf("%s sendto failed: %d\n", holder->name, rt);
#endif
  }
  return rt;
}

void fill_buf(udp_holder *holder, const char *buf, size_t len) {
  if (widx < ridx + WND_SIZE) {
    int idx = (int)(widx++ % WND_SIZE);
    char *ptr = kcp_buf + idx * MAXLINE;
    memcpy(ptr, buf, len);
    kcp_buf_len[idx] = len;
  } else {
#if __DEBUG
    printf("buf is full: %lld/%lld\n", widx, ridx);
#endif
  }
}

void udp_recv_cb(struct ev_loop *loop, ev_io *w, int revents) {
  if (EV_ERROR & revents) {
#if __DEBUG
    printf("read got invalid event...: %d\n", revents);
#endif
    return;
  }
  udp_holder *holder = (udp_holder *)w->data;
  ev_timer_again(loop, holder->tm_w);
  ikcpcb *kcp = holder->kcp;
  int *udp_recv_fd = holder->recv_fd;
  struct sockaddr_in cli_sock;
  socklen_t udp_addr_len = SIZE;
  int len = recvfrom(*udp_recv_fd, udp_buf, MAXLINE, 0,
                     (struct sockaddr *)&cli_sock, &udp_addr_len);
  if (len < 0) {
#if __DEBUG
    printf("%s recvfrom failed: %d\n", holder->name, len);
#endif
    return;
  }
  ev_timer_again(loop, holder->tm_w);
#if __DEBUG
  printf("%s received %d bytes\n", holder->name, len);
#endif
  fill_buf(holder, udp_buf, len);
  (holder->kcp_recv_func)(loop, holder);
}

void timeout_cb(struct ev_loop *loop, ev_timer *w, int revents) {
  if (EV_ERROR & revents) {
#if __DEBUG
    printf("timeout got invalid event...: %d\n", revents);
#endif
    return;
  }
  udp_holder *holder = (udp_holder *)w->data;
  (holder->kcp_recv_func)(loop, holder);
}

void udp_sock(int mode, int *udp_fd, struct sockaddr_in *udp_addr,
              in_addr_t addr, int port) {
  // 使用函数socket()，生成套接字文件描述符；
  if ((*udp_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
    perror("socket() error");
    exit(1);
  }

  // 通过struct sockaddr_in 结构设置服务器地址和监听端口；
  bzero(udp_addr, sizeof(*udp_addr));
  udp_addr->sin_family = AF_INET;
  udp_addr->sin_addr.s_addr = htonl(addr);
  udp_addr->sin_port = htons(port);

  if (mode) {
#if __DEBUG
    printf("try to bind sock: fd_%d->%s:%d\n", *udp_fd,
           inet_ntoa(*((struct in_addr *)&addr)), port);
#endif
    if (bind(*udp_fd, (struct sockaddr *)udp_addr, SIZE)) {
      perror("bind() error");
      exit(1);
    }
  } else {
#if __DEBUG
    printf("try to open remote sock: fd_%d->%s:%d\n", *udp_fd,
           inet_ntoa(*((struct in_addr *)&addr)), port);
#endif
  }
}

int fetch_buf(ikcpcb *kcp) {
  int count = 0;
  int idx;
  char *ptr;
  int len;
  int rt;
  int rc = 0;
  while (widx > ridx && count < max_fetch_times) {
#if __DEBUG
    printf("read from buf: %lld/%lld\n", widx, ridx);
#endif
    idx = (int)(ridx % WND_SIZE);
    ptr = kcp_buf + idx * MAXLINE;
    len = kcp_buf_len[idx];
    rt = ikcp_input(kcp, ptr, len);
    rc = 1;
    ridx++;
    count++;
    if (rt < 0) {
#if __DEBUG
      printf("input to kcp failed: %d bytes, rt: %d\n", len, rt);
#endif
      break;
    }
#if __DEBUG
    printf("input to kcp: %d bytes, rt: %d\n", len, rt);
#endif
  }
  return rc;
}

int check_quit(udp_holder *holder) {
#if __DEBUG
  printf("count:[%d][%d]\n", holder->recv_cnt, holder->send_cnt);
#endif
  ikcpcb *kcp = holder->kcp;
  if (kcp->nsnd_buf > 0 && kcp->nsnd_que > 0 && kcp->nrcv_buf > 0 &&
      kcp->nrcv_que > 0) {
    return 0;
  }
  if (holder->count >= TIMES) {
    holder->active = 0;
    shutdown(*holder->recv_fd, SHUT_RDWR);
    return 1;
  }
  return 0;
}

void test_kcp(char *name, int listen_port, in_addr_t remote_addr,
              int remote_port,
              void (*kcp_recv_func)(struct ev_loop *loop,
                                    struct udp_holder *holder)) {
  udp_buf = malloc(MAXLINE * sizeof(char));
  kcp_buf = malloc(WND_SIZE * MAXLINE * sizeof(char));
  kcp_buf_len = malloc(WND_SIZE * sizeof(int));
  int udp_send_fd;
  int udp_recv_fd;
  struct sockaddr_in udp_send_addr;
  struct sockaddr_in udp_recv_addr;
  udp_holder holder;
  holder.kcp_recv_func = kcp_recv_func;
  // setup kcp
  ikcpcb *kcp = ikcp_create((unsigned int)0x98badcfe, (void *)&holder);
  ikcp_setoutput(kcp, udp_send);
  ikcp_nodelay(kcp, 1, 5, 2, 1);
  ikcp_wndsize(kcp, WND_SIZE, WND_SIZE);
  // setup udp
  udp_sock(0, &udp_send_fd, &udp_send_addr, remote_addr, remote_port);
  udp_sock(1, &udp_recv_fd, &udp_recv_addr, INADDR_ANY, listen_port);
  // fill into holder
  holder.send_fd = &udp_send_fd;
  holder.recv_fd = &udp_recv_fd;
  holder.send_addr = &udp_send_addr;
  holder.recv_addr = &udp_recv_addr;
  holder.name = name;
  holder.kcp = kcp;
  holder.active = 1;
  holder.count = 0;
  // setup ev_watcher
  ev_io io_w;
  ev_timer tm_w;
  io_w.data = &holder;
  tm_w.data = &holder;
  holder.io_w = &io_w;
  holder.tm_w = &tm_w;
  // init ev_loop
  struct ev_loop *loop = ev_loop_new(EVFLAG_NOENV | EVBACKEND_EPOLL);
  ev_io_init(&io_w, udp_recv_cb, udp_recv_fd, EV_READ);
  ev_io_start(loop, &io_w);
  ev_timer_init(&tm_w, timeout_cb, 0.1, 0.001);
  ev_timer_start(loop, &tm_w);
  ev_run(loop, 0);
  ev_loop_destroy(loop);
  // release
  ikcp_release(kcp);
  free(kcp_buf);
  free(kcp_buf_len);
  free(udp_buf);
}

long double calc_avg(const IUINT64 *store, int size) {
  double sum = 0.0;
  for (int i = 0; i < size; i++) {
    sum += store[i];
  }
  return sum / size;
}

int compare(const void *a, const void *b) {
  IUINT64 *la = (IUINT64 *)a;
  IUINT64 *lb = (IUINT64 *)b;
  return *la == *lb ? 0 : (*la > *lb ? 1 : -1);
}

void printLatency(const IUINT64 *store) {
  qsort((void *)store, TIMES, sizeof(IUINT64), compare);
  double avg = calc_avg(store, TIMES);
  IUINT64 min = store[0];
  IUINT64 max = store[TIMES - 1];
  printf("latency detail:\n");
  printf("------\n");
  printf("avg     : % 6.2fns\n\n", avg);
  printf("min     : %9lluns\n", min);
  int prec_lines[] = {1, 5, 25, 50, 75, 95, 99};
  int prec_line;
  for (int i = 0; i < 7; i++) {
    prec_line = prec_lines[i];
    printf("line %02d%%: %9lluns\n", prec_line,
           store[(int)(TIMES * prec_line / 100.0)]);
  }
  printf("max     : %9lluns\n", max);
  printf("------\n");
}

#endif
