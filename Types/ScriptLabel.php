<?php

namespace VVTS\Types;

require_once(dirname(__FILE__) . "/../autoload.php");

use \VVTS\Types\ScriptParseError;

class ScriptLabel {
    var $szVariable;
    var $szLabel;

    function __construct($szVariable, $szLabel) {
        $this->szVariable = $szVariable;
        $this->szLabel = $szLabel;
    }

    function ToString() {
        return ($this->szVariable != null ? $this->szVariable . "." : "") . $this->szLabel;
    }

    static function Parse($oScriptParseCtx, $bObjectLabel) {
        $szVariable = null;
        $szLabel = "";
        $oScriptParseCtx->SkipComment();

        if ($oScriptParseCtx->dwOffset < strlen($oScriptParseCtx->szScript) && ((
            ord($oScriptParseCtx->szScript[$oScriptParseCtx->dwOffset]) >= ord('a') &&
            ord($oScriptParseCtx->szScript[$oScriptParseCtx->dwOffset]) <= ord('z')
        ) || (
            ord($oScriptParseCtx->szScript[$oScriptParseCtx->dwOffset]) >= ord('A') &&
            ord($oScriptParseCtx->szScript[$oScriptParseCtx->dwOffset]) <= ord('Z')
        ) || (
            $oScriptParseCtx->szScript[$oScriptParseCtx->dwOffset] == '_'
        ))) {
            $szLabel = $oScriptParseCtx->szScript[$oScriptParseCtx->dwOffset];
            $oScriptParseCtx->dwOffset++;
            $oScriptParseCtx->SkipComment();

            while ($oScriptParseCtx->dwOffset < strlen($oScriptParseCtx->szScript) && ((
                ord($oScriptParseCtx->szScript[$oScriptParseCtx->dwOffset]) >= ord('a') &&
                ord($oScriptParseCtx->szScript[$oScriptParseCtx->dwOffset]) <= ord('z')
            ) || (
                ord($oScriptParseCtx->szScript[$oScriptParseCtx->dwOffset]) >= ord('A') &&
                ord($oScriptParseCtx->szScript[$oScriptParseCtx->dwOffset]) <= ord('Z')
            ) || (
                ord($oScriptParseCtx->szScript[$oScriptParseCtx->dwOffset]) >= ord('0') &&
                ord($oScriptParseCtx->szScript[$oScriptParseCtx->dwOffset]) <= ord('9')
            ) || $oScriptParseCtx->szScript[$oScriptParseCtx->dwOffset] == '_'
              || ($bObjectLabel && $oScriptParseCtx->szScript[$oScriptParseCtx->dwOffset] == '.')
            )) {
                $szLabel .= $oScriptParseCtx->szScript[$oScriptParseCtx->dwOffset];
                $oScriptParseCtx->dwOffset++;
                $oScriptParseCtx->SkipComment();
            }
        }

        if ($szLabel === "") {
            throw new ScriptParseError($oScriptParseCtx, "Invalid first character for label");
        }

        if (strpos($szLabel, '.') !== false) {
            list($szVariable, $szLabel) = explode('.', $szLabel, 2);
            if (empty($szVariable) || empty($szLabel)) {
                throw new ScriptParseError($oScriptParseCtx, "Variable or member cannot be empty");
            }
        }
        return new ScriptLabel($szVariable, $szLabel);
    }
}

?>