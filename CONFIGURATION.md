The configuration file is ini format of 3 parts:

```ini
[global]
; ...
[topic]
; ...
[sdk]
; ...
```

The global and topic configurations are from [librdkafka 1.0.x Configuration](https://github.com/edenhill/librdkafka/blob/1.0.x/CONFIGURATION.md). Here're some configurations:

```ini
[global]
; If broker list was set in `init()` methods of producer or consumer,
; or broker.list of [sdk] part has been configured,
; this config won't work.
bootstrap.servers=

; This is the period of stats_cb. We use stats_cb to check if brokers are up again.
; Default: 1 minute
statistics.interval.ms=60000

[topic]
; NOTE: Our default value is earliest, while the rdkafka's default value is latest
auto.offset.reset=earliest

; NOTE: Our SDK will resend the message if the message reached its timeout.
; It's good when some brokers are broken, messages will be redelivered to available brokers.
; For messages with keys, they will be resent to the broker of partition 0.
; But it will reorder the messages and could cause repeated messages.
; Set it to 0 will disable this behavior of our SDK, which also means messages would never be timed out.
message.timeout.ms=300000

[sdk]
; If broker list was set in `init()` methods of producer or consumer,
; this config won't work.
broker.list=
```

If you want to change the log level of our SDK, use:

```ini
[sdk]
; It could be debug, info, warning or error.
; Default: info
log.level=info
```

If you want to see logs of rdkafka, use:

```ini
[global]
; It could be 0 ~ 7, 7 is debug, 6 is info, etc.
; Default: 6
log_level=7
; See rdkafka's configuration
debug=all

[sdk]
; Default: false. If it's true, rdkafka's log will be written to Rdkafka.log
enable.rdkafka.log=true
```

For producers:

```ini
[sdk]
; If you want to send messages synchronously, set true.
; We don't recommend synchrnous send. It still has some potential bugs.
; Default: false
send.sync=false

; When a producer calls `uninit()`, it will flush all queueing messages in case lost of messages.
; This is the max milliseconds it will wait.
; Default: 3 seconds
flush.timeout.ms=3000

; If it's true, the memory will leak after `uninit()`. But if your process will terminate after `uninit()`, the leak affects nothing.
producer.force.terminate=false

; If it's true, the failed messages will be recorded in the local file.
; It's an experimental feature.
; Default: false
enable.record.msg=false
```

For  consumers:

```ini
[sdk]
; The timeout of consumer's poll() in milliseconds
; Default: 100
consumer.poll.time.ms=100

; If it's true, the memory will leak after `stop()`. But if your process will terminate after `uninit()`, the leak affects nothing.
; Default: false
consumer.force.terminate=false
```

If you want to automatically commit offsets using our SDK, use:

```ini
[global]
enable.auto.commit=false

[sdk]
user.manual.commit.offset=false

; The minimal interval between two commits in milliseconds.
; Exceptional, when the consumer calls `stop()`, it will commit offsets ignoring this config.
; Default: 200
manual.commit.time.ms=200
```

If you want to commit offsets by yourself, use:

```ini
[global]
enable.auto.commit=false

[sdk]
user.manual.commit.offset=true
```

Which means you need to use `commitOffsets()` method in `deliveryMsgForCommitOffset()` callback.
