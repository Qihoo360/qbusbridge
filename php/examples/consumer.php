<?php


require("./qbus.php");

ini_set('error_reporting', E_ALL);

if (count($argv) < 5)
{
    print "Usage: php consumer.php config_path topic_name group_name cluster_name\n";
    exit(1);
}

$config = $argv[1];
$topic = $argv[2];
$group = $argv[3];
$cluster = $argv[4];

echo "topic: ". $topic . " | group: " . $group . " | cluster: " . $cluster;

$consumer = new QbusConsumer;
if (false == $consumer->init($cluster, "./consumer.log", $config))
{
    echo "Init failed\n";
    exit(2);
}

if (false == $consumer->subscribeOne($group, $topic))
{
    echo "subscribeOne failed\n";
    exit(3);
}

$run = true;
function sig_handler($signo)
{
    global $run;
    $run = false;
}

pcntl_signal(SIGINT, "sig_handler");
if (false == $consumer->start())
{
    echo "start failed";
    exit(4);
}

$info = new QbusMsgContentInfo;

$count = 0;
while ($run)
{
    declare(ticks = 1);
    if ($consumer->consume($info))
    {
        echo "topic: ".$info->topic." | msg: ".$info->msg."\n";
    }
}

$consumer->stop();
echo "done";

?>
