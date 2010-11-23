<?php
error_reporting(E_ERROR);

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

$host = get_post_value('host');
$port = get_post_value('port');

$txt = get_post_value('txt');

// Get rid of the linefeeds. Should I replace them with a token,
// and have Wopr divide into seperate sentences?
//
$txt = trim( preg_replace( '/\s+/', ' ', $txt ) );

$cmd = get_post_value('cmd');

$socket = fsockopen($host, $port, $errno, $errstr, 30);

header('Content-Type: application/xml; charset=utf-8');  
//echo "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\" ?_>";

if ( $socket ) {
  fwrite( $socket, $cmd."\n" );
  //delay needed? protocol?
  fwrite( $socket, $txt."\n" );

  stream_set_timeout($socket, 15);

  if ( ! feof($socket) ) { // if was while.
    echo fgets( $socket );
  }
  fwrite( $socket, "ok\n" );
  $info = stream_get_meta_data($socket);
  if ($info['timed_out']) {
    echo "<error><msg>Timeout</msg></error>";
  }
  fclose( $socket );
} else {
  echo "<error><msg>Server not running?</msg></error>";
}

?> 