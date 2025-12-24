<?php

namespace VVTS\Types;

require_once(dirname(__FILE__) . "/../autoload.php");

use \VVTS\Classes\MiscNet;

class VpnEndpointDescriptor {
    var $bProtocol;
    var $szHostname;
    var $wPort;

    function __construct($bProtocol, $szHostname, $wPort) {
        $this->bProtocol = $bProtocol;
        $this->szHostname = $szHostname;
        $this->wPort = $wPort;
    }

    function ToString() {
        switch($this->bProtocol) {
            case 6: $szOutput = "tcp://"; break;
            case 17: $szOutput = "udp://"; break;
            case 47: $szOutput = "gre://"; break;
            case 50: $szOutput = "ipsec://"; break;
            case 115: $szOutput = "l2tp://"; break;
            default: $szOutput = "unknown://"; break;
        }

        if (MiscNet::Ipv6StringToBinary($this->szHostname) !== false) {
            $szOutput .= "[" . $this->szHostname . "]";
        } else {
            $szOutput .= $this->szHostname;
        }
        if ($this->wPort !== null) {
            $szOutput .= ":" . $this->wPort;
        }

        return $szOutput;
    }

    static function FromString($szString) {
        if (!preg_match('/^(tcp|udp):\/\/(\[[0-9a-f:\.]+\]|[^:@\/\s]+):([0-9]+)$/', $szString, $aMatchTcpUdp) &&
            !preg_match('/^(gre|ipsec|l2tp):\/\/(\[[0-9a-f:\.]+\]|[^:@\/\s]+)$/', $szString, $aMatchGreIpsecL2tp)
        ) {
            return null;
        }

        $szHostname = (count($aMatchTcpUdp) != 0) ? $aMatchTcpUdp[2] : $aMatchGreIpsecL2tp[2];
        if (
            $szHostname[0] === '[' && $szHostname[strlen($szHostname)-1] === ']' &&
            MiscNet::Ipv6StringToBinary(substr($szHostname, 1, strlen($szHostname)-2)) !== false
        ) {
            $szHostname = substr($szHostname, 1, strlen($szHostname)-2);
        }
        $wPort = (count($aMatchTcpUdp) != 0) ? intval($aMatchTcpUdp[3]) : null;
        $bProtocol = (count($aMatchTcpUdp) != 0) ?
            (($aMatchTcpUdp[1] === "tcp") ? 6 : 17) :
            (($aMatchGreIpsecL2tp[1] === "gre") ? 47 : (($aMatchGreIpsecL2tp[1] === "ipsec") ? 50 : 115));

        return new VpnEndpointDescriptor($bProtocol, $szHostname, $wPort);
    }
}

?>