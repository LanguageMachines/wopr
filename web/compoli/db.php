<?php
$DBNAME="sqlite:submitted.sqll";

function get_db() {
  $DBNAME="sqlite:submitted.sqll";
  $db = null;

  try {
    //create or open the database
    $db = new PDO($DBNAME);
  } catch(Exception $e) {
    die("error");
  }
  $db->setAttribute(PDO::ATTR_TIMEOUT, 10);
  $db->setAttribute(PDO::ATTR_ERRMODE, PDO::ERRMODE_WARNING);
  return $db;
}

// --

function add_txt( $db, $txt, $ip ) {
  if ( $db == NULL ) {
    $db = get_db();
  }
  $dt = gmstrftime( "%Y-%m-%d %H:%M:%S", time() );
  $data = array( 'txt' => $txt, 'dt' => $dt, 'ip' => $ip );
  $stmt = $db->prepare("INSERT INTO texts (txt, ip, dt) VALUES (:txt, :ip, :dt);");
  $stmt->execute( $data );
  if ( $stmt === false ) {
    print "error\n";
  } else {
    print "ok\n";
  }

}
?>
