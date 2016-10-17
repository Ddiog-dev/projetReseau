#!/bin/bash
./LINGI1341-linksim/link_sim -p 64430 -P 64432 &
./receiver :: 64430 -f received.dat
./sender ::1 64432 -f sent.dat
