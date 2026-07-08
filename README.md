# 成员 E 独立工作区（mysrc）

本目录与 [`../src/`](../src/) 中的参考模板 **分离**：

| 目录 | 用途 |
|------|------|
| `src/` | 小组参考模板（JSON DataStore + Qt 界面原型，成员 B 参考） |
| `mysrc/` | 成员 E 的实现：SQLite + Service 分层（按分工计划） |

当前进度：**第 1～2 步**（工程骨架 + Database 建表与种子数据）。第 3 步再实现 `TicketService.cpp` 等。

**给其他成员的详细使用文档** → [`docs/Database模块使用说明.md`](../docs/Database模块使用说明.md)  
（含 SQLite/Qt 入门、表结构、各成员代码示例、联调约定）

## 目录结构

```
mysrc/
├── CMakeLists.txt      # 本工作区工程文件（仅 ticket_test）
├── core/               # Database：打开 SQLite、建表、种子数据
├── models/             # User / Train / Order 等 struct
├── services/           # Service 头文件（.cpp 待实现）
└── tests/              # 控制台测试 database_manual.cpp
```

## 如何打开与编译

1. 用 **Qt Creator** 打开 `mysrc/CMakeLists.txt`（不要打开 `src/` 里的，除非要看参考 UI）
2. 配置 Kit（MinGW 64-bit + Qt 6.x）
3. 构建并运行目标 **`ticket_test`**
4. 成功后会在 `build/.../data/test_ticket.db` 生成数据库

## 默认测试账号（种子数据）

| 用户名 | 密码 | 角色 |
|--------|------|------|
| admin | admin123 | 管理员 |
| seller | seller123 | 售票员 |

## 超售测试车次

种子数据中的 **T999**（北京→上海）仅 **1** 张二等座，供第 3 步 `TicketService` 测试超售拦截。

## 与全组最终合并

联调阶段可将 `core/`、`models/`、`services/` 合并进主工程，由 `main.cpp` 统一装配各 Service；在此之前在 `mysrc/` 独立开发即可。
