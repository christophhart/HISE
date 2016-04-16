<?php 
header('Content-Type: text/plain');
header('Content-Transfer-Encoding: UTF-8');
header('Expires: 0');

$CONFIG = include("licence_information.php");

if ($ALLOWED_IP != "" && $_SERVER['REMOTE_ADDR'] != $ALLOWED_IP)
{
    die("Illegal Request");
};

// The user information will be submitted by POST data.

$PRODUCT_ID = $CONFIG['product'];
$PRIVATE_KEY_PART_1 = $CONFIG['private_key_part_1'];
$PRIVATE_KEY_PART_2 = $CONFIG['private_key_part_2'];


$EMAIL = $_GET['email'];
$USER = $_GET['user'];
$MACHINE = $_GET['machine_id'];

$DATE = date("j M Y g:i:sa");
$TIME = dec2hex(round(microtime(true)*1000));

/** Create the key file */
        
/** Helper Functions */
function dec2hex($number)
{
    $hexvalues = array('0','1','2','3','4','5','6','7',
               '8','9','a','b','c','d','e','f');
    $hexval = '';
     while($number != '0')
     {
        $hexval = $hexvalues[bcmod($number,'16')].$hexval;
        $number = bcdiv($number,'16',0);
    }
    return $hexval;
}

include ('Math/BigInteger.php');  // get this from: phpseclib.sourceforge.net
function applyToValue ($message, $key_part1, $key_part2)
{
    $result = new Math_BigInteger();
    $zero  = new Math_BigInteger();
    $value = new Math_BigInteger (strrev ($message), 256);
    $part1 = new Math_BigInteger ($key_part1, 16);
    $part2 = new Math_BigInteger ($key_part2, 16);
    while (! $value->equals ($zero))
    {
        $result = $result->multiply ($part2);
        list ($value, $remainder) = $value->divide ($part2);
        $result = $result->add ($remainder->modPow ($part1, $part2));
    }
    return $result->toHex();
}

$KEY_FILE_TEXT = "";

// Create the comment section
$KEY_FILE_TEXT .= "Keyfile for " . $PRODUCT_ID . "\n";
$KEY_FILE_TEXT .= "User: ". $USER ."\n";
$KEY_FILE_TEXT .= "Email: ". $EMAIL ."\n";
$KEY_FILE_TEXT .= "Machine numbers: " . $MACHINE . "\n";
$KEY_FILE_TEXT .= "Created: ". $DATE  ."\n";
$KEY_FILE_TEXT .= "\n";
// Create the XML 
$dom = new DOMDocument("1.0", "utf-8");
$root = $dom->createElement("key");
$dom->appendChild($root);
$root->setAttribute("user", $USER);
$root->setAttribute("email", $EMAIL);
$root->setAttribute("mach", $MACHINE);
$root->setAttribute("app", $PRODUCT_ID);
$root->setAttribute("date", $TIME);
$XML_STRING = $dom->saveXML();
$ENCRYPTED_XML = "#" . applyToValue($XML_STRING, $PRIVATE_KEY_PART_1, $PRIVATE_KEY_PART_2);
$XML_DATA = chunk_split($ENCRYPTED_XML, 70);

$KEY_FILE_TEXT .= $XML_DATA;

echo $KEY_FILE_TEXT;
?>

