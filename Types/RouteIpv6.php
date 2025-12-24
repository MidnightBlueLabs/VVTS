<?php

namespace VVTS\Types;

require_once(dirname(__FILE__) . "/../autoload.php");

class RouteIpv6 {
    var $abSubnet;
    var $dwPrefixLen;
    var $abVia;
    var $szDev;
    var $abSrc;

    function __construct($abSubnet, $dwPrefixLen, $abVia, $szDev, $abSrc) {
        $this->abSubnet = $abSubnet;
        $this->dwPrefixLen = $dwPrefixLen;
        $this->abVia = $abVia;
        $this->szDev = $szDev;
        $this->abSrc = $abSrc;
    }
}

?>