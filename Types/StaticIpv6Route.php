<?php

namespace VVTS\Types;

require_once(dirname(__FILE__) . "/../autoload.php");

class StaticIpv6Route {
    var $abAddress;
    var $dwPrefixLen;

    function __construct($abAddress, $dwPrefixLen) {
        $this->abAddress = $abAddress;
        $this->dwPrefixLen = $dwPrefixLen;
    }
}

?>