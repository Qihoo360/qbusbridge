import qbus
import signal
import sys


if (len(sys.argv) > 4):
	cluster = sys.argv[1]
	topic = sys.argv[2]
else:
	print "Invaild parameter!"
	print "Usage: python producer.py <cluster> <topic> isSyncSend[1:sync 0:async] key[\"\" or ]"
	sys.exit(1)

producer = qbus.QbusProducer()
ret = producer.init(cluster, "./producer.log", "./producer.config", topic)
if ret != True:
	print "Failed init"
	sys.exit(1)

run = True
while run == True:
	try:
		line = raw_input()
		ret = producer.produce(line, len(line), sys.argv[4])
                if ret != True:
                    print "Failed to produce\n"
                    #Retry to produce
	except KeyboardInterrupt:
		producer.uninit()
		run = False

