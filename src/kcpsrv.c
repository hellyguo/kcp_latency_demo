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
    tsx = iclockX();
    ikcp_update(kcp, tsx.currentMills);
    rt = ikcp_recv(kcp, buf, DATA_SIZE);
    if (rt < 0) {
      continue;
    }
    p = buf;
    p = decode64u(p, &sec);
    p = decode64u(p, &nsec);
    latencyStore[count++] = (tsx.sec - sec) * 1000000000L + (tsx.nsec - nsec);
#if __DEBUG
    printf("%s received: %lld.%lld, %d packages\n", holder->name, sec, nsec,
           count);
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
