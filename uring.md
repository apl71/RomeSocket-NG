可以。你现在做 RomeSocket v0.1，最好先把 **liburing 的接口**按“生命周期 → 提交 → 完成 → 常用网络操作 → 高级能力”来理解。

先区分一下：

* Linux 内核原始 syscall：`io_uring_setup / io_uring_enter / io_uring_register`
* 实际开发通常用 **liburing**，也就是下面这些 `io_uring_*()` 包装函数

对 RomeSocket 来说，基本直接用 liburing 就行。

## 1. Ring 生命周期

最常用：

```cpp
io_uring_queue_init(entries, &ring, flags);
```

例如：

```cpp
io_uring ring;

int ret = io_uring_queue_init(
    256,
    &ring,
    0
);
```

创建：

```text
SQ = Submission Queue
CQ = Completion Queue
```

更高级一点：

```cpp
io_uring_queue_init_params(
    entries,
    &ring,
    &params
);
```

可以指定/查询更多 feature。

销毁：

```cpp
io_uring_queue_exit(&ring);
```

所以你的 Backend 最基本就是：

```cpp
class Backend {
private:
    io_uring ring_;
};
```

构造：

```cpp
Backend::Backend()
{
    io_uring_queue_init(
        256,
        &ring_,
        0
    );
}
```

析构：

```cpp
Backend::~Backend()
{
    io_uring_queue_exit(
        &ring_
    );
}
```

---

# 2. 获取一个 SQE

几乎所有操作都从：

```cpp
io_uring_get_sqe(&ring);
```

开始。

```cpp
io_uring_sqe* sqe =
    io_uring_get_sqe(&ring_);
```

SQE 可以理解成：

> “我要交给 Linux 的一张 I/O 任务单。”

然后再告诉它具体做什么。

---

# 3. 准备一个操作

这里是最多的一组 `prep_*`。

网络最重要的是：

| 操作       | liburing                   |
| -------- | -------------------------- |
| accept   | `io_uring_prep_accept()`   |
| recv     | `io_uring_prep_recv()`     |
| send     | `io_uring_prep_send()`     |
| connect  | `io_uring_prep_connect()`  |
| recvmsg  | `io_uring_prep_recvmsg()`  |
| sendmsg  | `io_uring_prep_sendmsg()`  |
| shutdown | `io_uring_prep_shutdown()` |

例如 recv：

```cpp
io_uring_prep_recv(
    sqe,
    fd,
    buffer,
    length,
    0
);
```

其实就是异步版本的：

```cpp
recv(fd, buffer, length, 0);
```

send：

```cpp
io_uring_prep_send(
    sqe,
    fd,
    data,
    length,
    0
);
```

相当于：

```cpp
send(fd, data, length, 0);
```

accept：

```cpp
io_uring_prep_accept(
    sqe,
    listen_fd,
    &addr,
    &addr_len,
    0
);
```

相当于：

```cpp
accept(...)
```

---

# 4. 给任务绑上自己的上下文

这个对 RomeSocket **极其重要**：

```cpp
io_uring_sqe_set_data(
    sqe,
    user_data
);
```

例如：

```cpp
Operation* op = ...;

io_uring_sqe_set_data(
    sqe,
    op
);
```

等任务完成以后，可以：

```cpp
auto* op =
    static_cast<Operation*>(
        io_uring_cqe_get_data(cqe)
    );
```

于是形成：

```text
提交：
Operation*
    ↓
SQE.user_data

Linux 做 I/O

完成：
CQE.user_data
    ↓
Operation*
```

这大概会成为你的 RomeSocket 核心机制。

另外还有整数版本：

```cpp
io_uring_sqe_set_data64(...)
io_uring_cqe_get_data64(...)
```

如果你想塞 `uint64_t` ID。

---

# 5. 真正提交 SQE

准备好了以后：

```cpp
io_uring_submit(&ring);
```

例如：

```cpp
auto* sqe =
    io_uring_get_sqe(&ring_);

io_uring_prep_recv(...);

io_uring_sqe_set_data(
    sqe,
    op
);

int ret =
    io_uring_submit(&ring_);
```

注意：

```text
get_sqe()
```

只是把任务写到 userspace SQ。

真正：

```text
告诉 kernel
```

通常发生在 `submit()`。

---

# 6. 等一个任务完成

最基本：

```cpp
io_uring_wait_cqe(
    &ring,
    &cqe
);
```

例如：

```cpp
io_uring_cqe* cqe;

int ret =
    io_uring_wait_cqe(
        &ring_,
        &cqe
    );
```

这个会阻塞，直到至少一个 completion 出现。

这非常适合你现在最简单的：

```cpp
while (running) {
    auto completion =
        backend.wait();

    ...
}
```

---

# 7. 非阻塞检查 CQE

如果不想等：

```cpp
io_uring_peek_cqe(
    &ring,
    &cqe
);
```

没有 CQE 时立即返回。

常见模式：

```cpp
io_uring_cqe* cqe;

while (
    io_uring_peek_cqe(
        &ring_,
        &cqe
    ) == 0
) {
    ...
}
```

一般：

```text
wait_cqe
↓
至少拿一个

然后 peek_cqe
↓
把已经完成的一批全部清掉
```

---

# 8. 告诉 io_uring：这个 CQE 我处理完了

一定不要忘：

```cpp
io_uring_cqe_seen(
    &ring,
    cqe
);
```

典型：

```cpp
auto* op =
    static_cast<Operation*>(
        io_uring_cqe_get_data(cqe)
    );

op->complete(cqe->res);

io_uring_cqe_seen(
    &ring_,
    cqe
);
```

否则 CQ ring 不会正确前进。

如果一次处理很多，还可以：

```cpp
io_uring_cq_advance(
    &ring,
    count
);
```

不过第一版用 `cqe_seen()` 最直观。

---

# 9. CQE 里最重要的是 `res`

```cpp
cqe->res
```

表示操作结果。

例如 recv：

```text
res > 0
→ 收到多少 bytes

res == 0
→ 对端正常关闭

res < 0
→ 错误
```

特别要注意：

> io_uring 通常把错误直接返回成**负 errno**。

例如：

```cpp
cqe->res == -ECONNRESET
```

而不是：

```cpp
cqe->res == -1
errno == ECONNRESET
```

所以可以：

```cpp
if (cqe->res < 0) {
    int error = -cqe->res;
}
```

这个 Rome Backend 的错误处理必须牢记。

---

# 10. `submit_and_wait`

还有一个常用接口：

```cpp
io_uring_submit_and_wait(
    &ring,
    wait_nr
);
```

意思是：

> 把 pending SQE 提交，然后至少等 `wait_nr` 个 CQE。

例如：

```cpp
io_uring_submit_and_wait(
    &ring_,
    1
);
```

可以减少：

```text
submit()
+
wait()
```

两步调用。

以后事件循环优化时很有用。

第一版不一定要立即用。

---

# 11. timeout

可以：

```cpp
io_uring_wait_cqe_timeout(...)
```

或者直接提交 timeout operation：

```cpp
io_uring_prep_timeout(...)
```

例如：

```cpp
io_uring_prep_timeout(
    sqe,
    &ts,
    0,
    0
);
```

以后 RomeServer 做：

```text
idle timeout
connection timeout
```

会用得上。

---

# 12. cancel

异步取消：

```cpp
io_uring_prep_cancel(...)
```

以及一些变体。

以后你可能：

```text
connection close
↓
取消 pending recv
```

这时会用。

但 v0.1 可以先不做。

---

# 13. 文件操作接口

虽然你现在先不做 File，不过顺便认识一下：

| 操作     | 接口                       |
| ------ | ------------------------ |
| read   | `io_uring_prep_read()`   |
| write  | `io_uring_prep_write()`  |
| readv  | `io_uring_prep_readv()`  |
| writev | `io_uring_prep_writev()` |
| open   | `io_uring_prep_openat()` |
| close  | `io_uring_prep_close()`  |
| fsync  | `io_uring_prep_fsync()`  |
| stat   | `io_uring_prep_statx()`  |

所以以后 Rome 加 File，本质结构不会变：

```text
get_sqe
↓
prep_read
↓
set_data
↓
submit
↓
wait CQE
```

正因为这样文件/网络才能大量共享 Backend 基础设施。

---

# 14. Multishot accept

这个以后对 RomeServer 很有意思：

```cpp
io_uring_prep_multishot_accept(...)
```

普通 accept：

```text
提交 ACCEPT
↓
accept 一个 client
↓
CQE
↓
再提交一个 ACCEPT
```

multishot：

```text
提交一次 ACCEPT
↓
client A → CQE
client B → CQE
client C → CQE
...
```

一个 SQE 能产生多个 CQE。

这时候要检查：

```cpp
cqe->flags & IORING_CQE_F_MORE
```

表示：

> 这个 multishot operation 以后还会产生 completion。

但我建议：

> **第一版先普通 accept。**

等正常版跑通再换 multishot。

---

# 15. opcode capability probe

不同 Linux kernel 支持不同 operation。

可以：

```cpp
io_uring_get_probe_ring(&ring);
```

然后：

```cpp
io_uring_opcode_supported(
    probe,
    IORING_OP_ACCEPT
);
```

最后：

```cpp
io_uring_free_probe(probe);
```

以后想支持不同 kernel 很有用。

但如果 v0.1 直接规定：

> Debian 13 / 新 Linux kernel

那先不做也可以。

---

# 16. 注册 Buffer

高级优化：

```cpp
io_uring_register_buffers(...)
```

然后：

```cpp
io_uring_prep_read_fixed(...)
io_uring_prep_write_fixed(...)
```

可以让 kernel 提前认识这些 buffer。

网络还有更先进的 buffer ring 等机制。

但全部属于：

```text
v0.5 性能优化
```

不是现在需要。

---

# 17. 注册 file descriptor

```cpp
io_uring_register_files(...)
```

之后 SQE 可以：

```cpp
sqe->flags |= IOSQE_FIXED_FILE;
```

避免每次 kernel 都重新查 fd。

socket 也可以注册进去。

同样先不要做。

---

# 18. SQE flags

以后经常会看到：

```cpp
sqe->flags |= ...
```

常见：

```text
IOSQE_FIXED_FILE
IOSQE_IO_LINK
IOSQE_IO_HARDLINK
IOSQE_ASYNC
```

其中 `IOSQE_IO_LINK` 很有意思：

```text
operation A
    ↓
operation B
    ↓
operation C
```

可以 link 成一串。

例如以后：

```text
read file
↓
send socket
```

之类可能利用。

现在完全可以不管。

---

# 对 RomeSocket v0.1 真正需要哪些？

其实就这些：

```cpp
io_uring_queue_init()
io_uring_queue_exit()

io_uring_get_sqe()

io_uring_prep_accept()
io_uring_prep_recv()
io_uring_prep_send()

io_uring_sqe_set_data()
io_uring_cqe_get_data()

io_uring_submit()

io_uring_wait_cqe()
io_uring_peek_cqe()

io_uring_cqe_seen()
```

**12 个左右。**

你甚至可以把它们按生命周期记：

```text
初始化

queue_init
    ↓

提交一个 operation

get_sqe
    ↓
prep_xxx
    ↓
set_data
    ↓
submit
    ↓

等待

wait_cqe
    ↓
get_data
    ↓
cqe->res
    ↓
cqe_seen
    ↓

结束

queue_exit
```

如果把 Rome v0.1 的 Backend 展开，其实核心就是：

```cpp
class Backend {
public:
    Backend();
    ~Backend();

    void accept(...);
    void recv(...);
    void send(...);

    Completion wait();

private:
    io_uring ring_;
};
```

这也是为什么我觉得我们现在把范围缩到 **`io_uring + TCP`** 是对的：你先真正把这十几个 API 玩熟，RomeSocket 的第一版基本就能跑起来了。之后再谈 multishot、registered buffers、zero-copy，理解起来会轻松非常多。
