<?php

namespace VVTS\Types;

require_once(dirname(__FILE__) . "/../autoload.php");

class PerClientRoute {
    var $dwIpv4Address;
    var $abIpv6Address;

    function __construct($dwIpv4Address, $abIpv6Address) {
        $this->dwIpv4Address = $dwIpv4Address;
        $this->abIpv6Address = $abIpv6Address;
    }
}

?>