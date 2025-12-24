<?php

namespace VVTS\Classes;

require_once(dirname(__FILE__) . "/../autoload.php");

use \stdClass;

class ScriptParseCtx {
    var $szScript;
    var $dwOffset;

    function __construct($szScript) {
        $this->szScript = $szScript;
        $this->dwOffset = 0;
    }

    function SkipComment() {
        $dwOffset = $this->dwOffset;

        if ($dwOffset+1 < strlen($this->szScript) &&
            $this->szScript[$dwOffset] === '(' &&
            $this->szScript[$dwOffset+1] === '*'
        ) {
            $dwOffset += 2;
            while (
                $dwOffset < strlen($this->szScript) && (
                    $this->szScript[$dwOffset] != '*' ||
                    $dwOffset+1 == strlen($this->szScript) ||
                    $this->szScript[$dwOffset+1] != ')'
                )
            ) {
                $dwOffset++;
            }

            if ($dwOffset < strlen($this->szScript)) {
                $dwOffset += 2;
            }

            $this->dwOffset = $dwOffset;
        }
    }

    function GetHumanReadableOffset() {
        $dwLineNo = 1;
        $dwCharNo = 0;

        for ($i = 0; $i < $this->dwOffset; $i++) {
            if ($this->szScript[$i] === "\n") {
                $dwLineNo++;
                $dwCharNo = 0;
            } else {
                $dwCharNo++;
            }
        }

        $oOutput = new stdClass;
        $oOutput->dwLineNo = $dwLineNo;
        $oOutput->dwCharNo = $dwCharNo;
        return $oOutput;
    }

    function ConsumeSpaces() {
        $bSkipped = false;

        $this->SkipComment();

        while ($this->dwOffset < strlen($this->szScript) && (
            $this->szScript[$this->dwOffset] === " " ||
            $this->szScript[$this->dwOffset] === "\t" ||
            $this->szScript[$this->dwOffset] === "\r" ||
            $this->szScript[$this->dwOffset] === "\n"
        )) {
            $this->dwOffset++;
            $this->SkipComment();
            $bSkipped = true;
        }

        return $bSkipped;
    }

    function SaveState() {
        return $this->dwOffset;
    }

    function RestoreState($dwState) {
        $this->dwOffset = $dwState;
    }

    function ConsumeKeyword($szString) {
        $dwState = $this->SaveState();

        $this->SkipComment();
        for($i = 0; $i < strlen($szString) && $this->dwOffset < strlen($this->szScript); $i++) {
            if ($szString[$i] === $this->szScript[$this->dwOffset]) {
                $this->dwOffset++;
                $this->SkipComment();
            } else {
                $this->RestoreState($dwState);
                return false;
            }
        }

        return true;
    }
}

?>