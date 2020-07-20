import qbus
import signal
import sys

class Callback(qbus.QbusConsumerCallback):
    def __init__(self):
        qbus.QbusConsumerCallback.__init__(self)

    def setConsumer(self, consumer):
        self.consumer = consumer

    def deliveryMsg(self, topic, msg, msg_len):
        print "Topic: %s | msg: %s" % (topic, msg)

    def deliveryMsgForCommitOffset(self, info):
        print "User commit offset | Topic: %s | msg: %s" % (info.topic, info.msg)
        self.consumer.CommitOffset(info)


if len(sys.argv) < 5:
    print "Usage: ./consumer config_path topic_name group_name cluster_name"
    sys.exit(1)

config_path = sys.argv[1]
topic_name = sys.argv[2]
group = sys.argv[3]
cluster_name = sys.argv[4]

print "topic: %s | group: %s | cluster: %s" % (topic_name, group, cluster_name)

callback = Callback().__disown__()
consumer = qbus.QbusConsumer()
callback.setConsumer(consumer)

if False == consumer.init(cluster_name, "consumer.log", config_path, callback):
    print "Init failed"
    sys.exit(2)

if False == consumer.subscribeOne(group, topic_name):
    print "SubscribeOne failed"
    sys.exit(3)

if False == consumer.start():
    print "Start failed"
    sys.exit(4)

try:
    signal.pause()
except KeyboardInterrupt:
    consumer.stop()
    print "done"
