#include "kcpdemo.h"

static char *buf;

void loop_send_kcp(udp_holder *holder) {
  ikcpcb *kcp = holder->kcp;
  // 固定休眠
  usleep(1000);
  // 获取底层数据包
  fetch_buf(kcp);
  // 获取时间戳并更新kcp
  timestamp tsx = iclockX();
  ikcp_update(kcp, tsx.curr_ms);
  int rt = ikcp_waitsnd(kcp);
  if (rt >= WND_SIZE) {
#if __DEBUG
    printf("%s too many wait msgs: %d packages\n", holder->name, rt);
#endif
    usleep(10 * 1000);
    return;
  }
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
  ++holder->count;
#if __DEBUG
  rt = ikcp_waitsnd(kcp);
  printf("%s sent: %d packages, %d packages waiting\n", holder->name,
         holder->count, rt);
#else
  ikcp_waitsnd(kcp);
#endif
  // 校验退出
  if (check_quit(holder)) {
    printf("quit %s\n", holder->name);
  }
}

int main(int argc, char *argv[]) {
  buf = malloc(DATA_SIZE);
  test_kcp(1, loop_send_kcp);
  free(buf);
  return 0;
}
