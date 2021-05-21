clear
make
# ./atpg -genFailLog ../patterns/c17.ptn ../sample_circuits/c17.ckt -fault 16GAT g4 GO SA0 
# ./atpg -genFailLog ../patterns/c17.ptn ../sample_circuits/c17.ckt -fault 23GAT g5 GO SA0 #> ../failLog/c17-001.failLog
./atpg -genFailLog ../patterns/c17.ptn ../sample_circuits/c17.ckt -fault 16GAT g4 GO SA0 -fault 2GAT g4 GI SA0 > ../FailLog/c17-001.failLog
