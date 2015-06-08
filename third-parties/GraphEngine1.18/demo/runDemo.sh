#Demo to run graph inference
#This example will run graph inference on a regular graph of 20M vertices with degree=12.
#Befor executing, compile your code and verify your paths.
#@Fei,Tere,Krishna, Hideaki
#!/bin/bash

echo "This is demo run of the graph inference"
echo "The inference will run on 10 threads one socket"
echo "To change configuration, please read our REAME file"
echo "....."
../code/gibbsDNSApp ../data/configure_g20.txt ../data/graph_D12V20ME120M.alchemy.factors -refresh=100

