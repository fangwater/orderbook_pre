根据需求修改 4月30日需求讨论
1、撮合日志，撮合日志的形式只会是对某一笔订单的状态修改，因此其格式更接近于

2、因此不会收到Trade，只包含OrderNo，因此查询订单的方式不能按照价格，需要修改
具体改为，对于任意种类的变更信息，查看是否存在价格
订单的种类包括OrderAdd，OrderDel两种

1、Pirce为0，即不包含有效的价格信息，或者有Price，但无法保证价格的稳定性
(1)对于Price为0的订单，则根据B/S方向，从pricelevel使用遍历的方式寻找
(2)对于有正常价格的订单，则可以利用成交价格进行搜索，还是可以知道挂单价?
我认为挂单的价格在撮合阶段可以是已知的，具体看TXN

矩阵形式
需要启动多少个买卖队列

并行构建，tbb + barray