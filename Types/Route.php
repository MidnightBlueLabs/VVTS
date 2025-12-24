<?php

namespace VVTS\Types;

require_once(dirname(__FILE__) . "/../autoload.php");

class Route {
    var $dwSubnet;
    var $dwNetmask;
    var $dwVia;
    var $szDev;
    var $dwSrc;

    function __construct($dwSubnet, $dwNetmask, $dwVia, $szDev, $dwSrc) {
        $this->dwSubnet = $dwSubnet;
        $this->dwNetmask = $dwNetmask;
        $this->dwVia = $dwVia;
        $this->szDev = $szDev;
        $this->dwSrc = $dwSrc;
    }
}

?>