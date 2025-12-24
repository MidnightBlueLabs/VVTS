<?php

namespace VVTS\Types;

require_once(dirname(__FILE__) . "/../autoload.php");

class TimerEvent {
    var $oObject;
    var $lpHandler;
    var $lpArgument;
    var $dwTimestamp;

    function __construct($oObject, $lpHandler, $lpArgument, $dwTimestamp) {
        $this->oObject = $oObject;
        $this->lpHandler = $lpHandler;
        $this->lpArgument = $lpArgument;
        $this->dwTimestamp = $dwTimestamp;
    }
}

?>