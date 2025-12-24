<?php

namespace VVTS\Types;

require_once(dirname(__FILE__) . "/../autoload.php");

class DnsTransactionMapEntry {
    var $hSocket;
    var $szPeer;
    var $dwExpire;

    function __construct($hSocket, $szPeer, $dwExpire) {
        $this->hSocket = $hSocket;
        $this->szPeer = $szPeer;
        $this->dwExpire = $dwExpire;
    }
}

?>