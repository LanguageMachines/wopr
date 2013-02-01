<?php
error_reporting(E_ERROR);

$wopr_port = "1981";
$wopr_host = "erebus";
if ( substr( $_SERVER["DOCUMENT_ROOT"], 0, 18 ) === "/Applications/MAMP" ) {
  $wopr_port = "1984";
  $wopr_host = "localhost";
}

if ( isset( $_POST['p'] ) ) {
   $wopr_port = $_POST['p'];
}

$address = gethostbyname($wopr_host);
if ( isset( $_POST['h'] ) ) {
   $address = $_POST['h'];
}

$cmd = "CTX";
if ( isset( $_POST['c'] ) ) {
   $cmd = $_POST['c'];
}

$socket = fsockopen($address, $wopr_port, $errno, $errstr, 30);

if ( $socket ) {
  fwrite( $socket, $cmd."\n" );

  stream_set_timeout($socket, 5);

  if ( ! feof($socket) ) { // if was while.
    echo fgets( $socket );
  }
  $info = stream_get_meta_data($socket);
  if ($info['timed_out']) {
    echo "<result><error>time out</error></result>";
  }
  fclose( $socket );
 } else {
  echo "<result><error>no socket</error></result>";
}
?> 
