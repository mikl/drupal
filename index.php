<?php
// $Id: index.php,v 1.54.2.1 2002/12/26 12:14:53 dries Exp $

include_once "includes/common.inc";

page_header();

check_php_setting("magic_quotes_gpc", 0);

if (module_hook(variable_get("site_frontpage", "node"), "page")) {
  module_invoke(variable_get("site_frontpage", "node"), "page");
}
else {
  $theme->header();
  $theme->footer();
}

page_footer();

?>
