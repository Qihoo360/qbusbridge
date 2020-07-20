# -*- coding: utf-8 -*-
import qbus
import signal
import sys
import time

# msg_batch_size为一个批次的消息数量，每次消费该数量的消息，就会暂停消费该topic
# 等到pause_seconds秒后恢复消费，即模拟对该批次的消息进行消费的时间
msg_batch_size = 2
pause_seconds = 3

# 将consumer和topic保存为全局变量传递给信号处理器中消费者的resume()方法
# 实际场景下可以通过线程或者其他方式在方法之间传递这些对象
consumer = qbus.QbusConsumer()
topic = ""

def now():
    ct = time.time()
    date = time.strftime('%Y-%m-%d %H:%M:%S', time.localtime(ct))
    milli_secs = (ct - int(ct)) * 1000
    return "%s.%03d" % (date, milli_secs)


def alrm_handler(signum, frame):
    topics = qbus.StringVector()
    topics.push_back(topic)
    if consumer.resume(topics):
        print '%s | Resume consuming %s' % (now(), topic)
    else:
        print 'Consumer.resume("%s") failed' % topic


class Callback(qbus.QbusConsumerCallback):
    def __init__(self):
        qbus.QbusConsumerCallback.__init__(self)

    def deliveryMsg(self, topic, msg, msg_len):
        self.msg_cnt = self.msg_cnt + 1
        # 实际场景下会保存该消息到用户自定义的数据结构中，以实现分批消费
        print '[%d] %s | %s' % (self.msg_cnt, topic, msg[0:msg_len])

        if self.msg_cnt % msg_batch_size == 0:
            topics = qbus.StringVector()
            topics.push_back(topic)
            if consumer.pause(topics):
                print '%s | Pause consuming %s for %d seconds' % (now(), topic, pause_seconds)
                signal.alarm(pause_seconds)
            else:
                print 'Consumer.pause("%s") failed' % topic
                return


if __name__ == '__main__':
    if len(sys.argv) < 2 or sys.argv[1] == '-h' or sys.argv[1] == '--help':
        print 'Usage: python %s <cluster> <topic> <group>' % sys.argv[0]
        sys.exit(1)

    cluster = sys.argv[1]
    topic = sys.argv[2]
    group = sys.argv[3]

    callback = Callback().__disown__()

    if not consumer.init(cluster, './consumer.log', './consumer.config', callback):
        print 'Consumer.init() failed'
        sys.exit(1)

    if not consumer.subscribeOne(group, topic):
        print 'Consumer.subscribeOne("%s", "%s") failed' % (group, topic)
        sys.exit(1)

    callback.msg_cnt = 0
    if not consumer.start():
        print 'Consumer.start() failed'
        sys.exit(1)

    print 'Start to consume...'
    signal.signal(signal.SIGALRM, alrm_handler)
    while True:
        try:
            signal.pause()
        except KeyboardInterrupt:
            consumer.stop()
            print '\nStopped.'
            break
