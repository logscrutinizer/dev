
#!/bin/bash
set -e

cp build_cc/*.rpm ls.rpm
rpm  -qlp ls.rpm

# Any subsequent(*) commands which fail will cause the shell script to exit immediately
rm -rf ftp
mkdir ftp
curlftpfs ftp.servage.net ftp -o ssl,no_verify_peer,no_verify_hostname,user=klang:gin4allgin4all
cp build_cc/*LogScrutinizer*.rpm ftp/logscrutinizer.com/downloads/.
fusermount -u ftp
