# kcp + libev 带负载测试结果

**负载** 472bytes 数据，与时间(`s` 8bytes; `ns` 8bytes)、`kcp` 包头加起来共512bytes

最新结果

千次

```verilog
latency detail:
------
avg     :  24022.90ns

min     :      2245ns
line 01%:      2472ns
line 05%:      3480ns
line 25%:      4377ns
line 50%:      5479ns
line 75%:     41034ns
line 95%:     78388ns
line 99%:    176959ns
max     :    220086ns
------
```

十万次
```verilog
latency detail:
------
avg     :  17060.37ns

min     :      2025ns
line 01%:      2523ns
line 05%:      3769ns
line 25%:      5387ns
line 50%:      6042ns
line 75%:     10418ns
line 95%:     62717ns
line 99%:     90788ns
max     :   1176105ns
------
```

百万次
```verilog
latency detail:
------
avg     :  13818.62ns

min     :      1991ns
line 01%:      2644ns
line 05%:      4135ns
line 25%:      5728ns
line 50%:      6021ns
line 75%:      6370ns
line 95%:     59809ns
line 99%:     81830ns
max     :  12921315ns
------
```

从测试结果看，相当稳定。
