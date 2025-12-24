<?php

namespace VVTS\Types;

require_once(dirname(__FILE__) . "/../autoload.php");

use \VVTS\Types\ScriptParseError;
use \VVTS\Types\ScriptLabel;
use \VVTS\Types\ScriptStringLiteral;

class ScriptInvokeBuiltin {
    var $oLabel;
    var $aArguments;

    function __construct($oLabel, $aArguments) {
        $this->oLabel = $oLabel;
        $this->aArguments = $aArguments;
    }

    function ToString() {
        $szOutput = $this->oLabel->ToString() . "(";
        foreach($this->aArguments as $i => $oArgument) {
            $szOutput .= $oArgument->ToString();
            if ($i != (count($this->aArguments)-1)) {
                $szOutput .= ", ";
            }
        }
        $szOutput .= ")";
        return $szOutput;
    }

    static function Parse($oScriptParseCtx) {
        $oScriptParseCtx->ConsumeSpaces();

        try {
            $oLabel = ScriptLabel::Parse($oScriptParseCtx, true);
        } catch (ScriptParseError $e) {
            $oLabel = null;
        }
        if (!($oLabel instanceof ScriptLabel)) {
            throw new ScriptParseError($oScriptParseCtx, "Expected a function label");
        }

        $oScriptParseCtx->ConsumeSpaces();

        if(!$oScriptParseCtx->ConsumeKeyword("(")) {
            throw new ScriptParseError($oScriptParseCtx, "Expected a '('");
        }

        $oScriptParseCtx->ConsumeSpaces();

        $aArguments = [];

        for(;;) {
            $dwState = $oScriptParseCtx->SaveState();
            try {
                $oString = ScriptStringLiteral::Parse($oScriptParseCtx);
            } catch (ScriptParseError $e) {
                $oString = null;
            }
            if ($oString instanceof ScriptStringLiteral) {
                array_push($aArguments, $oString);
            } else {
                $oScriptParseCtx->RestoreState($dwState);
                try {
                    $oCall = ScriptInvokeBuiltin::Parse($oScriptParseCtx);
                } catch (ScriptParseError $e) {
                    $oCall = null;
                }
                if ($oCall instanceof ScriptInvokeBuiltin) {
                    array_push($aArguments, $oCall);
                } else {
                    $oScriptParseCtx->RestoreState($dwState);
                    try {
                        $oArgLabel = ScriptLabel::Parse($oScriptParseCtx, true);
                    } catch (ScriptParseError $e) {
                        $oArgLabel = null;
                    }
                    if ($oArgLabel instanceof ScriptLabel) {
                        array_push($aArguments, $oArgLabel);
                    } else {
                        break;
                    }
                }
            }

            $oScriptParseCtx->ConsumeSpaces();
            if ($oScriptParseCtx->ConsumeKeyword(",")) {
                $oScriptParseCtx->ConsumeSpaces();
                continue;
            }
            break;
        }

        if ($oScriptParseCtx->ConsumeKeyword(")")) {
            return new ScriptInvokeBuiltin($oLabel, $aArguments);
        } else {
            throw new ScriptParseError($oScriptParseCtx, "Expected a ',' or ')'");
        }
    }
}

?>