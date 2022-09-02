#include "kcpdemo.h"

void loop_send_kcp0(udp_holder *holder) {
  ikcpcb *kcp = holder->kcp;
  int count = 0;
  char buf[DATA_SIZE];
  int rt;
  char *p;
  timestamp tsx;
  while (1) {
    // 固定休眠
    usleep(1000);
    // 获取底层数据包
    fetch_buf(kcp);
    // 获取时间戳并更新kcp
    tsx = iclockX();
    ikcp_update(kcp, tsx.curr_ms);
    if (rt >= WND_SIZE) {
#if __DEBUG
      printf("%s too many wait msgs: %d packages\n", holder->name, rt);
#endif
      usleep(10 * 1000);
      rt = ikcp_waitsnd(kcp);
      continue;
    }
    // 获得时间戳并发送
    p = buf;
    p = encode64u(p, tsx.sec);
    p = encode64u(p, tsx.nsec);
    rt = ikcp_send(kcp, buf, DATA_SIZE);
    // 强制刷新，确保第一时间发出，降低延时
    if (rt < 0) {
      continue;
    }
    ikcp_flush(kcp);
    rt = ikcp_waitsnd(kcp);
    ++count;
#if __DEBUG
    printf("%s sent: %d packages, %d packages waiting\n", holder->name, count,
           rt);
#endif
    // 校验退出
    if (check_quit(holder, &count)) {
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
