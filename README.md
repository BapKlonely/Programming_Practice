# 火车票预订系统 — 人员模块设计方案

> **版本**: v1.0  
> **日期**: 2026-07-08  
> **分工**: 模型层（`struct/`）+ 业务层（`service/`），不涉及 UI 层  
> **技术栈**: Qt 5.14 C++（QtCore 模块），MinGW 64-bit，C++17

---

## 目录

1. [模块定位与目标](#一模块定位与目标)
2. [整体架构](#二整体架构)
3. [数据模型设计](#三数据模型设计)
4. [业务服务设计](#四业务服务设计)
5. [权限模型](#五权限模型)
6. [数据持久化方案](#六数据持久化方案)
7. [接口契约（供队友调用）](#七接口契约供队友调用)
8. [模块间交互](#八模块间交互)
9. [完整实现规格](#九完整实现规格)
10. [测试方案](#十测试方案)
11. [附录：文件清单](#十一附录文件清单)

---

## 一、模块定位与目标

### 1.1 模块定位

人员模块是火车票预订系统的**基础模块**，负责系统中所有与"人"相关的数据管理与业务逻辑。它是整个系统的入口——用户在操作任何功能之前，必须先通过人员模块建立身份。

```
┌──────────────────────────────────────────────────┐
│                  火车票预订系统                    │
│                                                  │
│  ┌──────────┐  ┌──────────┐  ┌───────────────┐  │
│  │  人员模块  │  │  车次模块  │  │   订单模块     │  │
│  │ (本次设计) │  │ (队友负责) │  │ (依赖前两者)   │  │
│  └─────┬─────┘  └─────┬─────┘  └───────┬───────┘  │
│        │              │                │          │
│  ┌─────┴──────────────┴────────────────┴───────┐  │
│  │              Qt UI 层（队友负责）              │  │
│  └──────────────────────────────────────────────┘  │
└──────────────────────────────────────────────────┘
```

### 1.2 核心功能

| 编号 | 功能 | 说明 |
|------|------|------|
| F-01 | 用户注册 | 消费者/管理员创建账号，自动校验重名与必填项 |
| F-02 | 用户登录 | 账号密码验证，区分消费者与管理员身份 |
| F-03 | 游客模式 | 无需登录即可浏览车次（权限受限） |
| F-04 | 密码修改 | 消费者自行改密（需旧密码），管理员可强制重置 |
| F-05 | 账户管理 | 管理员启用/禁用任意账户 |
| F-06 | 用户列表 | 管理员查看全部注册用户 |
| F-07 | 会话追踪 | 记录当前登录用户的身份类型与账号，供其他模块鉴权 |
| F-08 | 登出 | 清除当前会话，退回游客状态 |

### 1.3 设计原则

1. **接口与实现分离**：暴露给队友的头文件只含声明，实现细节在 `.cpp` 中
2. **防御式编程**：所有公有方法通过 `QString& err` 输出错误信息，返回值 `bool` 表示成败
3. **事务性操作**：写文件失败时自动回滚内存状态，保证数据一致性
4. **最小依赖**：仅依赖 QtCore，不依赖任何第三方库
5. **向后兼容**：保留 `is_adminstrator` 布尔字段，新增 `user_type` 枚举，新旧代码可共存

---

## 二、整体架构

### 2.1 分层结构

```
summer_project/
├── struct/                     # 模型层 — 纯数据结构
│   ├── user.h                  #   用户数据结构 + 用户类型枚举
│   ├── train.h                 #   车次数据结构（已有，本次不动）
│   └── order.h                 #   订单数据结构（已有，本次不动）
│
├── service/                    # 业务层 — 逻辑 + 持久化
│   ├── user_service.h          #   ★ 人员模块核心：声明
│   ├── user_service.cpp        #   ★ 人员模块核心：实现（待创建）
│   ├── train_service.h         #   车次纯虚接口（已有）
│   └── order_service.h         #   订单服务声明（已有，需小幅修改）
│
├── data/                       # 运行时数据目录
│   └── users.json              #   用户持久化文件
│
└── 人员模块设计方案.md           #   本文档
```

### 2.2 类关系图

```
┌─────────────────────┐
│   user_information   │  模型层：用户数据结构
│  (struct/user.h)    │
├─────────────────────┤
│ + account: QString   │
│ + password: QString  │
│ + name: QString      │
│ + is_adminstrator    │
│ + enabled_account    │
│ + type: user_type    │
└──────────┬──────────┘
           │ 被管理
           ▼
┌─────────────────────┐     依赖      ┌─────────────────────┐
│    user_service      │ ───────────→ │    order_service     │
│ (service/user_      │   传入指针    │ (service/order_      │
│  service.h/.cpp)    │              │  service.h)          │
├─────────────────────┤              ├─────────────────────┤
│ - users: QList       │              │ - u_service*         │
│ - user_file_path     │              │   用于权限判断        │
│ - current_user_type  │              └─────────────────────┘
│ - current_account    │
│ - error              │
├─────────────────────┤
│ + login_as_guest()   │
│ + checklogin()       │
│ + newuser()          │
│ + changepassword()   │
│ + setuserenabled()   │
│ + getlist()          │
│ + is_logged()        │
│ + is_adminstrator()  │
│ - loadfromfile()     │
│ - savetofile()       │
│ - defaultuser()      │
└─────────────────────┘
```

---

## 三、数据模型设计

### 3.1 用户类型枚举

```cpp
// struct/user.h

enum class user_type
{
    guest = 0,         // 游客 — 无需登录，仅可查询车次
    consumer = 1,      // 消费者 — 已注册的普通用户
    administrator = 2  // 管理员 — 拥有系统最高权限
};
```

**设计决策**：

| 决策点 | 选择 | 理由 |
|--------|------|------|
| 枚举 vs 布尔 | `enum class` | 三种身份无法用 `bool` 表达；`enum class` 强类型，避免隐式转换 |
| 起始值 | `guest = 0` | 0 作为默认值，新用户/未初始化状态默认为游客，安全 |
| 保留 `is_adminstrator` | 是 | 兼容已有代码（如 `order_service` 中的管理员判断逻辑），降低迁移风险 |

### 3.2 用户信息结构体

```cpp
// struct/user.h

struct user_information
{
    QString account;           // 用户名（唯一标识，主键）
    QString password;          // 密码（明文存储，演示项目简化处理）
    QString name;              // 真实姓名
    bool is_adminstrator;      // 是否为管理员（兼容字段，逐步废弃）
    bool enabled_account;      // 账户启用状态（false = 禁用，禁止登录）
    user_type type;            // 用户类型（权威身份来源）
};
```

**字段约束**：

| 字段 | 类型 | 必填 | 唯一 | 约束 |
|------|------|------|------|------|
| `account` | `QString` | ✅ | ✅ | 非空，注册时校验重名 |
| `password` | `QString` | ✅ | ❌ | 非空，无复杂度要求（演示项目） |
| `name` | `QString` | ❌ | ❌ | 可为空 |
| `is_adminstrator` | `bool` | — | ❌ | 兼容字段，与 `type` 保持同步 |
| `enabled_account` | `bool` | — | ❌ | 默认 `true`，管理员可切换 |
| `type` | `user_type` | ✅ | ❌ | 权威身份字段，注册时自动赋值 |

---

## 四、业务服务设计

### 4.1 类声明总览

| 文件 | 类名 | 职责 |
|------|------|------|
| `service/user_service.h` | `user_service` | 用户注册、登录、密码管理、启用/禁用、列表查询、会话管理 |
| `service/user_service.cpp` | （同上） | 所有方法的具体实现 |

### 4.2 成员变量设计

| 变量名 | 类型 | 访问 | 说明 |
|--------|------|------|------|
| `users` | `QList<user_information>` | private | 内存中的用户表，启动时从文件加载 |
| `user_file_path` | `QString` | private | 持久化文件路径，构造时传入 |
| `current_user_type` | `user_type` | private | 当前会话身份，初始 `guest` |
| `current_account` | `QString` | private | 当前会话账号，游客时为空 |
| `error` | `QString` | private | 最近一次错误信息 |

### 4.3 方法详细规格

#### 4.3.1 构造函数与析构

```cpp
user_service();   // 默认构造：初始化 current_user_type=guest, current_account=""
                  // 调用 loadfromfile()，若文件不存在则调用 defaultuser() 创建初始管理员
                  // 然后调用 savetofile() 持久化

~user_service();  // 析构：当前无需额外清理（QList/QString 自动析构）
```

#### 4.3.2 `login_as_guest()` — 游客模式入口

```
功能：将当前会话设置为游客身份
调用方：UI 层"游客浏览"按钮
前置条件：无
后置条件：current_user_type = guest, current_account = ""
返回值：void（永不失败）
```

```cpp
void login_as_guest()
{
    current_user_type = user_type::guest;
    current_account = "";
}
```

#### 4.3.3 `checklogin()` — 登录验证

```
功能：验证账号密码，登录成功后更新会话状态
调用方：UI 层"登录"按钮
前置条件：无
后置条件（成功）：current_user_type = 对应用户的type, current_account = 对应用户的account
后置条件（失败）：error 包含失败原因
```

| 参数 | 方向 | 类型 | 说明 |
|------|------|------|------|
| `account` | in | `const QString&` | 输入的用户名 |
| `password` | in | `const QString&` | 输入的密码 |
| `error` | out | `QString&` | 错误信息 |
| 返回值 | out | `user_information*` | 成功返回内部元素指针，失败返回 `nullptr` |

**处理流程**：

```
输入: account, password
      │
      ▼
遍历 users 查找 account 匹配的用户
      │
      ├── 找不到 ──→ error = "用户名不存在" → 返回 nullptr
      │
      └── 找到了
            │
            ├── enabled_account == false
            │     ──→ error = "账号已被禁用" → 返回 nullptr
            │
            ├── password 不匹配
            │     ──→ error = "密码错误" → 返回 nullptr
            │
            └── 全部通过
                  ──→ 设置 current_user_type, current_account
                  ──→ 返回 &user
```

#### 4.3.4 `newuser()` — 注册新用户

```
功能：在系统中创建新用户账号
调用方：UI 层"注册"按钮
前置条件：无（任何人都可注册）
后置条件（成功）：users 列表 +1，文件已更新
后置条件（失败）：users 列表不变，文件不变，error 包含原因
```

| 参数 | 方向 | 类型 | 说明 |
|------|------|------|------|
| `account` | in | `const user_information&` | 完整用户信息（含 account、password、name 等） |
| `error` | out | `QString&` | 错误信息 |
| 返回值 | out | `bool` | `true` 成功，`false` 失败 |

**校验规则**：

| 检查项 | 规则 | 错误信息 |
|--------|------|---------|
| 用户名非空 | `account.account.isEmpty()` | "用户名不能为空" |
| 密码非空 | `account.password.isEmpty()` | "密码不能为空" |
| 用户名唯一 | 遍历 users，无相同 account | "用户名 'xxx' 已被注册" |

**事务流程**：

```
1. 校验 account、password 非空
2. 遍历 users，检查 account 是否重复
3. 将新用户 append 到 users
4. 调用 savetofile()
   ├── 成功 → 返回 true
   └── 失败 → removeLast() 回滚 → error 记录原因 → 返回 false
```

**自动字段赋值**：

```cpp
// 注册时自动设置（无论调用方传什么值）
user.enabled_account = true;             // 新注册默认启用
user.type = user.is_adminstrator
    ? user_type::administrator
    : user_type::consumer;               // 根据 is_adminstrator 推导 type
```

#### 4.3.5 `changepassword()` — 修改密码

```
功能：修改指定用户的密码
调用方：UI 层"修改密码"入口
前置条件：消费者需验证旧密码，管理员可跳过旧密码验证
后置条件（成功）：密码已更新，文件已写入
后置条件（失败）：密码不变，文件不变
```

| 参数 | 方向 | 类型 | 说明 |
|------|------|------|------|
| `is_adminstrator` | in | `bool` | 调用者是否为管理员（true 时跳过旧密码验证） |
| `account` | in | `const QString&` | 要修改的目标用户名 |
| `old_one` | in | `const QString&` | 旧密码（非管理员模式需验证） |
| `new_one` | in | `const QString&` | 新密码 |
| `error` | out | `QString&` | 错误信息 |
| 返回值 | out | `bool` | `true` 成功 |

**处理流程**：

```
1. 遍历 users，查找 account
   └── 找不到 → error = "用户 'xxx' 不存在" → 返回 false

2. 非管理员模式：检查 old_one == user.password
   └── 不匹配 → error = "旧密码错误" → 返回 false

3. 新密码非空校验

4. user.password = new_one

5. savetofile()
   ├── 成功 → 返回 true
   └── 失败 → 回滚密码 → 返回 false
```

#### 4.3.6 `setuserenabled()` — 账户启停

```
功能：启用或禁用指定用户的登录权限
调用方：管理员用户管理界面
前置条件：调用者必须是管理员（UI 层保证，服务层也做二次校验）
后置条件（成功）：用户 enabled_account 已更新，文件已写入
后置条件（失败）：状态不变，文件不变
```

| 参数 | 方向 | 类型 | 说明 |
|------|------|------|------|
| `is_adminstrator` | in | `bool` | 调用者身份（必须为 true） |
| `account` | in | `const QString&` | 目标用户名 |
| `enabled` | in | `bool` | 目标状态 |
| `error` | out | `QString&` | 错误信息 |
| 返回值 | out | `bool` | `true` 成功 |

**处理流程**：

```
1. 权限检查：is_adminstrator == false
   ──→ error = "无权限" → 返回 false

2. 遍历 users，查找 account
   └── 找不到 → error = "用户不存在" → 返回 false

3. 保存旧值：old_enabled = user.enabled_account
4. 更新：user.enabled_account = enabled
5. savetofile()
   ├── 成功 → 返回 true
   └── 失败 → 回滚 user.enabled_account = old_enabled → 返回 false
```

#### 4.3.7 `getlist()` — 用户列表

```
功能：返回系统中所有注册用户的副本
调用方：管理员用户管理界面
前置条件：调用者是管理员（方法内做权限校验）
后置条件：无副作用（只读）
```

| 参数 | 方向 | 类型 | 说明 |
|------|------|------|------|
| `error` | out | `QString&` | 错误信息（权限不足时填充） |
| 返回值 | out | `QList<user_information>` | 用户列表副本 |

#### 4.3.8 会话查询方法

```cpp
// 是否已登录（非游客即为已登录）
bool is_logged() const
{
    return current_user_type != user_type::guest;
}

// 当前用户是否为管理员
bool is_adminstrator() const
{
    return current_user_type == user_type::administrator;
}

// 获取当前登录用户信息（供 UI 显示"欢迎，xxx"）
void get_current_user() const;  // 通过成员变量 current_account 查找并输出
```

---

## 五、权限模型

### 5.1 三级权限矩阵

| 操作 | 游客 (Guest) | 消费者 (Consumer) | 管理员 (Administrator) |
|------|:---:|:---:|:---:|
| 查询车次 | ✅ | ✅ | ✅ |
| 注册账号 | ✅ | ✅ | ✅ |
| 登录 | — | ✅ | ✅ |
| 修改自己密码 | — | ✅ | ✅ |
| 订票 | ❌ | ✅ | ✅ |
| 查看自己的订单 | ❌ | ✅ | ✅ |
| 退票 | ❌ | ✅ | ✅ |
| 查看所有订单 | ❌ | ❌ | ✅ |
| 查看用户列表 | ❌ | ❌ | ✅ |
| 启停用户账号 | ❌ | ❌ | ✅ |
| 强制重置用户密码 | ❌ | ❌ | ✅ |

### 5.2 权限校验流程

```
                    ┌──────────────┐
                    │  操作请求     │
                    └──────┬───────┘
                           ▼
              ┌────────────────────────┐
              │ 查询当前会话用户类型      │
              │ u_service->is_logged()  │
              └────────────┬───────────┘
                           │
              ┌────────────┴────────────┐
              ▼                         ▼
         游客 (Guest)              已登录 (Consumer/Admin)
              │                         │
              ▼                         ▼
      ┌──────────────┐        ┌──────────────────┐
      │ 仅允许查询车次 │        │ 根据具体方法鉴权    │
      │ 其他操作拒绝   │        │ - 订票/退票: 需登录  │
      └──────────────┘        │ - 用户管理: 需管理员  │
                              └──────────────────┘
```

### 5.3 双重校验原则

权限在两个层面各校验一次：

| 层级 | 校验方 | 目的 |
|------|--------|------|
| UI 层 | 队友的 Qt 界面代码 | 隐藏/灰显无权限按钮，提升用户体验 |
| 服务层 | `user_service` / `order_service` | 防止绕过 UI 直接调用 API（防御式编程） |

---

## 六、数据持久化方案

### 6.1 文件格式

- **路径**: `data/users.json`（相对于可执行文件运行目录）
- **格式**: JSON 数组，每个元素为一个用户对象
- **编码**: UTF-8（Qt 默认）

```json
[
    {
        "account": "admin",
        "password": "admin123",
        "name": "系统管理员",
        "is_adminstrator": true,
        "enabled_account": true,
        "type": 2
    },
    {
        "account": "zhangsan",
        "password": "123456",
        "name": "张三",
        "is_adminstrator": false,
        "enabled_account": true,
        "type": 1
    }
]
```

### 6.2 JSON 字段映射

| JSON Key | C++ 字段 | 类型 | 序列化方式 |
|----------|---------|------|-----------|
| `account` | `user_information::account` | `QString` | `obj["account"] = u.account` |
| `password` | `user_information::password` | `QString` | `obj["password"] = u.password` |
| `name` | `user_information::name` | `QString` | `obj["name"] = u.name` |
| `is_adminstrator` | `user_information::is_adminstrator` | `bool` | `obj["is_adminstrator"] = u.is_adminstrator` |
| `enabled_account` | `user_information::enabled_account` | `bool` | `obj["enabled_account"] = u.enabled_account` |
| `type` | `user_information::type` | `int`（枚举值） | `obj["type"] = static_cast<int>(u.type)` |

### 6.3 序列化/反序列化实现要点

```cpp
// 反序列化 (loadfromfile)
bool user_service::loadfromfile()
{
    QFile file(user_file_path);
    if (!file.open(QIODevice::ReadOnly)) return false;

    QJsonParseError parseErr;
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &parseErr);
    file.close();

    if (parseErr.error != QJsonParseError::NoError) {
        error = QString("JSON 解析失败: %1").arg(parseErr.errorString());
        return false;
    }

    users.clear();
    const QJsonArray arr = doc.array();
    for (const QJsonValue& val : arr) {
        QJsonObject obj = val.toObject();
        user_information u;
        u.account          = obj["account"].toString();
        u.password         = obj["password"].toString();
        u.name             = obj["name"].toString();
        u.is_adminstrator  = obj["is_adminstrator"].toBool();
        u.enabled_account  = obj["enabled_account"].toBool(true);  // 默认 true
        // 兼容旧数据：如果 JSON 中没有 type 字段，根据 is_adminstrator 推导
        if (obj.contains("type")) {
            u.type = static_cast<user_type>(obj["type"].toInt());
        } else {
            u.type = u.is_adminstrator
                ? user_type::administrator
                : user_type::consumer;
        }
        users.append(u);
    }
    return true;
}

// 序列化 (savetofile)
bool user_service::savetofile()
{
    QJsonArray arr;
    for (const auto& u : users) {
        QJsonObject obj;
        obj["account"]          = u.account;
        obj["password"]         = u.password;
        obj["name"]             = u.name;
        obj["is_adminstrator"]  = u.is_adminstrator;
        obj["enabled_account"]  = u.enabled_account;
        obj["type"]             = static_cast<int>(u.type);
        arr.append(obj);
    }

    QJsonDocument doc(arr);
    QFile file(user_file_path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        error = QString("无法写入文件: %1").arg(user_file_path);
        return false;
    }
    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();
    return true;
}
```

### 6.4 初始化种子数据

```cpp
void user_service::defaultuser()
{
    user_information admin;
    admin.account         = "admin";
    admin.password        = "admin123";
    admin.name            = "系统管理员";
    admin.is_adminstrator = true;
    admin.enabled_account = true;
    admin.type            = user_type::administrator;
    users.append(admin);
}
```

---

## 七、接口契约（供队友调用）

### 7.1 队友需要了解的方法

队友在编写 Qt UI 时，按以下顺序调用：

```
应用启动
    │
    ▼
QCoreApplication 初始化
    │
    ▼
user_service userSvc;          // 自动加载 data/users.json
    │
    ▼
┌─────────────────────────────────────────────────┐
│                   登录/注册界面                    │
│                                                 │
│  [游客浏览] → userSvc.login_as_guest()           │
│  [登录]     → userSvc.checklogin(acc, pwd, err)  │
│  [注册]     → userSvc.newuser(info, err)          │
└─────────────────────────────────────────────────┘
    │
    ▼
┌─────────────────────────────────────────────────┐
│                   主界面                          │
│                                                 │
│  判断身份:                                       │
│  - userSvc.is_logged() → true/false             │
│  - userSvc.is_adminstrator() → true/false       │
│                                                 │
│  消费者功能:                                      │
│  - userSvc.changepassword(false, acc, old, new)  │
│                                                 │
│  管理员功能:                                      │
│  - userSvc.getlist(err) → QList                  │
│  - userSvc.setuserenabled(true, acc, en, err)    │
│  - userSvc.changepassword(true, acc, "", new)    │
└─────────────────────────────────────────────────┘
```

### 7.2 调用约定速查表

| 你要做的事 | 调用方法 | 参数要点 |
|-----------|---------|---------|
| 游客进入 | `login_as_guest()` | 无参数 |
| 登录 | `checklogin(account, password, err)` | 失败看 err |
| 注册 | `newuser(info, err)` | 先填好 `user_information` |
| 改自己密码 | `changepassword(false, account, old, new, err)` | 需旧密码 |
| 管理员改别人密码 | `changepassword(true, account, "", new, err)` | 旧密码传空串 |
| 启用/禁用用户 | `setuserenabled(true, account, enabled, err)` | 仅管理员 |
| 查看用户列表 | `getlist(err)` | 权限不够 err 会有提示 |
| 判断是否登录 | `is_logged()` | 控制订票按钮灰显 |
| 判断是否管理员 | `is_adminstrator()` | 控制管理菜单可见性 |

---

## 八、模块间交互

### 8.1 对 order_service 的影响

`order_service` 需要引入对 `user_service` 的依赖，用于权限判断：

```cpp
// service/order_service.h 变更点

#include "user_service.h"    // 新增 include

class order_service
{
private:
    user_service* u_service; // 新增成员，指向 user_service 实例

    // 新增私有辅助方法
    bool check_booking_permission(QString& err)
    {
        if (u_service && !u_service->is_logged())
        {
            err = "请先登录后再进行操作";
            return false;
        }
        return true;
    }

public:
    // 构造函数新增参数
    order_service(const QString& order_file_path,
                  train_service* t_service,
                  user_service* u_service);  // 新增
};
```

### 8.2 交互时序图：购票流程

```
用户          UI            order_service     user_service      train_service
 │            │                 │                 │                 │
 │  点击购票   │                 │                 │                 │
 │───────────→│                 │                 │                 │
 │            │                 │                 │                 │
 │            │  bookticket()   │                 │                 │
 │            │────────────────→│                 │                 │
 │            │                 │                 │                 │
 │            │                 │ is_logged()     │                 │
 │            │                 │────────────────→│                 │
 │            │                 │    true/false   │                 │
 │            │                 │←────────────────│                 │
 │            │                 │                 │                 │
 │            │                 │ if guest: return false            │
 │            │                 │                 │                 │
 │            │                 │ getTrain()      │                 │
 │            │                 │─────────────────────────────────→│
 │            │                 │         TrainInfo                 │
 │            │                 │←─────────────────────────────────│
 │            │                 │                 │                 │
 │            │                 │ deductSeats()   │                 │
 │            │                 │─────────────────────────────────→│
 │            │                 │         OK/FAIL                   │
 │            │                 │←─────────────────────────────────│
 │            │                 │                 │                 │
 │            │    Order/err    │  写入订单文件    │                 │
 │            │←────────────────│                 │                 │
 │            │                 │                 │                 │
 │  显示结果   │                 │                 │                 │
 │←───────────│                 │                 │                 │
```

---

## 九、完整实现规格

### 9.1 需要创建的文件

| 文件 | 操作 | 内容 |
|------|------|------|
| `struct/user.h` | **修改** | 已存在，添加 `user_type` 枚举和 `type` 字段 |
| `service/user_service.h` | **修改** | 已存在，需增加会话管理成员和方法声明 |
| `service/user_service.cpp` | **新建** | 所有方法的完整实现 |

### 9.2 需要修改的文件

| 文件 | 修改内容 |
|------|---------|
| `service/order_service.h` | 新增 `#include "user_service.h"`、`user_service*` 成员、构造函数增加参数、`check_booking_permission()` 私有方法 |
| `service/order_service.cpp` | （待创建时）在 `bookticket`/`cancel_order`/`get_order` 开头调用权限检查 |

### 9.3 错误信息规范

所有错误信息使用中文，格式统一：

| 场景 | 错误信息模板 |
|------|------------|
| 用户名已存在 | `用户名 '%1' 已被注册` |
| 用户不存在 | `用户 '%1' 不存在` |
| 密码错误 | `密码错误` |
| 旧密码错误 | `旧密码错误` |
| 账号已禁用 | `账号已被禁用，请联系管理员` |
| 必填为空 | `用户名不能为空` / `密码不能为空` |
| 权限不足 | `当前没有操作权限` |
| 文件读写失败 | `无法写入文件: %1` / `JSON 解析失败: %1` |

---

## 十、测试方案

### 10.1 测试场景清单

#### 基本功能测试

| 编号 | 场景 | 前置条件 | 操作 | 预期结果 |
|------|------|---------|------|---------|
| T-01 | 首次启动创建管理员 | 无 users.json | 构造 user_service | 自动创建 admin/admin123，写入文件 |
| T-02 | 管理员登录 | admin 存在且启用 | checklogin("admin", "admin123", err) | 返回非空指针，is_logged()=true, is_adminstrator()=true |
| T-03 | 错误密码登录 | admin 存在 | checklogin("admin", "wrong", err) | 返回 nullptr，err="密码错误" |
| T-04 | 不存在的用户登录 | — | checklogin("nobody", "x", err) | 返回 nullptr，err="用户名不存在" |
| T-05 | 注册新用户 | — | newuser(合法信息, err) | 返回 true，users 列表 +1 |
| T-06 | 注册重名用户 | T-05 已注册 | newuser(同 account, err) | 返回 false，err 提示重名 |
| T-07 | 注册空用户名 | — | newuser(account="", err) | 返回 false，err="用户名不能为空" |
| T-08 | 修改密码成功 | 已登录 | changepassword(false, acc, old, new, err) | 返回 true，新密码可登录 |
| T-09 | 旧密码错误改密 | 已登录 | changepassword(false, acc, "wrong", new, err) | 返回 false，err="旧密码错误" |
| T-10 | 管理员强制改密 | 管理员 | changepassword(true, acc, "", new, err) | 返回 true，跳过旧密码验证 |

#### 权限测试

| 编号 | 场景 | 前置条件 | 操作 | 预期结果 |
|------|------|---------|------|---------|
| T-11 | 游客登录 | — | login_as_guest() | is_logged()=false, is_adminstrator()=false |
| T-12 | 非管理员查看用户列表 | 以消费者登录 | getlist(err) | 返回空列表或 err 提示无权限 |
| T-13 | 非管理员启停用户 | 以消费者登录 | setuserenabled(false, ...) | 返回 false |
| T-14 | 管理员查看列表 | 以管理员登录 | getlist(err) | 返回完整列表 |
| T-15 | 管理员禁用用户 | 以管理员登录 | setuserenabled(true, "zhangsan", false, err) | 返回 true，该用户无法登录 |

#### 持久化与回滚测试

| 编号 | 场景 | 前置条件 | 操作 | 预期结果 |
|------|------|---------|------|---------|
| T-16 | 重启后数据保持 | 已注册用户 | 重新构造 user_service | 之前注册的用户仍存在 |
| T-17 | 被禁用用户无法登录 | T-15 执行后 | checklogin("zhangsan", "xxx", err) | 返回 nullptr，err 含"禁用" |
| T-18 | 旧数据兼容 | JSON 中无 type 字段 | loadfromfile() | 根据 is_adminstrator 自动推导 type |

#### 交互测试（与 order_service 联调）

| 编号 | 场景 | 前置条件 | 操作 | 预期结果 |
|------|------|---------|------|---------|
| T-19 | 游客订票被拒 | 游客模式 | orderSvc.bookticket(...) | 返回 false，err 提示登录 |
| T-20 | 消费者订票成功 | 消费者登录 | orderSvc.bookticket(...) | 权限通过，执行购票逻辑 |
| T-21 | 管理员可查看所有订单 | 管理员登录 | orderSvc.get_order(任意账号) | 返回该账号的订单列表 |

### 10.2 建议的测试 main 函数结构

```cpp
// test_user_module.cpp（独立测试用）
#include <QCoreApplication>
#include <QDebug>
#include "service/user_service.h"

#define TEST(name) qDebug() << "\n==========" << #name << "=========="
#define PASS       qDebug() << "  ✅ 通过"
#define FAIL(msg)  qDebug() << "  ❌ 失败:" << msg

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);

    // 测试套件：按 T-01 ~ T-18 逐个执行
    TEST(T01_首次启动创建管理员);
    {
        // 确保 data/users.json 不存在
        QFile::remove("data/users.json");
        user_service svc;
        QString err;
        auto* u = svc.checklogin("admin", "admin123", err);
        if (u && svc.is_adminstrator()) PASS else FAIL(err);
    }

    // ... 其余测试用例 ...

    return 0;
}
```

---

## 十一、附录：文件清单

### 11.1 人员模块文件

| 文件路径 | 类型 | 行数（预估） | 说明 |
|---------|------|:---:|------|
| `struct/user.h` | 模型头文件 | ~30 | 用户枚举 + 结构体 |
| `service/user_service.h` | 服务头文件 | ~50 | 类声明与接口 |
| `service/user_service.cpp` | 服务实现 | ~200 | 全部业务逻辑 + JSON 读写 |

### 11.2 受影响的非人员模块文件

| 文件路径 | 影响程度 | 说明 |
|---------|:---:|------|
| `service/order_service.h` | 小幅修改 | 新增 include、成员、构造函数参数 |
| `service/order_service.cpp` | 小幅修改 | 在三个方法中调用权限检查 |

### 11.3 与本模块无直接关系的文件（本次不动）

| 文件 | 原因 |
|------|------|
| `struct/train.h` | 车次数据结构，人员模块不涉及 |
| `struct/order.h` | 订单数据结构，订单模块负责 |
| `service/train_service.h` | 纯虚接口，队友负责实现 |

---

## 设计评审检查清单

在开始编码前，请确认以下事项：

- [ ] `user_type` 枚举与 `is_adminstrator` 布尔值的职责边界清晰吗？
- [ ] 所有公有方法的错误信息都通过 `QString& err` 输出了吗？
- [ ] 写文件失败时的回滚逻辑覆盖了所有修改操作吗？
- [ ] `order_service` 引入 `user_service*` 后，构造函数签名同步更新了吗？
- [ ] 旧 JSON 数据（缺少 `type` 字段）能被正确兼容加载吗？
- [ ] 禁用用户后，该用户当前已建立的会话是否需要强制登出？
  - > 当前方案：不强制登出。下次登录时才拦截。如需即时生效，可在 `setuserenabled` 中判断是否为当前会话用户，若禁用自己则调用类似 `logout()` 的逻辑。
- [ ] 密码是否考虑后续改为哈希存储？
  - > 当前明文存储仅用于演示。生产环境建议 `QCryptographicHash::hash(password, QCryptographicHash::Sha256).toHex()`。

---

> **下一步**: 参照《人员模块实现教程-Qt版.md》逐个创建 `.cpp` 实现文件，编译运行，用上述测试场景逐项验证。
