<?php

namespace VVTS\Types;

require_once(dirname(__FILE__) . "/../autoload.php");

class SignalSubscriber {
    var $szSignal;
    var $oCallbackObj;
    var $szCallbackFunction;
    var $lpCallbackArgument;
    var $bOneShot;

    function __construct($szSignal, $oCallbackObj, $szCallbackFunction, $lpCallbackArgument, $bOneShot) {
        $this->szSignal = $szSignal;
        $this->oCallbackObj = $oCallbackObj;
        $this->szCallbackFunction = $szCallbackFunction;
        $this->lpCallbackArgument = $lpCallbackArgument;
        $this->bOneShot = $bOneShot;
    }
}

?>