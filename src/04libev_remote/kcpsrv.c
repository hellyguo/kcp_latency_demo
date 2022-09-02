#include "kcpdemo.h"

static IUINT64 *latencyStore;
static char *buf;

void echo_kcp(struct ev_loop *loop, udp_holder *holder) {
  ikcpcb *kcp = holder->kcp;
  // 获取底层数据包
  fetch_buf(kcp);
  // 获取时间戳并更新kcp，从kcp读取数据
  timestamp tsx = iclockX();
  ikcp_update(kcp, tsx.curr_ms);
  int rt = ikcp_recv(kcp, buf, DATA_SIZE);
  if (rt < 0) {
#if __DEBUG
    printf("%s ikcp_recv failed: %d, %d|%d\n", holder->name, rt,
           holder->kcp->nrcv_buf, holder->kcp->nrcv_que);
#endif
    return;
  }
  // 解析时间戳并计算延时
  IUINT64 sec;
  IUINT64 nsec;
  const char *p = buf;
  p = decode64u(p, &sec);
  p = decode64u(p, &nsec);
  latencyStore[holder->count++] =
      (tsx.sec - sec) * 1000000000UL + tsx.nsec - nsec;
#if __DEBUG
  printf("%s received: %llu.%llu, current: %llu.%llu, %d packages\n",
         holder->name, sec, nsec, tsx.sec, tsx.nsec, holder->count);
#endif
  // 校验退出
  if (check_quit(holder)) {
    // this causes all nested ev_run's to stop iterating
    ev_break(loop, EVBREAK_ALL);
  }
}

int main(int argc, char *argv[]) {
  in_addr_t addr = conv(argv[1]);
  latencyStore = malloc(TIMES * sizeof(IUINT64));
  buf = malloc(DATA_SIZE * sizeof(char));
  test_kcp("kcpsrv", UDP_SRV_PORT, addr, UDP_CLI_PORT, echo_kcp);
  printLatency(latencyStore);
  free(latencyStore);
  free(buf);
  return 0;
}
