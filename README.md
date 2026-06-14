# TicketSystem

## 功能概述

### 用户管理
- `add_user`：注册新用户。第一个用户自动获得最高权限10，后续用户必须由高权限用户创建
- `login` / `logout`：登录、登出
- `query_profile` / `modify_profile`：查询和修改用户信息，权限低不能改权限高的用户

### 车次管理
- `add_train`：录入车次（站序、票价、时刻、停留、售卖日期范围、座位数）
- `delete_train`：删除未发布的车次
- `release_train`：发布车次，发布后才能被查询和购票
- `query_train`：查询某趟车在某天的详细信息（站次、时刻、剩余票数）
- `query_ticket`：查询从 A 站到 B 站某天的所有直达车次，可按耗时或价格排序
- `query_transfer`：查询恰好换乘一次的最优方案

### 订单管理
- `buy_ticket`：购票。余票不足时可选择候补
- `query_order`：查询当前用户的所有订单
- `refund_ticket`：退票。如果是已成交订单，退票后会自动尝试满足候补队列中的订单

## 设计要点

### 存储
所有数据通过B+存储，程序退出再启动后数据依然存在。每个B+树对应一个数据文件（叶子节点）和一个索引文件（内部节点）。

### 数据与索引分离
为了控制内存占用，大对象（如 `TrainInfo`、`SeatInfo`、`OrderInfo` 等）单独存到 `.data` 文件中，B+ 树的 value 只存 int 类型的文件偏移。这样 B+ 树内部节点很小，可以全部放在内存中。

### B+ 树内部节点的动态分配
B+ 树的内部节点通过 `sjtu::vector<InternalNode*>` 按需申请动态空间

## 文件说明

| 文件 | 说明                           |
| --- |------------------------------|
| `src/bpt.h` | B+ 树，支持插入、删除、单点查找、范围查找、修改    |
| `src/storage.h` | 具体数据，提供 read/write/update 接口 |
| `src/userManager.h` | 用户系统                         |
| `src/trainManager.h` | 车次系统                         |
| `src/orderManager.h` | 订单系统                         |
| `src/date.h` | 日期、时间相关的辅助函数                 |
| `main.cpp` | 解析输入指令，分发到对应的 manager        |
