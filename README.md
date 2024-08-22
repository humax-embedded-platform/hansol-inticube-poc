# hansol-inticube
Linux C programming - A massive HTTP traffic generator

1. ./release_deb.sh
2. copy hansol_inticube_poc_packet.deb to run machine
3. sudo apt install ./hansol_inticube_poc_packet.deb
4. /usr/bin/httppostclient --host ./hostdb.txt --input ./msg.txt --request 100 --log ./