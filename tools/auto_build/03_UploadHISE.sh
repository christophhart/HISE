
echo "Uploading to FTP..."

echo "Enter version with underscores"
read version


cd "$(dirname "$0")"

filename="HISE_"$version".pkg"

echo $filename



hostname="hartinstruments.net"

cd Output

echo "Enter FTP User Name:"
read user_name

ftp $user_name@$hostname <<EOF

cd "html/hise/download/"
put $filename
bye

if [ $? != "0"];
then
	echo "========================================================================"
	echo "Error at FTP Upload. Aborting..."
    exit
fi

echo "OK"

echo "================================="
echo "Build and upload sucessful"