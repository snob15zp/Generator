mode com7: baud=115200 parity=n data=8 stop=1 dtr=off rts=off
echo ^2>com7
copy test_10.txt com7
