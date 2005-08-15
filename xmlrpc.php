<?php
// $Id: xmlrpc.php,v 1.9.2.1 2005/08/15 00:01:53 dries Exp $

/**
 * @file
 * PHP page for handling incoming XML-RPC requests from clients.
 */

include_once 'includes/bootstrap.inc';
include_once 'includes/common.inc';
include_once 'includes/xmlrpc.inc';
include_once 'includes/xmlrpcs.inc';

xmlrpc_server(module_invoke_all('xmlrpc'));
?>
