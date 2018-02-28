<?php

$f = fgets(STDIN);

$params = explode("&", $f);

$pairs = array();

if(!empty($params)) {
    foreach($params as $keyval) {
        $keyval_aux = explode("=", $keyval);
        if(!empty($keyval_aux)) {
            $pairs[$keyval_aux[0]] = trim($keyval_aux[1]);
        }
    }
}

if(isset($pairs["name"])) {
    fwrite(STDOUT, "Hello there, ".$pairs["name"]."!\r\n\r\n");
} else {
    fwrite(STDOUT, "Hello there, my unnamed friend!\r\n\r\n");
}

?>
