#include "ikcp.h"
#include "kcpdemo.h"

void loop_send_kcp(struct ev_loop *loop, udp_holder *holder) {
  ikcpcb *kcp = holder->kcp;
  int rt = ikcp_waitsnd(kcp);
  if (rt >= WND_SIZE) {
#if __DEBUG
    printf("%s too many wait msgs: %d packages\n", holder->name, rt);
#endif
    return;
  }
  char buf[DATA_SIZE];
  char *p;
  timestamp tsx;
  // 获取底层数据包
  fetch_buf(kcp);
  // 获取时间戳并更新kcp
  tsx = iclockX();
  ikcp_update(kcp, tsx.curr_ms);
  // 获得时间戳并发送
  p = buf;
  p = encode64u(p, tsx.sec);
  p = encode64u(p, tsx.nsec);
  rt = ikcp_send(kcp, buf, DATA_SIZE);
  // 强制刷新，确保第一时间发出，降低延时
  if (rt < 0) {
    return;
  }
  ikcp_flush(kcp);
  rt = ikcp_waitsnd(kcp);
  ++holder->count;
#if __DEBUG
  printf("%s sent: %d packages, %d packages waiting\n", holder->name,
         holder->count, rt);
#endif
  // 校验退出
  if (check_quit(holder)) {
    // this causes all nested ev_run's to stop iterating
    ev_break(loop, EVBREAK_ALL);
  }
}

int main(int argc, char *argv[]) {
  test_kcp(1, loop_send_kcp);
  return 0;
}
