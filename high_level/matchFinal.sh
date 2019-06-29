#!/bin/sh
java -Xmx1G -Xms1G -cp target/chariot-hl.jar senpai.MatchFinal $@ | tee last_all.txt
java -Xmx1G -Xms1G -cp target/chariot-hl.jar senpai.Smoke 20000
