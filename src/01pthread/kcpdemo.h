#ifndef _KCP_DEMO_H

#define _KCP_DEMO_H

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
};

typedef struct udp_holder udp_holder;

struct timestamp {
  IUINT64 sec;
  IUINT64 nsec;
  IUINT32 curr_ms;
};

typedef struct timestamp timestamp;

pthread_mutex_t lock;

const int max_fetch_times = 100;

long long padding1[7];
long long volatile widx = 0;
long long padding2[7];
long long volatile ridx = 0;
long long padding3[7];

long *recv_buf_ptr;
int *recv_buf_len;

#define LOCK pthread_mutex_lock(&lock)
#define UNLOCK pthread_mutex_unlock(&lock)

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
  int quit = 0;
  while (holder->active) {
    LOCK;
    if (widx < ridx + WND_SIZE) {
      char *ptr = malloc(len);
      memcpy(ptr, buf, len);
      int idx = (int)(widx++ % WND_SIZE);
      recv_buf_ptr[idx] = (long)ptr;
      recv_buf_len[idx] = len;
      quit = 1;
    }
    UNLOCK;
    if (quit) {
      break;
    }
#if __DEBUG
    printf("buf is full: %lld/%lld\n", widx, ridx);
#endif
  }
}

void udp_recv0(udp_holder *holder) {
  char buf[MAXLINE];
  ikcpcb *kcp = holder->kcp;
  int *udp_recv_fd = holder->recv_fd;
  struct sockaddr_in cli_sock;
  socklen_t udp_addr_len = SIZE;
  int len;
  while (holder->active) {
    len = recvfrom(*udp_recv_fd, buf, MAXLINE, 0, (struct sockaddr *)&cli_sock,
                   &udp_addr_len);
    if (!holder->active) {
      break;
    }
    if (len < 0) {
#if __DEBUG
      printf("%s recvfrom failed: %d\n", holder->name, len);
#endif
      continue;
    }
#if __DEBUG
    printf("%s received %d bytes\n", holder->name, len);
#endif
    fill_buf(holder, buf, len);
  }
}

void *udp_recv(void *args) {
  udp_holder *holder = (udp_holder *)args;
  udp_recv0(holder);
#if __DEBUG
  printf("%s thread udp recv quit\n", holder->name);
#endif
  pthread_exit(NULL);
}

void udp_sock(int mode, int *udp_fd, struct sockaddr_in *udp_addr, int port) {
  // 使用函数socket()，生成套接字文件描述符；
  if ((*udp_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
    perror("socket() error");
    exit(1);
  }

  // 通过struct sockaddr_in 结构设置服务器地址和监听端口；
  bzero(udp_addr, sizeof(*udp_addr));
  udp_addr->sin_family = AF_INET;
  udp_addr->sin_addr.s_addr = htonl(mode ? INADDR_ANY : INADDR_LOOPBACK);
  udp_addr->sin_port = htons(port);

  if (mode) {
#if __DEBUG
    printf("try to bind\n");
#endif
    if (bind(*udp_fd, (struct sockaddr *)udp_addr, SIZE)) {
      perror("bind() error");
      exit(1);
    }
  }
#if __DEBUG
  printf("udp sock init, done. fd: %d\n", *udp_fd);
#endif
}

void fetch_buf(ikcpcb *kcp) {
  LOCK;
  int count = 0;
  int idx;
  char *ptr;
  int len;
  while (widx > ridx && count < max_fetch_times) {
#if __DEBUG
    printf("read from buf: %lld/%lld\n", widx, ridx);
#endif
    idx = (int)(ridx++ % WND_SIZE);
    ptr = (char *)recv_buf_ptr[idx];
    len = recv_buf_len[idx];
    ikcp_input(kcp, ptr, len);
    free(ptr);
    count++;
  }
  UNLOCK;
}

int check_quit(udp_holder *holder, int *count) {
  if (*count >= TIMES) {
    holder->active = 0;
    shutdown(*holder->recv_fd, SHUT_RDWR);
    return 1;
  }
  return 0;
}

void test_kcp(int mode, void *(*udp_func)(void *args),
              void *(*kcp_func)(void *args)) {
  recv_buf_ptr = malloc(WND_SIZE * sizeof(long));
  recv_buf_len = malloc(WND_SIZE * sizeof(int));
  int udp_send_fd;
  int udp_recv_fd;
  struct sockaddr_in udp_send_addr;
  struct sockaddr_in udp_recv_addr;
  udp_holder holder;
  // setup kcp
  ikcpcb *kcp = ikcp_create((unsigned int)0x98badcfe, (void *)&holder);
  ikcp_setoutput(kcp, udp_send);
  ikcp_nodelay(kcp, 1, 5, 2, 1);
  ikcp_wndsize(kcp, WND_SIZE, WND_SIZE);
  // setup udp
  udp_sock(0, &udp_send_fd, &udp_send_addr, mode ? UDP_SRV_PORT : UDP_CLI_PORT);
  udp_sock(1, &udp_recv_fd, &udp_recv_addr, mode ? UDP_CLI_PORT : UDP_SRV_PORT);
  // fill into holder
  holder.send_fd = &udp_send_fd;
  holder.recv_fd = &udp_recv_fd;
  holder.send_addr = &udp_send_addr;
  holder.recv_addr = &udp_recv_addr;
  holder.name = mode ? "client" : "server";
  holder.kcp = kcp;
  holder.active = 1;
  // setup threads
  pthread_t th_udp;
  pthread_t th_kcp;
  void *status_udp;
  void *status_kcp;
  pthread_create(&th_udp, NULL, udp_func, &holder);
  pthread_create(&th_kcp, NULL, kcp_func, &holder);
  pthread_join(th_kcp, &status_kcp);
  pthread_join(th_udp, &status_udp);
  // release
  ikcp_release(kcp);
  free(recv_buf_ptr);
  free(recv_buf_len);
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
  for (int i = 0; i < 7; i++) {
    int prec_line = prec_lines[i];
    printf("line %02d%%: %9lluns\n", prec_line,
           store[(int)(TIMES * prec_line / 100.0)]);
  }
  printf("max     : %9lluns\n", max);
  printf("------\n");
}

#endif
