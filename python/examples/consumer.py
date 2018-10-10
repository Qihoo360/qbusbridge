import qbus
import signal
import sys

class Callback(qbus.QbusConsumerCallback):
	def __init__(self):
		qbus.QbusConsumerCallback.__init__(self)

	def deliveryMsg(self, topic, msg, msg_len):
		print msg

        def deliveryMsgForCommitOffset(self, msg_info):
                try:
                    print msg_info.msg
                    consumer.commitOffset(msg_info)
                except:
                     print sys.exc_info()

if (len(sys.argv) > 3):
	cluster = sys.argv[1]
	topic = sys.argv[2]
	group = sys.argv[3]
else:
	print "Invaild parameter!"
	print "Usage: python consumer.py <cluster> <topic> <group>"
	sys.exit(1)

callback = Callback().__disown__()

consumer = qbus.QbusConsumer()
ret = consumer.init(cluster, "./consumer.log", "./consumer.config", callback)
if ret != True:
	print "Failed init"
	sys.exit(1)

#topics = qbus.StringVector();
#topics.push_back("test");

ret = consumer.subscribeOne(group, topic)
if ret != True:
	print "Failed subscribe"
	sys.exit(1)

try:
	consumer.start()
	signal.pause()
except KeyboardInterrupt:
	consumer.stop()

