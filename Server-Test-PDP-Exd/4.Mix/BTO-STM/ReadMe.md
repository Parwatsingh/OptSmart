# 1. Description
Implementation of Concurrent Miner and Validator Algorithm using BTO-STM,
Lock-Free Graph library, Decentralized, and Fork-Join Validator Approaches.
Block graph optimization is done to efficiently store block graph in a block,
where AUs which do not have dependencies are stored in Concurrent Bin,
while dependent AUs information is stored in the block graph.
We combined the advantage of both bin based approach and the STM graph-based
approach to make the proposed approach furthermore efficient. 
Moreover, there are two approaches to construct the concurrent bin.
In the first approach, miner thread concurrently constructs the block graph and
concurrent bin while executing the AUs. While in the second approach, once all
AUs to be kept into the block are executed, and the block graph is ready,
block graph analysis is performed, and AUs nodes in the graph which do not have
in-degree and out-degree is kept into the concurrent bin. Additional time is
taken to construct concurrent bin added in miner time.

# 2. Version
<< Optimized-Version 1.4  >>

# 3. Last Modified
<< 28-03-2020 >>

# 4. Team Members
Parwat Singh Anjana, Sweta Kumari, Sathya Peri, Archit Somani.
	
# 5. Content
1.  < inp-output/inp-params.txt >  ::::  Make sure that this file is in the <inp-output> directory.
2.  < default-main-approach1.cpp > ::::  Source file for concurrent miner and decentralised concurrent validator for approach 1.
3.  < default-main-approach2.cpp > ::::  Source file for concurrent miner and decentralised concurrent validator for approach 2.
4.  < Util/FILEOPR.h >             ::::  Header file defines File related input/output function.
5.  < Util/Timer.h >               ::::  Header file defines time related functions.
6.  < Graph/Lockfree/Graph.h >     ::::  Header file defines <LOCK-FREE> Graph Construction functions using <CAS>.
7.  < Contract/Mix.h >             ::::  Header file defines <Smart Contract> functions for Miner <Using BTO> & Validator <Without BTO>.
8.  < Contract/stmBTO-lib >        ::::  Directory Consists of <STM-BTO> library functions.
9.  < README.md >                  ::::  This File.
10. < run.py >                     ::::  Python script to run the application on different workloads.
11. < make >                       ::::  Compile the program to generate the executable.

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
1. The terminal output gives time in microseconds, Block Graph information, and acceptance of the block by concurrent miner and validator.
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
