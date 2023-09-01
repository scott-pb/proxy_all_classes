--TEST--
Check if proxy_all_classes is loaded
--EXTENSIONS--
proxy_all_classes
--FILE--
<?php
echo 'The extension "proxy_all_classes" is available';
?>
--EXPECT--
The extension "proxy_all_classes" is available
