#include "kcpdemo.h"

IUINT64 *latencyStore;

void echo_kcp0(udp_holder *holder) {
  ikcpcb *kcp = holder->kcp;
  char buf[DATA_SIZE];
  int count = 0;
  const char *p;
  int rt;
  IUINT64 sec;
  IUINT64 nsec;
  timestamp tsx;
  while (1) {
    // 获取底层数据包
    fetch_buf(kcp);
    // 获取时间戳并更新kcp，从kcp读取数据
    tsx = iclockX();
    ikcp_update(kcp, tsx.curr_ms);
    rt = ikcp_recv(kcp, buf, DATA_SIZE);
    if (rt < 0) {
#if __DEBUG
      printf("%s ikcp_recv failed: %d, %d|%d\n", holder->name, rt,
             holder->kcp->nrcv_buf, holder->kcp->nrcv_que);
#endif
      continue;
    }
    // 解析时间戳并计算延时
    p = buf;
    p = decode64u(p, &sec);
    p = decode64u(p, &nsec);
    latencyStore[count++] = (tsx.sec - sec) * 1000000000UL + tsx.nsec - nsec;
#if __DEBUG
    printf("%s received: %llu.%llu, current: %llu.%llu, %d packages\n",
           holder->name, sec, nsec, tsx.sec, tsx.nsec, count);
#endif
    // 校验退出
    if (check_quit(holder, &count)) {
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
