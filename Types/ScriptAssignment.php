<?php

namespace VVTS\Types;

require_once(dirname(__FILE__) . "/../autoload.php");

use \VVTS\Types\ScriptParseError;
use \VVTS\Types\ScriptLabel;
use \VVTS\Types\ScriptStringLiteral;
use \VVTS\Types\ScriptInvokeBuiltin;

class ScriptAssignment {
    var $oLvalue;
    var $oRvalue;

    function __construct($oLvalue, $oRvalue) {
        $this->oLvalue = $oLvalue;
        $this->oRvalue = $oRvalue;
    }

    function ToString() {
        return $this->oLvalue->ToString() . " = " . $this->oRvalue->ToString();
    }

    static function Parse($oScriptParseCtx) {
        $oScriptParseCtx->ConsumeSpaces();

        try {
            $oLvalue = ScriptLabel::Parse($oScriptParseCtx, true);
        } catch (ScriptParseError $e) {
            $oLvalue = null;
        }
        if (!($oLvalue instanceof ScriptLabel)) {
            throw new ScriptParseError($oScriptParseCtx, "Expected an lvalue for assignment");
        }

        $oScriptParseCtx->ConsumeSpaces();

        if (!$oScriptParseCtx->ConsumeKeyword("=")) {
            throw new ScriptParseError($oScriptParseCtx, "Expected '=' for assignment");
        }

        $oScriptParseCtx->ConsumeSpaces();

        $dwState = $oScriptParseCtx->SaveState();
        try {
            $oString = ScriptStringLiteral::Parse($oScriptParseCtx);
        } catch (ScriptParseError $e) {
            $oString = null;
        }
        if ($oString instanceof ScriptStringLiteral) {
            return new ScriptAssignment($oLvalue, $oString);
        } else {
            $oScriptParseCtx->RestoreState($dwState);
            try {
                $oCall = ScriptInvokeBuiltin::Parse($oScriptParseCtx);
            } catch (ScriptParseError $e) {
                $oCall = null;
            }
            if ($oCall instanceof ScriptInvokeBuiltin) {
                return new ScriptAssignment($oLvalue, $oCall);
            } else {
                $oScriptParseCtx->RestoreState($dwState);
                try {
                    $oLabel = ScriptLabel::Parse($oScriptParseCtx, true);
                } catch (ScriptParseError $e) {
                    $oLabel = null;
                }
                if ($oLabel instanceof ScriptLabel) {
                    return new ScriptAssignment($oLvalue, $oLabel);
                } else {
                    return $oLabel;
                }
            }
        }
    }
}

?>