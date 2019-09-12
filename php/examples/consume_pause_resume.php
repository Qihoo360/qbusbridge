<?php

require("../qbus.php");

ini_set('error_reporting', E_ALL);

if (count($argv) < 3)
{
	print "Invaild parameter!\n";
	print "usage: php consumer.php <cluster> <topic> <group>\n";
	exit(1);
}

$cluster = $argv[1];
$topic   = $argv[2];
$group   = $argv[3];

# msg_batch_size为一个批次的消息数量，每次消费该数量的消息，就会暂停消费该topic
# 等到pause_seconds秒后恢复消费，即模拟对该批次的消息进行消费的时间
$msg_batch_size = 2;
$pause_seconds  = 3;

$consumer = new QbusConsumer;

if (!$consumer->init($cluster, "./consumer.log", "./consumer.config"))
{
    print "Consumer.init() failed\n";
    exit(1);
}

$topics = new StringVector;
$topics->push($topic);
if (!$consumer->subscribe($group, $topics))
{
    print "Consumer.subscribe() failed\n";
    exit(1);
}

function now()
{
    list($msec, $sec) = explode(' ', microtime());
    return sprintf("%s.%03d", date("Y-m-d H:i:s", $sec), (int)($msec * 1000));
}

$run = true;
function sig_handler($signo)
{
    switch ($signo) {
    case SIGINT:
        global $run;
        $run = false;
        break;
    case SIGALRM:
        global $consumer;
        global $topics;
        global $pause_seconds;
        printf("%s | Resume consuming %s for %s seconds\n", now(), $topics->get(0), $pause_seconds);
        if (!$consumer->resume($topics))
        {
            print "Consumer.resume() failed\n";
            exit(1);
        }
        break;
    default:
        printf("Unknown signal: %d\n", $signo);
        exit(1);
    }
}

pcntl_signal(SIGINT, "sig_handler");
pcntl_signal(SIGALRM, "sig_handler");
if (!$consumer->start())
{
    print "Consumer.start() failed\n";
    exit(1);
}
print "Start to consume...\n";

$msg_info = new QbusMsgContentInfo;

$msg_cnt = 0;
while ($run)
{
    declare(ticks = 1);
    if ($consumer->consume($msg_info))
    {
        $msg_cnt++;
        # 实际场景下会保存该消息到用户自定义的数据结构中，以实现分批消费
        printf("[%d] %s | %s\n", $msg_cnt, $topics->get(0), $msg_info->msg);
        if ($msg_cnt % $msg_batch_size == 0)
        {
            printf("%s | Pause consuming %s for %d seconds\n", now(), $topics->get(0), $pause_seconds);
            if (!$consumer->pause($topics))
            {
                print "Consumer.pause() failed\n";
                exit(1);
            }
            pcntl_alarm($pause_seconds);
        }
    }
}

$msg_info = NULL;
$consumer->stop();
$consumer = NULL;
print "\nStopped\n";

?>
