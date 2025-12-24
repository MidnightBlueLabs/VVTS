<?php

namespace VVTS\Types;

require_once(dirname(__FILE__) . "/../autoload.php");

use \VVTS\Types\ScriptParseError;
use \VVTS\Types\ScriptState;
use \VVTS\Types\ScriptLabel;
use \VVTS\Types\ScriptAssignment;
use \VVTS\Types\ScriptInvokeBuiltin;

class ScriptState {
    var $oLabel;
    var $bIsInitState;
    var $aStatements;

    function __construct($oLabel, $bIsInitState, $aStatements) {
        $this->oLabel = $oLabel;
        $this->bIsInitState = $bIsInitState;
        $this->aStatements = $aStatements;
    }

    function ToString() {
        $szOutput = "state " . ($this->bIsInitState ? "init " : "") . $this->oLabel->ToString() . ":\n";
        foreach($this->aStatements as $oStatement) {
            $szOutput .= "  " . $oStatement->ToString() . ";\n";
        }
        return $szOutput;
    }

    static function Parse($oScriptParseCtx) {
        $oScriptParseCtx->ConsumeSpaces();

        if ($oScriptParseCtx->ConsumeKeyword("state") &&
            $oScriptParseCtx->ConsumeSpaces()
        ) {
            $dwStateBeforeInitKeyword = $oScriptParseCtx->SaveState();
            if ($oScriptParseCtx->ConsumeKeyword("init") &&
                $oScriptParseCtx->ConsumeSpaces()
            ) {
                $bIsInitState = true;
            } else {
                $bIsInitState = false;
                $oScriptParseCtx->RestoreState($dwStateBeforeInitKeyword);
            }

            try {
                $oLabel = ScriptLabel::Parse($oScriptParseCtx, false);
            } catch (ScriptParseError $e) {
                $oLabel = null;
            }
            if (!($oLabel instanceof ScriptLabel)) {
                throw new ScriptParseError($oScriptParseCtx, "Expected a state label");
            }

            $oScriptParseCtx->ConsumeSpaces();

            if (!$oScriptParseCtx->ConsumeKeyword(":")) {
                throw new ScriptParseError($oScriptParseCtx, "Expected ':'");
            }

            $aStatements = [];
            for(;;) {
                $dwState = $oScriptParseCtx->SaveState();
                try {
                    $oAssignment = ScriptAssignment::Parse($oScriptParseCtx);
                } catch (ScriptParseError $e) {
                    $oAssignment = null;
                }
                if ($oAssignment instanceof ScriptAssignment) {
                    array_push($aStatements, $oAssignment);
                } else {
                    $oScriptParseCtx->RestoreState($dwState);
                    try {
                        $oCall = ScriptInvokeBuiltin::Parse($oScriptParseCtx);
                    } catch (ScriptParseError $e) {
                        $oCall = null;
                    }
                    if ($oCall instanceof ScriptInvokeBuiltin) {
                        array_push($aStatements, $oCall);
                    } else {
                        $oScriptParseCtx->RestoreState($dwState);
                        break;
                    }
                }

                $oScriptParseCtx->ConsumeSpaces();
                if (!$oScriptParseCtx->ConsumeKeyword(";")) {
                    throw new ScriptParseError($oScriptParseCtx, "Expected ';'");
                }
            }
            return new ScriptState($oLabel, $bIsInitState, $aStatements);
        } else {
            throw new ScriptParseError($oScriptParseCtx, "Expected 'state' keyword");
        }
    }
}

?>