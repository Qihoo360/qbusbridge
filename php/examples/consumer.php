<?php


require("../qbus.php");

#ini_set('memory_limit', '-1');

ini_set('error_reporting', E_ALL);

if (count($argv) > 3)
{
	$cluster = $argv[1];
	$topic = $argv[2];
	$group = $argv[3];
}
else
{
	print "Invaild parameter!"."\n";
	print "usage: php consumer.php <cluster> <topic> <group>"."\n";
	exit(1);
}

$consumer = new QbusConsumer;
$ret = $consumer->init($cluster, "./consumer.log", "./consumer.config");
if ($ret == false)
{
	print "Failed init";
	exit(1);
}

$ret = $consumer->subscribeOne($group, $topic);
if ($ret == false)
{
	print "Failed subscribe";
	exit(1);
}

$run = true;
function sig_handler($signo)
{
    print "stop...\n";
	global $run;
	$run = false;
}

pcntl_signal(SIGINT, "sig_handler");
$consumer->start();

$msg_info = new QbusMsgContentInfo;

$count = 0;
while ($run)
{
    declare(ticks = 1);
    if ($consumer->consume($msg_info))
	{
        echo "topic: ".$msg_info->topic." | msg: ".$msg_info->msg."\n";
    }
}

$consumer->stop();

?>
