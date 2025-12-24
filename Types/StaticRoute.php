<?php

namespace VVTS\Types;

require_once(dirname(__FILE__) . "/../autoload.php");

class StaticRoute {
    var $dwAddress;
    var $dwNetmask;
    var $dwGateway;

    function __construct($dwAddress, $dwNetmask, $dwGateway) {
        $this->dwAddress = $dwAddress;
        $this->dwNetmask = $dwNetmask;
        $this->dwGateway = $dwGateway;
    }
}

?>