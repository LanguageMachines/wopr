<?php

$service_port = "1984";
if ( isset( $_POST['p'] ) ) {
   $service_port = $_POST['p'];
}

$address = gethostbyname('localhost');
if ( isset( $_POST['h'] ) ) {
   $address = $_POST['h'];
}

$s = "";
if ( isset( $_POST['s'] ) ) {
   $s = $_POST['s'];
}
$s = $s . "\n";

$socket = fsockopen($address, $service_port, $errno, $errstr, 30);
$res    = "<response>";

if ( $socket ) {
  fwrite( $socket, $s );

  stream_set_timeout($socket, 5);

  while ( ! feof($socket) ) { 
    $res = $res . fgets( $socket );
  }
  $res = $res . "</response>";

  header("Content-Type: application/xml; charset=UTF-8"); 
  echo $res;

  $info = stream_get_meta_data($socket);
  if ($info['timed_out']) {
    echo "<response><data>error:timeout</data></response>";
  }
  fclose( $socket );
 } else {
  echo "<response><data>error:error</data></response>";
 }
?> 
