<?php
include("db.php");

function get_post_value($name) {
  if ( $_POST[$name] ) {
    return $_POST[$name];
  }
  return "";
}
function get_get_value($name) {
  if ( $_GET[$name] ) {
    return $_GET[$name];
  }
  return "";
}

$txt = get_post_value("txt");
$ip  = get_post_value("ip");

add_txt( NULL, $txt, $ip );

echo "<result>ok</result>";
?>