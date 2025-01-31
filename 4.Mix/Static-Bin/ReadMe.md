# 1. Description
Implementation of Concurrent Miner and Concurrent Validator Algorithm using
Static Analysis based technique which group the AUs in the concurrent and
sequential bins. Later, a decentralized validator deterministically executes
the concurrent and sequential bin AUs and validate the final state.

# 2. Version
<< Optimized-Version 2 >>
	
# 3. Last Modified
<< 04-01-2020 >>

# 4. Team Members
Parwat Singh Anjana, Sweta Kumari, Sathya Peri, Archit Somani.
	
# 5. Content
1. < inp-output/inp-params.txt >  ::::  Make sure that this file is in the <inp-output> directory.
2. < staticBin.cpp >              ::::  Source file for concurrent Miner and decentralised Validator Algorithm.
3. < Util/FILEOPR.h >             ::::  Header file defines File related input/output function.
4. < Util/Timer.h >               ::::  Header file defines time related functions.
5. < Contract >                   ::::  Defines <Smart Contract> functions.
6. < README.md >                  ::::  This File.
7. < make >                       ::::  Make file to compile the src.
8. < run.py >                     ::::  Python script for multiple execution of the application.

# 6. Compile on Linux(X86_64) and Run
$ make
$ python run.py

# 7. Input
	<inp-output/inp-params.txt>
	<content should be four lines with space separated values as follows:>
	50 400
	10
	3 20
	20 3
		Where:
		1. l = 50 : # of threads
		2. m = 400: # of AUs or Transactions
		3. n = 10 : # of Coin:Account Shared Objects
		4. o = 3  : # of Ballot:Proposal Shared Objects
		5. p = 20 : # of Ballot:Voter Shared Objects
		6. q = 20 : # of Auction:Bidder Shared Objects
		7. r = 3  : Auction: Bid End Time or duration (milliseconds)

# 8. Output
1. The terminal output gives time in microseconds and acceptance of the block by concurrent miner and validator.
2. Time taken by an individual thread and other relevant information is stored in the inp-output/Time.csv file.


# 9. System
Architecture:          x86_64
CPU op-mode(s):        32-bit, 64-bit
Byte Order:            Little Endian
CPU(s):                56
On-line CPU(s) list:   0-55
Thread(s) per core:    2
Core(s) per socket:    14
Socket(s):             2
NUMA node(s):          2
Vendor ID:             GenuineIntel
CPU family:            6
Model:                 79
Model name:            Intel(R) Xeon(R) CPU E5-2690 v4 @ 2.60GHz
Stepping:              1
CPU MHz:               1254.906
CPU max MHz:           3500.0000
CPU min MHz:           1200.0000
BogoMIPS:              5189.36
Virtualization:        VT-x
L1d cache:             32K
L1i cache:             32K
L2 cache:              256K
L3 cache:              35840K
NUMA node0 CPU(s):     0-13,28-41
NUMA node1 CPU(s):     14-27,42-55
