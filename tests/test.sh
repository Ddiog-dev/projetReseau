#!/bin/bash

echo " -- Connexion test --"
./LINGI1341-linksim/link_sim -p 64430 -P 64432 -e 1 -c 1 -l 2 &
LINKSIM_PID=$!
./receiver :: 64430 -f received.dat &
./sender ::1 64432 -f sent.dat &
echo "-- Run clearing script when transfer finished --"
