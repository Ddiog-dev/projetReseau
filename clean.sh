$#!/bin/bash
echo "Cleaning"
LINKSIM_PID=(ps -ef|awk '$8=="link_sim" {print $2}')
kill $LINKSIM_PID
