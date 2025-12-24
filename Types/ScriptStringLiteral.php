<?php

namespace VVTS\Types;

require_once(dirname(__FILE__) . "/../autoload.php");

use \VVTS\Types\ScriptParseError;

class ScriptStringLiteral {
    var $szLiteral;

    function __construct($szLiteral) {
        $this->szLiteral = $szLiteral;
    }

    function ToString() {
        return '"' . str_replace("\"", "\\\"", str_replace("\\", "\\\\", $this->szLiteral)) . '"';
    }

    static function Parse($oScriptParseCtx) {
        $oScriptParseCtx->ConsumeSpaces();
        $oScriptParseCtx->SkipComment();

        if (
            $oScriptParseCtx->dwOffset < strlen($oScriptParseCtx->szScript) && (
                $oScriptParseCtx->szScript[$oScriptParseCtx->dwOffset] === '"' ||
                $oScriptParseCtx->szScript[$oScriptParseCtx->dwOffset] === '\''
        )) {
            $bQuoteChar = $oScriptParseCtx->szScript[$oScriptParseCtx->dwOffset];
            $oScriptParseCtx->dwOffset++;
            $oScriptParseCtx->SkipComment();
        } else {
            throw new ScriptParseError($oScriptParseCtx, "Expected a \" or ' at the beginning of a string");
        }

        $bEscape = false;
        $szLiteral = "";

        for (;;) {
            if (!$bEscape) {
                if (
                    $oScriptParseCtx->dwOffset < strlen($oScriptParseCtx->szScript) &&
                    $oScriptParseCtx->szScript[$oScriptParseCtx->dwOffset] === '\\'
                ) {
                    $bEscape = true;
                    $oScriptParseCtx->dwOffset++;
                    $oScriptParseCtx->SkipComment();
                } else if (
                    $oScriptParseCtx->dwOffset < strlen($oScriptParseCtx->szScript) &&
                    $oScriptParseCtx->szScript[$oScriptParseCtx->dwOffset] === $bQuoteChar
                ) {
                    $oScriptParseCtx->dwOffset++;
                    $oScriptParseCtx->SkipComment();
                    break;
                } else if ($oScriptParseCtx->dwOffset >= strlen($oScriptParseCtx->szScript)) {
                    throw new ScriptParseError($oScriptParseCtx, "Expected " . $bQuoteChar . " to conclude a string");
                } else {
                    $szLiteral .= $oScriptParseCtx->szScript[$oScriptParseCtx->dwOffset];
                    $oScriptParseCtx->dwOffset++;
                    $oScriptParseCtx->SkipComment();
                }
            } else {
                if ($oScriptParseCtx->dwOffset >= strlen($oScriptParseCtx->szScript)) {
                    throw new ScriptParseError($oScriptParseCtx, "Expected " . $bQuoteChar . " to conclude a string");
                }
                $szLiteral .= $oScriptParseCtx->szScript[$oScriptParseCtx->dwOffset];
                $bEscape = false;
                $oScriptParseCtx->dwOffset++;
                $oScriptParseCtx->SkipComment();
            }
        }

        return new ScriptStringLiteral($szLiteral);
    }
}

?>