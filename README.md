# 样本

测试 [KCP](https://github.com/skywind3000/kcp/) 延时的样例程序

**说明**

为了达到最佳延时效果，按 `KCP` 的建议，每次都调用 `ikcp_flush` 函数进行强制发送。

## 结构

- `kcpsrv` 监听 `udp` `12345` 端口
- `kcpcli` 监听 `udp` `54321` 端口

1. `kcpcli` 连接到 `kcpsrv` 的监听端口上，每隔 `1ms` 上送执行时的时间戳。
2. `kcpsrv` 接收到报文后，用当前时间与报文时间戳相减，得到单次通讯延时。
3. `kcpsrv` 将 `KCP` 底层确认包发送到 `kcpcli` 的监听端口。

## 测试结果

最新结果

```verilog
latency detail:
------
avg     :  30610.31ns

min     :      5684ns
line 01%:      6450ns
line 05%:      8617ns
line 25%:     14490ns
line 50%:     15833ns
line 75%:     46509ns
line 95%:     59999ns
line 99%:     84907ns
max     :    117780ns
------
```

## 编译

**前提**

- 将 `libkcp.a` 放于 `lib` 内
- 或将 `libkcp.a` 安装到 `/usr` 下

```shell
$> mkdir build
$> cd build
$> cmake .. && make
```

可打开 `kcpdemo.h` 中的 `__DEBUG` 宏，添加更多输出信息

## 执行

开两个终端

**终端1**

```shell
$> cd build
$> ./kcpsrv
```

**终端2**

```shell
$> cd build
$> ./kcpcli
```

## 缺陷

1. ~~目前无法支持太多次连续调用~~
    > 原因：`KCP` 的队列不支持多线程操作，需要在一个线程中操作
2. ~~使用了线程锁，延迟降低严重~~
    > 原因：`kcpcli` 一次只读一个包过慢，一次性读取足够多就能加速

## 感谢

感谢[林伟](https://github.com/skywind3000/)大佬提供的优秀网络库！
