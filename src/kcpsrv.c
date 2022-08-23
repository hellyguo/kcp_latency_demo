#include "kcpdemo.h"

IUINT64 *latencyStore;

void echo_kcp0(udp_holder *holder) {
  ikcpcb *kcp = holder->kcp;
  char buf[DATA_SIZE];
  const char *p;
  int rt;
  int count = 0;
  IUINT64 sec;
  IUINT64 nsec;
  timestamp tsx;
  while (1) {
    fetch_buf(kcp);
    tsx = iclockX();
    ikcp_update(kcp, tsx.currentMills);
    rt = ikcp_recv(kcp, buf, DATA_SIZE);
    if (rt < 0) {
#if __DEBUG
      printf("%s ikcp_recv failed: %d, %d|%d\n", holder->name, rt,
             holder->kcp->nrcv_buf, holder->kcp->nrcv_que);
#endif
      continue;
    }
    p = buf;
    p = decode64u(p, &sec);
    p = decode64u(p, &nsec);
    latencyStore[count++] = (tsx.sec - sec) * 1000000000UL + tsx.nsec - nsec;
#if __DEBUG
    printf("%s received: %llu.%llu, current: %llu.%llu, %d packages\n",
           holder->name, sec, nsec, tsx.sec, tsx.nsec, count);
#endif
    if (count >= TIMES) {
      holder->active = 0;
      shutdown(*holder->recv_fd, SHUT_RDWR);
      break;
    }
  }
}

void *echo_kcp(void *args) {
  udp_holder *holder = (udp_holder *)args;
  echo_kcp0(holder);
#if __DEBUG
  printf("%s thread kcp quit\n", holder->name);
#endif
  pthread_exit(NULL);
}

int main(int argc, char *argv[]) {
  latencyStore = malloc(TIMES * sizeof(IUINT64));
  test_kcp(0, udp_recv, echo_kcp);
  printLatency(latencyStore);
  free(latencyStore);
  return 0;
}
