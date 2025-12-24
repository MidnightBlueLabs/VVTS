<?php

namespace VVTS\Types;

require_once(dirname(__FILE__) . "/../autoload.php");

use \VVTS\Types\ScriptParseError;
use \VVTS\Types\ScriptState;

class ScriptDocument {
    var $aStates;

    function __construct($aStates) {
        $this->aStates = $aStates;
    }

    function ToString() {
        $szOutput = "";

        foreach($this->aStates as $i => $oState) {
            $szOutput .= $oState->ToString();
            if ($i != (count($this->aStates)-1)) {
                $szOutput .= "\n";
            }
        }

        return $szOutput;
    }

    static function Parse($oScriptParseCtx) {
        $aStates = [];
        $oState = null;
        $dwNumInitStates = 0;
        $oException = null;
        for(;;) {
            try {
                $oState = ScriptState::Parse($oScriptParseCtx);
            } catch (ScriptParseError $e) {
                $oException = $e;
                break;
            }
            array_push($aStates, $oState);
            if ($oState->bIsInitState) {
                $dwNumInitStates++;
            }
        }

        $oScriptParseCtx->ConsumeSpaces();

        if ($oScriptParseCtx->dwOffset === strlen($oScriptParseCtx->szScript)) {
            if ($dwNumInitStates !== 1) {
                throw new ScriptParseError($oScriptParseCtx, "Expected exactly one initial state (have " . $dwNumInitStates . ")");
            }
            return new ScriptDocument($aStates);
        }

        if ($oException !== null) {
            throw $oException;
        }

        throw new ScriptParseError($oScriptParseCtx, "Expected state definition or EOF");
    }
}

?>