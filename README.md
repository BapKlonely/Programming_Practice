# 本分支当前主要内容
完成了数据库的建立和初始测试数据的加载

## 目录结构

```
mysrc/
├── CMakeLists.txt      # 本工作区工程文件（仅 ticket_test）
├── core/               # Database：打开 SQLite、建表、种子数据
├── models/             # User / Train / Order 等 struct
├── services/           # Service 头文件（.cpp 待实现）
└── tests/              # 控制台测试 database_manual.cpp
```

## 如何使用
1. 先阅读database模块使用说明，掌握如何使用QT内置的SQL驱动实现数据的增删查改；
2. 阅读models和services两个文件夹对users和trains的基本变量、方法的定义，确保每个人推进项目的过程中名称统一。

## 如何打开与编译

1. 用 **Qt Creator** 打开 `CMakeLists.txt`
2. 配置 Kit（MinGW 64-bit + Qt 6.x）
3. 构建并运行目标 **`ticket_test`**
4. 成功后会在 `build/.../data/test_ticket.db` 生成数据库，有该文件说明本分支程序跑通

## 默认测试账号（种子数据）

| 用户名 | 密码 | 角色 |
|--------|------|------|
| admin | admin123 | 管理员 |
| seller | seller123 | 售票员 |

## 超售测试车次

种子数据中的 **T999**（北京→上海）仅 **1** 张二等座，供后续测试超售拦截。

## 与全组最终合并

联调阶段可将 `core/`、`models/`、`services/` 合并进主工程，由 `main.cpp` 统一装配各 Service；在此之前在 `mysrc/` 独立开发即可。
