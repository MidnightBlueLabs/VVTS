<?php

spl_autoload_register(function($szClass) {
    require_once(dirname(__FILE__) . "/" . preg_replace('/^(?:\/?VVTS)?\/?/i', "", str_replace("\\", "/", $szClass)) . ".php");
});

?>