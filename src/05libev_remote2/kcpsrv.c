#include "kcpdemo.h"

static char *buf;

void echo_kcp(struct ev_loop *loop, udp_holder *holder) {
  ikcpcb *kcp = holder->kcp;
  // 获取底层数据包
  fetch_buf(kcp);
  // 获取时间戳并更新kcp，从kcp读取数据
  timestamp tsx = iclockX();
  ikcp_update(kcp, tsx.curr_ms);
  int len = ikcp_recv(kcp, buf, DATA_SIZE);
  if (len > 0) {
    holder->recv_cnt++;
    int rt = ikcp_send(kcp, buf, len);
    // 强制刷新，确保第一时间发出，降低延时
    if (rt < 0) {
#if __DEBUG
      printf("%s ikcp_send failed: %d, %d|%d\n", holder->name, len,
             holder->kcp->nsnd_buf, holder->kcp->nsnd_que);
#endif
      return;
    }
    ikcp_flush(kcp);
#if __DEBUG
    printf("%s ikcp_send %d bytes\n", holder->name, len);
#endif
    holder->send_cnt++;
    // 校验退出
    if (check_quit(holder)) {
      // this causes all nested ev_run's to stop iterating
      ev_break(loop, EVBREAK_ALL);
    }
  } else {
#if __DEBUG
    printf("%s recv:[%d]/sent:[%d ikcp_recv failed: %d, %d|%d\n", holder->name,
           holder->recv_cnt, holder->send_cnt, len, holder->kcp->nrcv_buf,
           holder->kcp->nrcv_que);
#endif
  }
}

int main(int argc, char *argv[]) {
  in_addr_t addr = conv(argv[1]);
  buf = malloc(DATA_SIZE * sizeof(char));
  test_kcp("kcpsrv", UDP_SRV_PORT, addr, UDP_CLI_PORT, echo_kcp);
  free(buf);
  return 0;
}
