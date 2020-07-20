import qbus
import signal
import sys

if len(sys.argv) < 4:
    print "Usage: python producer.py config_path topic_name cluster_name"
    sys.exit(1)

config_path = sys.argv[1]
topic_name = sys.argv[2]
cluster_name = sys.argv[3]

producer = qbus.QbusProducer()
if False == producer.init(cluster_name, "producer.log", config_path, topic_name):
    print "Init failed"
    sys.exit(2)

print "%% Please input messages (Press Ctrl+C or Ctrl+D to exit):"

while True:
    try:
        line = raw_input()
        if False == producer.produce(line, len(line), ""):
            print "Failed to produce"
    except (EOFError, KeyboardInterrupt):
        producer.uninit()
        print "%% Done."
        break
