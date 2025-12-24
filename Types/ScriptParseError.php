<?php

namespace VVTS\Types;

require_once(dirname(__FILE__) . "/../autoload.php");

use \Exception;

class ScriptParseError extends Exception {
    var $dwLineNo;
    var $dwCharNo;
    var $szError;

    function __construct($oScriptParseCtx, $szError) {
        $oOffset = $oScriptParseCtx->GetHumanReadableOffset();
        $this->dwLineNo = $oOffset->dwLineNo;
        $this->dwCharNo = $oOffset->dwCharNo;
        $this->szError = $szError;
        parent::__construct($szError . " at line " . $this->dwLineNo . ", position " . $this->dwCharNo);
    }
}

?>