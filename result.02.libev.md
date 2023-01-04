# kcp + libev 测试结果

最新结果

千次

```verilog
latency detail:
------
avg     :  6649.54ns

min     :      2497ns
line 01%:      4153ns
line 05%:      4558ns
line 25%:      4917ns
line 50%:      5047ns
line 75%:      5352ns
line 95%:      6814ns
line 99%:     58236ns
max     :     88822ns
------
```

十万次
```verilog
latency detail:
------
avg     :  6658.67ns

min     :      2265ns
line 01%:      3699ns
line 05%:      4570ns
line 25%:      4891ns
line 50%:      5098ns
line 75%:      5402ns
line 95%:      7579ns
line 99%:     56112ns
max     :   4258464ns
------
```

百万次
```verilog
latency detail:
------
avg     :  6885.84ns

min     :      2191ns
line 01%:      3366ns
line 05%:      4595ns
line 25%:      4903ns
line 50%:      5122ns
line 75%:      5453ns
line 95%:      7827ns
line 99%:     57009ns
max     :   2468035ns
------
```

从测试结果看，相当稳定。
