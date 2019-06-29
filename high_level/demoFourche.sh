#!/bin/sh
java -Xmx1G -Xms1G -cp target/chariot-hl.jar senpai.DemoFourche $@ | tee last_all.txt
