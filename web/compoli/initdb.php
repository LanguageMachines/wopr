<?php
$DBNAME="sqlite:submitted.sqll";

try {
  //create or open the database
  $db = new PDO($DBNAME);
} catch(Exception $e) {
  die("error");
}
$db->setAttribute(PDO::ATTR_TIMEOUT, 10);
$db->setAttribute(PDO::ATTR_ERRMODE, PDO::ERRMODE_WARNING);

$db->beginTransaction();
$res = $db->exec('PRAGMA encoding = "UTF-8";');   
$res = $db->exec("CREATE TABLE texts (
  id  INTEGER PRIMARY KEY,
  txt TEXT,
  ip VARCHAR(32),
  dt VARCHAR(32)
)"
);
$db->commit();
echo "<h2>ok</h2>";
?>
