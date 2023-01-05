#include "kcpdemo.h"

static IUINT64 *latencyStore;
static char *buf;
static unsigned long long last_sent_timestamp = 0;

void test_recv(udp_holder *holder, ikcpcb *kcp, timestamp *tsx) {
  int rt = ikcp_recv(kcp, buf, DATA_SIZE);
  if (rt > 0) {
    // 解析时间戳并计算延时
    IUINT64 sec;
    IUINT64 nsec;
    const char *p = buf;
    p = decode64u(p, &sec);
    decode64u(p, &nsec);
#if __DEBUG
    printf("%s received: %llu.%llu, current: %llu.%llu, %d packages\n",
           holder->name, sec, nsec, tsx->sec, tsx->nsec, holder->recv_cnt);
#endif
    latencyStore[holder->recv_cnt++] =
        (tsx->sec - sec) * 1000000000UL + tsx->nsec - nsec;
  }
}

void test_send(udp_holder *holder, ikcpcb *kcp) {
  if (holder->send_cnt >= TIMES) {
    return;
  }
  int rt = ikcp_waitsnd(kcp);
  if (rt >= WND_SIZE) {
#if __DEBUG
    printf("%s too many wait msgs: %d packages\n", holder->name, rt);
#endif
    return;
  }
  timestamp tsx = iclockX();
  unsigned long long current = tsx.sec * 1000000000UL + tsx.nsec;
  if (current - last_sent_timestamp < 1000000UL) {
    return;
  }
  last_sent_timestamp = current;
  // 获得时间戳并发送
  char *p = buf;
  p = encode64u(p, tsx.sec);
  encode64u(p, tsx.nsec);
  rt = ikcp_send(kcp, buf, DATA_SIZE);
  // 强制刷新，确保第一时间发出，降低延时
  if (rt < 0) {
    return;
  }
  ikcp_flush(kcp);
  holder->send_cnt++;
#if __DEBUG
  printf("%s sent: %d packages, %d packages waiting\n", holder->name,
         holder->send_cnt, rt);
#endif
}

void loop_send_kcp(struct ev_loop *loop, udp_holder *holder) {
  ikcpcb *kcp = holder->kcp;
  // 获取底层数据包
  fetch_buf(kcp);
  // 获取时间戳并更新kcp
  timestamp tsx = iclockX();
  ikcp_update(kcp, tsx.curr_ms);
  test_recv(holder, kcp, &tsx);
  test_send(holder, kcp);
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
  test_kcp("kcpcli", UDP_CLI_PORT, addr, UDP_SRV_PORT, loop_send_kcp);
  printLatency(latencyStore);
  free(latencyStore);
  free(buf);
  return 0;
}
