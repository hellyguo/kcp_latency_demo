# kcp + libev 测试结果

最新结果

千次

```verilog
latency detail:
------
avg     :  7937.01ns

min     :      2948ns
line 01%:      4164ns
line 05%:      5458ns
line 25%:      5769ns
line 50%:      6005ns
line 75%:      6312ns
line 95%:     19437ns
line 99%:     59472ns
max     :    117847ns
------
```

十万次
```verilog
latency detail:
------
avg     :  10530.75ns

min     :      1978ns
line 01%:      3389ns
line 05%:      4455ns
line 25%:      5808ns
line 50%:      6057ns
line 75%:      6279ns
line 95%:     52187ns
line 99%:     75193ns
max     :   5714344ns
------
```

百万次
```verilog
latency detail:
------
avg     :  13860.78ns

min     :      1895ns
line 01%:      2763ns
line 05%:      4163ns
line 25%:      5621ns
line 50%:      5923ns
line 75%:      6289ns
line 95%:     60637ns
line 99%:     78152ns
max     :  10053585ns
------
```

从测试结果看，相当稳定。
