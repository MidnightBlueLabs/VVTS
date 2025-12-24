<?php

namespace VVTS\Types;

require_once(dirname(__FILE__) . "/../autoload.php");

class ScriptStateTransition {
    var $oDestinationState;

    function __construct($oDestinationState) {
        $this->oDestinationState = $oDestinationState;
    }
}

?>