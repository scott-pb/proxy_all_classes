--TEST--
test1() Basic test
--EXTENSIONS--
proxy_all_classes
--FILE--
<?php
$ret = test1();

var_dump($ret);
?>
--EXPECT--
The extension proxy_all_classes is loaded and working!
NULL
