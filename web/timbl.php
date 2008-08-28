<?php
error_reporting(E_ERROR);

$service_port = "1983";
if ( isset( $_POST['p'] ) ) {
   $service_port = $_POST['p'];
}

$address = gethostbyname('ls0168.uvt.nl');
if ( isset( $_POST['h'] ) ) {
   $address = $_POST['h'];
}

$s = "interest continues to rise in the east";
if ( isset( $_POST['s'] ) ) {
   $s = $_POST['s'];
}

$socket = fsockopen($address, $service_port, $errno, $errstr, 30);

if ( $socket ) {
  fwrite( $socket, $s );

  stream_set_timeout($socket, 5);

  if ( ! feof($socket) ) { // if was while.
    echo fgets( $socket );
  }
  $info = stream_get_meta_data($socket);
  if ($info['timed_out']) {
    echo "{'status':'error'}";
  }
  fclose( $socket );
 } else {
  echo "{'status':'error'}";
 }
?> 
