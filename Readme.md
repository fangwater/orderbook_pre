依据Order和Trade来构造订单簿

1、前提Order和Trade的时序要正确

2、Trade和Order的消息要完整

如何还原即时成交的Order

即时成交的Order如何在盘口进行还原

即时成交的本质:
Order A --- Order B, Order B可以和A直接成交，Order B为主动单

1、A_vol > B_vol 会收到一笔Trade 没有Order
判断的方式是，收到的Trade，根据B-S得到主动方，发现OrderNo不存在于OrderBook，则得到Order B

2、A_vol == B_vol 处理方式相同

3、A_vol < B_vol 


增量授信计算
1、分治计算，每个订单薄只维护一部分机构的报价，orderbook层次进行一次拆分
2、新订单判定在OrderBook范围，则加入Orderbook
3、假设此时这个OrderBook维护了K个机构的报价量，订单添加更新总报价量，O(1)时间不计
4、公有行情的增量算法是显然的
5、K个机构，因此需要维护K个档，每个档上维护报价总量，私有行情的计算是每个位置取和额度矩阵对应行取min，然后sum
6、当存在orderAdd，即某一个档上的量出现变化，需要和额度重新取min，sum - prev_con + new, 即可以获得增量行情
7、当存在orderDel，某一个档的量变少，使用同样的算法
8、但这个算法存在海量的空间换时间，是不可接受的，应该用哈希表维护这个矩阵






