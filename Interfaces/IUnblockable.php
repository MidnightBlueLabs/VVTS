<?php

namespace VVTS\Interfaces;

require_once(dirname(__FILE__) . "/../autoload.php");

interface IUnblockable {
    function Sockets();
    function Onunblock($hSocket);
    function Teardown();
}

?>