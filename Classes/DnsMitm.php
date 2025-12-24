<?php

namespace VVTS\Classes;

require_once(dirname(__FILE__) . "/../autoload.php");

use \VVTS\Classes\MainLoop;
use \VVTS\Classes\MiscNet;
use \VVTS\Classes\Subscribable;
use \VVTS\Interfaces\IUnblockable;
use \VVTS\Types\DnsTransactionMapEntry;

class DnsMitm extends Subscribable implements IUnblockable {
    var $hListeningSocket;
    var $hClientSocket;
    var $szNextDns;
    var $aTransactionMap;
    var $aOverridesA;
    var $aOverridesAAAA;
    var $dwBindAddr;
    var $abBindIpv6Addr;
    var $bNat64Enabled;

    function __construct() {
        parent::__construct();
        $this->aTransactionMap = [];
        $this->aOverridesA = [];
        $this->aOverridesAAAA = [];

        MainLoop::GetInstance()->RegisterObject($this);
    }

    static function ParseDomainName(&$abRequest, &$dwOffset) {
        $szName = "";
        $bSetOffset = true;
        $dwCurOffset = $dwOffset;

        for(;;) {
            $bSegmentLength = ord($abRequest[$dwCurOffset]); $dwCurOffset++;
            if ($bSegmentLength == 0) {
                break;
            }
            if (($bSegmentLength & 0xc0) == 0xc0) {
                if ($bSetOffset) {
                    $dwOffset = $dwCurOffset + 1;
                    $bSetOffset = false;
                }
                $dwCurOffset = (($bSegmentLength & 0x3f) << 8) | ord($abRequest[$dwCurOffset]);
                continue;
            }
            $szName .= (strlen($szName) == 0) ? "" : ".";
            for($j = 0; $j < $bSegmentLength; $j++, $dwCurOffset++) {
                $szName .= $abRequest[$dwCurOffset];
            }
        }

        if ($bSetOffset) {
            $dwOffset = $dwCurOffset;
        }
        return $szName;
    }

    function ApplyOverrides(&$abRequest, $szPeer) {
        $wQdCount = (ord($abRequest[4]) << 8) | ord($abRequest[5]);
        $dwOffset = 12;
        $wFlags = (ord($abRequest[2]) << 8) | ord($abRequest[3]);
        $wOffsetCorrect = 0;
        $wAdCount = (ord($abRequest[6]) << 8) | ord($abRequest[7]);

        for ($i = 0; $i < $wQdCount; $i++) {
            $szName = self::ParseDomainName($abRequest, $dwOffset);
            $wType = (ord($abRequest[$dwOffset]) << 8) | ord($abRequest[$dwOffset+1]);
            $dwOffset += 2;
            $wClass = (ord($abRequest[$dwOffset]) << 8) | ord($abRequest[$dwOffset+1]);
            $dwOffset += 2;

            // printf ("[i] received DNS %s from %s for %s type=%s class=%s\n", ($wFlags & 0x8000) ? "response" : "request", $szPeer, $szName, ($wType == 1) ? "A" : "unknown", ($wClass == 1) ? "IN" : "unknown");
        }

        if (!($wFlags & 0x8000)) {
            return;
        }

        for ($i = 0; $i < $wAdCount; $i++) {
            $szName = self::ParseDomainName($abRequest, $dwOffset);
            $wType = (ord($abRequest[$dwOffset]) << 8) | ord($abRequest[$dwOffset+1]);
            $dwOffset += 2;
            $wClass = (ord($abRequest[$dwOffset]) << 8) | ord($abRequest[$dwOffset+1]);
            $dwOffset += 2;
            /* TTL */
            $dwOffset += 4;
            $wDataLength = (ord($abRequest[$dwOffset]) << 8) | ord($abRequest[$dwOffset+1]);
            $dwOffset += 2;
            $dwOffset += $wDataLength;

            if ((
                isset($this->aOverridesA[strtolower($szName)]) || 
                isset($this->aOverridesAAAA[strtolower($szName)])
            ) && $wType == 5) {
                /* CNAME record -> add result to override list */
                $dwTmpOffset = $dwOffset - $wDataLength;
                $szCname = self::ParseDomainName($abRequest, $dwTmpOffset);
                if (isset($this->aOverridesA[strtolower($szName)])) {
                    $this->aOverridesA[strtolower($szCname)] = $this->aOverridesA[strtolower($szName)];
                }
                if (isset($this->aOverridesAAAA[strtolower($szName)])) {
                    $this->aOverridesAAAA[strtolower($szCname)] = $this->aOverridesAAAA[strtolower($szName)];
                }
            }

            if (isset($this->aOverridesA[strtolower($szName)]) && $wType == 1 && $wDataLength == 4) {
                $szOverride = $this->aOverridesA[strtolower($szName)];
                $dwOriginal = (ord($abRequest[$dwOffset - 4]) << 24) | (ord($abRequest[$dwOffset - 3]) << 16) | (ord($abRequest[$dwOffset - 2]) << 8) | ord($abRequest[$dwOffset - 1]);
                $szOriginal = MiscNet::DwordToIpv4String($dwOriginal);
                printf("[i] applying DNS response override for %s: %s -> %s\n", $szName, $szOriginal, $szOverride);
                $dwOverride = MiscNet::Ipv4StringToDword($szOverride);
                $abOverride = chr($dwOverride >> 24) . chr($dwOverride >> 16) . chr($dwOverride >> 8) . chr($dwOverride);
                $abRequest = substr($abRequest, 0, $dwOffset - 4) . $abOverride . substr($abRequest, $dwOffset);
            }
            if ((
                isset($this->aOverridesAAAA[strtolower($szName)]) ||
                ($this->bNat64Enabled && isset($this->aOverridesA[strtolower($szName)]))
            ) && $wType == 28 && $wDataLength == 16) {
                $szOverride = isset($this->aOverridesAAAA[strtolower($szName)]) ?
                    $this->aOverridesAAAA[strtolower($szName)] :
                    MiscNet::BinaryToIPv6String(MiscNet::Ipv4ToNat64(MiscNet::Ipv4StringToDword($this->aOverridesA[strtolower($szName)])));
                $abOriginal = substr($abRequest, $dwOffset - 16, 16);
                $szOriginal = MiscNet::BinaryToIpv6String($abOriginal);
                printf("[i] applying DNS response override for %s: %s -> %s\n", $szName, $szOriginal, $szOverride);
                $abOverride = MiscNet::Ipv6StringToBinary($szOverride);
                $abRequest = substr($abRequest, 0, $dwOffset - 16) . $abOverride . substr($abRequest, $dwOffset);
            }
        }
    }

    function SetOverride($szDomainName, $szAddress) {
        if (MiscNet::Ipv4StringToDword($szAddress) !== false) {
            $this->aOverridesA[strtolower($szDomainName)] = $szAddress;
        } else if (MiscNet::Ipv6StringToBinary($szAddress) !== false) {
            $this->aOverridesAAAA[strtolower($szDomainName)] = $szAddress;
        }
    }

    function Sockets() {
        $ahSockets = [];
        if ($this->hListeningSocket != null) {
            array_push($ahSockets, $this->hListeningSocket);
        }
        if ($this->hListeningIpv6Socket != null) {
            array_push($ahSockets, $this->hListeningIpv6Socket);
        }
        if ($this->hClientSocket != null) {
            array_push($ahSockets, $this->hClientSocket);
        }
        return $ahSockets;
    }

    function BringUp() {
        if (!isset($this->szNextDns) || $this->szNextDns == null) {
            $szResolvConf = file_get_contents("/etc/resolv.conf");
            if (
                preg_match('/^nameserver\s+([0-9\.]+|[0-9a-f:]+)\s*$/im', $szResolvConf, $aMatch) && (
                    MiscNet::Ipv4StringToDword($aMatch[1]) !== false ||
                    MiscNet::Ipv6StringToBinary($aMatch[1]) !== false
                )
            ) {
                $this->szNextDns = $aMatch[1];
            } else {
                $this->Signal("dns_err", "Don't know where to forward DNS packets to");
                $this->Teardown();
                return;
            }
        }

        $dwBindAddr = ($this->dwBindAddr === null) ? 0 : $this->dwBindAddr;
        $abBindIpv6Addr = ($this->abBindIpv6Addr === null) ? str_repeat("\x00", 16) : $this->abBindIpv6Addr;
        $this->hListeningSocket = stream_socket_server("udp://" . MiscNet::DwordToIpv4String($dwBindAddr) . ":53", $dwErrno, $szErrstr, STREAM_SERVER_BIND);
        $this->hListeningIpv6Socket = stream_socket_server("udp://[" . MiscNet::BinaryToIpv6String($abBindIpv6Addr) . "]:53", $dwErrno, $szErrstr, STREAM_SERVER_BIND);
        if (MiscNet::Ipv4StringToDword($this->szNextDns) !== false) {
            $this->hClientSocket = stream_socket_client("udp://" . $this->szNextDns . ":53", $dwErrno, $szErrstr);
        } else {
            $this->hClientSocket = stream_socket_client("udp://[" . $this->szNextDns . "]:53", $dwErrno, $szErrstr);
        }

        if (
            ($this->dwBindAddr !== null && $this->hListeningSocket === false) ||
            ($this->abBindIpv6Addr !== null && $this->hListeningIpv6Socket === false) ||
            $this->hClientSocket === false
        ) {
            $this->Signal("dns_err", "Could not create DNS sockets");
            $this->Teardown();
            return;
        } else {
            $this->Signal("dns_up");
        }
    }

    function Onunblock($hSocket) {
        foreach ($this->aTransactionMap as $dwTrid => $oEntry) {
            if ($oEntry->dwExpire < intval(microtime(1) * 1000)) {
                unset($this->aTransactionMap[$dwTrid]);
            }
        }

        $abBuf = stream_socket_recvfrom($hSocket, 65536, 0, $szPeer);
        if ($abBuf === "" || $abBuf === false) {
            $this->Signal("dns_err", "DNS socket closed");
            $this->Teardown();
            return;
        }
        $wTransactionId = (ord($abBuf[0]) << 8) | ord($abBuf[1]);

        if ($hSocket === $this->hListeningSocket || $hSocket === $this->hListeningIpv6Socket) {
            // printf("[i] incoming dns request\n");
            $this->ApplyOverrides($abBuf, $szPeer);
            $this->aTransactionMap[$wTransactionId] = new DnsTransactionMapEntry($hSocket, $szPeer, intval(microtime(1) * 1000) + 10000);
            stream_socket_sendto($this->hClientSocket, $abBuf, 0, $this->szNextDns . ":53");
        } else {
            // printf("[i] incoming dns response\n");
            $this->ApplyOverrides($abBuf, $szPeer);
            if (!isset($this->aTransactionMap[$wTransactionId])) {
                printf("[-] incoming DNS response with unknown source\n");
            } else {
                stream_socket_sendto($this->aTransactionMap[$wTransactionId]->hSocket, $abBuf, 0, $this->aTransactionMap[$wTransactionId]->szPeer);
                unset($this->aTransactionMap[$wTransactionId]);
            }
        }
    }

    function Teardown() {
        if ($this->hListeningSocket != null) {
            fclose($this->hListeningSocket);
            $this->hListeningSocket = null;
        }
        if ($this->hListeningIpv6Socket != null) {
            fclose($this->hListeningIpv6Socket);
            $this->hListeningIpv6Socket = null;
        }
        if ($this->hClientSocket != null) {
            fclose($this->hClientSocket);
            $this->hClientSocket = null;
        }
        $this->CancelAllSubscriptions();
    }
}



?>