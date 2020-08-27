<?php

declare(ticks = 1);

require("./qbus.php");

if (count($argv) < 4)
{
    echo "Usage: php producer.php config_path topic_name cluster_name\n";
    exit(1);
}

$config = $argv[1];
$topic = $argv[2];
$cluster = $argv[3];

$producer = new QbusProducer;
if (false == $producer->init($cluster, "./producer.log", $config, $topic))
{
    echo "Init failed\n";
    exit(1);
}

echo "%% Please input messages (Press Ctrl+D to exit):\n";

while (true)
{
    $line = fgets(STDIN);
    if (feof(STDIN)) {
        break;
    }

    $line = str_replace(array("\r", "\n"), '', $line);
    if (false == $producer->produce($line, strlen($line), ""))
    {
        echo "Failed to produce\n";
    }
}

$producer->uninit();
echo "%% Done.\n";

?>
