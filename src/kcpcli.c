#include "kcpdemo.h"

void loop_send_kcp0(udp_holder *holder) {
  ikcpcb *kcp = holder->kcp;
  int rt = 0;
  int count = 0;
  char buf[DATA_SIZE];
  char *p;
  timestamp tsx;
  while (1) {
    usleep(1000);
    fetch_buf(kcp);
    tsx = iclockX();
    ikcp_update(kcp, tsx.currentMills);
    if (rt >= WND_SIZE) {
#if __DEBUG
      printf("%s too many wait msgs: %d packages\n", holder->name, rt);
#endif
      usleep(10 * 1000);
      rt = ikcp_waitsnd(kcp);
      continue;
    }
    p = buf;
    p = encode64u(p, tsx.sec);
    p = encode64u(p, tsx.nsec);
    rt = ikcp_send(kcp, buf, DATA_SIZE);
    ikcp_flush(kcp);
    if (rt < 0) {
      continue;
    }
    rt = ikcp_waitsnd(kcp);
    ++count;
#if __DEBUG
    printf("%s sent: %d packages, %d packages waiting\n", holder->name, count,
           rt);
#endif
    if (count >= TIMES) {
      holder->active = 0;
      shutdown(*holder->recv_fd, SHUT_RDWR);
      break;
    }
  }
}

void *loop_send_kcp(void *args) {
  udp_holder *holder = (udp_holder *)args;
  loop_send_kcp0(holder);
#if __DEBUG
  printf("%s thread kcp quit\n", holder->name);
#endif
  pthread_exit(NULL);
}

int main(int argc, char *argv[]) {
  test_kcp(1, udp_recv, loop_send_kcp);
  return 0;
}
