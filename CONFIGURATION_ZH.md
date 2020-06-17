配置文件是 INI 格式，分为 3 部分：

```ini
[global]
; ...
[topic]
; ...
[sdk]
; ...
```

global 和 topic 配置参考 [librdkafka 1.0.x 配置](https://github.com/edenhill/librdkafka/blob/1.0.x/CONFIGURATION.md)。以下是一些配置：

```ini
[global]
; 如果在 init() 方法中设置了 broker list，
; 或者 [sdk] 部分配置了 broker.list，
; 那么这个配置不起作用。
bootstrap.servers=

; stats 回调被调用的周期，我们使用 stats 回调检测 broker 是否重新变成 UP 状态
; 默认：1 分钟
statistics.interval.ms=60000

[topic]
; 注意：我们的默认值是 earliest，而 rdkafka 的默认值是 latest
auto.offset.reset=earliest

; 注意：我们的 SDK 在消息超时的场合会重发消息。这在有些 broker 挂掉的情况下很好，消息会
; 重发到可用的 broker。
; 对于带 key 的消息，它们只会被重发到分区 0 对应的 broker。
; 但是这种行为会导致消息的乱序，也可能导致消息的重复发送，因为超时并不意味着消息送达失败。
; 将它设为 0 来禁止我们 SDK 的这种行为，这也意味着消息将永不超时。
message.timeout.ms=300000

[sdk]
; 如果在 init() 方法中设置了 broker list，
; 那么这个配置不起作用。
broker.list=
```

修改 SDK 的日志等级：

```ini
[sdk]
; 可以是 debug, info, warning 或 error.
; 默认：info
log.level=info
```

如果要查看 rdkafka 的日志，使用以下配置：

```ini
[global]
; 0 到 7 之间的整数（包含 0 和 7）。7 是 debug，6 是 info，等等。
; 默认: 6
log_level=7
; 见 rdkafka 的配置
debug=all

[sdk]
; 默认：false。若设为 true，则 rdkafka 的日志会写到本地的 Rdkafka.log 文件。
enable.rdkafka.log=true
```

对于生产者：

```ini
[sdk]
; 设为 true 则同步发送。不推荐同步发送，它目前存在潜在的 BUG。
; 默认：false
send.sync=false

; 当生产者调用 uninit() 时，它会冲刷所有排队等待的消息以防止消息丢失。该配置是 flush 操作会等待的最大时间。
; 默认：3 秒
flush.timeout.ms=3000

; 若设为 true，则即使生产者调用 uninit() 也会造成内存泄漏。但如果 uninit() 后进程退出了，则内存泄漏没有关系。
; 默认：false
producer.force.terminate=false

; 若设为 true，则发送失败的消息会记录到本地文件中，这是个实验性的功能，不建议使用。
; 默认：false
enable.record.msg=false
```

对于消费者：

```ini
[sdk]
; 消费者调用 poll() 的最大等待时间，单位：毫秒
; 默认：100
consumer.poll.time.ms=100

; 若设为 true，则即使消费者调用 stop() 也会造成内存泄漏。但如果 stop() 后进程退出了，则内存泄漏没有关系。
; 默认：false
consumer.force.terminate=false
```

如果想要使用我们 SDK 来自动提交 offset：

```ini
[global]
enable.auto.commit=false

[sdk]
user.manual.commit.offset=false

; 两次提交之间的最小时间间隔，单位：毫秒
; 例外地，消费者调用 stop() 时也会提交 offset，无视这个配置。
; 默认：200
manual.commit.time.ms=200
```

如果想要自己提交 offset：

```ini
[global]
enable.auto.commit=false

[sdk]
user.manual.commit.offset=true
```

这意味着你需要在 `deliveryMsgForCommitOffset()` 回调中使用 `commitOffsets()` 方法。
