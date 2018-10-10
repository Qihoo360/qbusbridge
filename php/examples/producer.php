<?php

declare(ticks = 1);

require("../qbus.php");

$run = true;

function sig_handler($signo)
{
	global $run;
	$run = false;
}

if (count($argv) > 4)
{
	$cluster = $argv[1];
	$topic = $argv[2];
}
else
{
	print "Invaild parameter!\n";
	print "php producer.php <cluster> <topic> isSyncSend[1:sync 0:async] key[\"\" or ]\n";
	exit(1);
}

pcntl_signal(SIGINT, "sig_handler");

$producer = new QbusProducer;
$ret = $producer->init($cluster, "./producer.log", "./producer.config", $topic);
if ($ret == false)
{
	print "Failed init";
	exit(1);
}

while ($run)
{
	$line = fgets(STDIN);

	# trim new line
	$line = str_replace(array("\r", "\n"), '', $line);
    if (false == $producer->produce($line, strlen($line), $argv[4])) {
        print "Failed to produce\n";
        //Retry to produce
    }
}

$producer->uninit();

?>
