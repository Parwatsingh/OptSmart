#include <iostream>
#include <thread>
#include <list>
#include <atomic>
#include "Util/Timer.cpp"
#include "Contract/SimpleAuction.cpp"
#include "Util/FILEOPR.cpp"

#define maxThreads 128
#define maxBObj 5000
#define maxbEndT 5000 //millseconds
#define funInContract 6
#define pl "===================================================\n"
#define MValidation true   //! true or false
#define malMiner true      //! set the flag to make miner malicious.
#define NumOfDoubleSTx 2   //! # double-spending Tx for malicious final state by Miner, multiple of 2.

using namespace std;
using namespace std::chrono;

int NumBlock     = 26;     //! at least two blocks, the first run is warmup run.
int numValidator = 50;
int beneficiary  = 0;      //! fixed beneficiary id to 0, it can be any unique address/id.
int    nBidder   = 2;      //! nBidder: number of bidder shared objects.
int    nThread   = 1;      //! nThread: total number of concurrent threads; default is 1.
int    numAUs;             //! numAUs: total number of Atomic Unites to be executed.
double lemda;              //! λ: random delay seed.
int    bidEndT;            //! time duration for auction.
double tTime[2];           //! total time taken by miner and validator algorithm respectively.
SimpleAuction *auction;    //! smart contract for miner.
SimpleAuction *auctionV;   //! smart contract for Validator.
int    *aCount;            //! aborted transaction count.
float_t*mTTime;            //! time taken by each miner Thread to execute AUs (Transactions).
float_t*vTTime;            //! time taken by each validator Thread to execute AUs (Transactions).
vector<string>listAUs;     //! holds AUs to be executed on smart contract: "listAUs" index+1 represents AU_ID.
std::atomic<int>currAU;    //! used by miner-thread to get index of Atomic Unit to execute.
std::atomic<int>eAUCount;  //! used by validator threads to keep track of how many valid AUs executed by validator threads.


//State Data
int mHBidder;
int mHBid;
int vHBidder;
int vHBid;
int *mPendingRet;
int *vPendingRet;


/*************************MINER CODE BEGINS***********************/
/*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
!    Class "Miner" CREATE & RUN "1" miner-THREAD                  !
!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
class Miner
{
	public:
	Miner( )
	{
		//! initialize the counter used to execute the numAUs to
		//! 0, and graph node counter to 0 (number of AUs added
		//! in graph, invalid AUs will not be part of the grpah).
		currAU = 0;
		//! index location represents respective thread id.
		mTTime = new float_t [nThread];
		aCount = new int [nThread];
		for(int i = 0; i < nThread; i++) {
			mTTime[i] = 0;
			aCount[i] = 0;
		}
		auction = new SimpleAuction(bidEndT, beneficiary, nBidder);
	}

	//!----------------------------- 
	//!!!!!!!! MAIN MINER !!!!!!!!!!
	//!-----------------------------
	void mainMiner()
	{
		Timer mTimer;
		thread T[nThread];

		//!---------------------------------------------------
		//!!!!!!!!!!    CREATE 1 MINER THREADS      !!!!!!!!!!
		//!---------------------------------------------------
		double start = mTimer.timeReq();
		for(int i = 0; i < nThread; i++) T[i] = thread(concMiner, i, numAUs);
		for(auto& th : T) th.join();
		tTime[0] = mTimer.timeReq() - start;

		//! print the final state of the shared objects.
		finalState();
//		 auction->AuctionEnded( );
	}


	//!--------------------------------------------------
	//! The function to be executed by a miner threads. !
	//!--------------------------------------------------
	static void concMiner( int t_ID, int numAUs)
	{
		Timer thTimer;
		//! get the current index, and increment it.
		int curInd = currAU++;
		//! statrt clock to get time taken by this.AU
		auto start = thTimer._timeStart();
		while( curInd < numAUs ) {
			//!get the AU to execute, which is of string type.
			istringstream ss(listAUs[curInd]);
			string tmp;
			ss >> tmp;
			int AU_ID = stoi(tmp);
			ss >> tmp;
			if(tmp.compare("bid") == 0) {
				ss >> tmp;
				int payable = stoi(tmp);//! payable
				ss >> tmp;
				int bID = stoi(tmp);//! Bidder ID
				ss >> tmp;
				int bAmt = stoi(tmp);//! Bidder value
				bool v = auction->bid(payable, bID, bAmt);
				if(v != true) aCount[0]++;
			}
			if(tmp.compare("withdraw") == 0) {
				ss >> tmp;
				int bID = stoi(tmp);//! Bidder ID
				bool v = auction->withdraw(bID);
				if(v != true) aCount[0]++;
			}
			if(tmp.compare("auction_end") == 0) {
				bool v = auction->auction_end( );
				if(v != true) aCount[0]++;
			}
			//! get the current index to execute, and increment it.
			curInd = currAU++;
		}
		mTTime[t_ID] += thTimer._timeStop(start);
	}

	//!------------------------------------------
	//!Final state of all the shared object.    |
	//! Once all AUs executed.                  |
	//!------------------------------------------
	void finalState() {
		for(int id = 1; id <= nBidder; id++) {
			auction->state(&mHBidder, &mHBid, mPendingRet);
		}
	}

	~Miner() { };
};
/*********************MINER CODE ENDS*************************/




/*************************VALIDATOR CODE BEGINS****************/
/*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
! Class "Validator" CREATE & RUN "1" validator-THREAD          !
!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
class Validator
{
	public:
	Validator(int chairperson)
	{
		//! initialize the counter used to execute the numAUs to
		//! 0, and graph node counter to 0 (number of AUs added
		//! in graph, invalid AUs will not be part of the grpah).
		currAU = 0;
		//! index location represents respective thread id.
		vTTime = new float_t [nThread];
		aCount = new int [nThread];
		for(int i = 0; i < nThread; i++) {
			vTTime[i] = 0;
			aCount[i] = 0;
		}
		auctionV = new SimpleAuction(bidEndT, beneficiary, nBidder);
	}

	//!-------------------------------------------------------- 
	//!! MAIN Validator:: CREATE ONE VALIDATOR THREADS !!!!!!!!
	//!--------------------------------------------------------
	void mainValidator()
	{
		Timer vTimer;
		thread T[nThread];
		auction->reset(bidEndT);

		//!--------------------------------------
		//!!!!! CREATE ONE VALIDATOR THREAD  !!!!
		//!--------------------------------------
		double start = vTimer.timeReq();
		for(int i = 0; i < nThread; i++) T[i] = thread(concValidator, i);
		for(auto& th : T) th.join( );
		tTime[1] = vTimer.timeReq() - start;

		//!print the final state of the shared objects by validator.
		finalState();
//		auctionV->AuctionEnded( );
	}

	//!------------------------------------------------------
	//! THE FUNCTION TO BE EXECUTED BY A VALIDATOR THREAD.  !
	//!------------------------------------------------------
	static void concValidator( int t_ID )
	{
		Timer thTimer;
		//! get the current index, and increment it.
		int curInd = currAU++;
		//! statrt clock to get time taken by this.AU
		auto start = thTimer._timeStart();
		while( curInd < numAUs ) {
			//!get the AU to execute, which is of string type.
			istringstream ss(listAUs[curInd]);
			string tmp;
			ss >> tmp;
			int AU_ID = stoi(tmp);
			ss >> tmp;
			if(tmp.compare("bid") == 0) {
				ss >> tmp;
				int payable = stoi(tmp);//! payable
				ss >> tmp;
				int bID = stoi(tmp);//! Bidder ID
				ss >> tmp;
				int bAmt = stoi(tmp);//! Bidder value
				bool v = auctionV->bid(payable, bID, bAmt);
				if(v != true) aCount[0]++;
			}
			if(tmp.compare("withdraw") == 0) {
				ss >> tmp;
				int bID = stoi(tmp);//! Bidder ID

				bool v = auctionV->withdraw(bID);
				if(v != true) aCount[0]++;
			}
			if(tmp.compare("auction_end") == 0) {
				bool v = auctionV->auction_end( );
				if(v != true) aCount[0]++;
			}
			//! get the current index to execute, and increment it.
			curInd = currAU++;
		}
		vTTime[t_ID] += thTimer._timeStop(start);
	}


	//!------------------------------------------
	//! Final state of all the shared object.   |
	//! Once all AUs executed.                  |
	//!------------------------------------------
	void finalState() {
		for(int id = 1; id <= nBidder; id++) {
			auction->state(&vHBidder, &vHBid, vPendingRet);
		}
	}
	~Validator() { };
};
/******************VALIDATOR CODE ENDS*****************************/



/*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
/*!!!!!!!!     State Validation    !!!!!!!!!!*/
/*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
bool stateVal() {
	//State Validation
	bool flag = false;
	if(mHBidder != vHBidder || mHBid != vHBid) flag = true;
//	cout<<"\n============================================"
//	    <<"\n     Miner Auction Winer "<<mHBidder
//	    <<" |  Amount "<<mHBid;
//	cout<<"\n Validator Auction Winer "<<to_string(vHBidder)
//	    <<" |  Amount "<<to_string(vHBid);
//	cout<<"\n============================================\n";
	return flag;
}



/**********************MAIN FUN CODE BEGINS************************/
/*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
/*!!!!!!!!          main()         !!!!!!!!!!*/
/*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
int main(int argc, char *argv[]) {
	cout<<pl<<"Serial Miner and Serial Validator\n";
	cout<<"--------------------------------\n";

	if(argc<3) 
		cout<<"\nPlease Enter Command Line Argument as follows:"
			<<"\n\t./a.out <num Blocks> <num Validator> <num Iteration>\n"; 

	NumBlock      = atoi(argv[1]);
	numValidator  = atoi(argv[2]);
	int nItertion = atoi(argv[3]);

	if(NumBlock < 2) cout<<"\nNumber of Blocks should be >= 2\n";
	if(numValidator < 1)cout<<"\nNumber of Validators should be >= 1\n";
	if(nItertion < 1)cout<<"\nNumber of Iterations should be >= 1\n";

	float tMiner  = 0;
	float tVal    = 0;
	int tReject   = 0;
	int tMaxAcc   = 0;

	//! list holds the avg time taken by miner and validator
	//! threads for multiple consecutive runs of the algorithm.
	list<float>mItrT;         
	list<float>vItrT;	
	int totalRun     = NumBlock; //at least 2
	int maxAccepted  = 0;
	int totalRejCont = 0; //number of validator rejected the blocks;


	FILEOPR file_opr;

	//! read from input file:: nBidder = #numProposal; nThread = #threads;
	//! numAUs = #AUs; λ = random delay seed.
	file_opr.getInp(&nBidder, &bidEndT, &nThread, &numAUs, &lemda);

	//! max Proposal shared object error handling.
	if(nBidder > maxBObj) {
		nBidder = maxBObj;
		cout<<"Max number of Proposal Shared Object can be "<<maxBObj<<"\n";
	}

	for(int itr = 0; itr < nItertion; itr++)
	{
		//!------------------------------------------------------------------
		//! Num of threads should be 1 for serial so we are fixing it to 1, !
		//! Whatever be # of threads in inputfile, it will be always one.   !
		//!------------------------------------------------------------------
		nThread = 1;

		for(int nBlock = 0; nBlock < NumBlock; nBlock++)
		{
			 //! generates AUs (i.e. trans to be executed by miner & validator).
			file_opr.genAUs(numAUs, nBidder, funInContract, listAUs);
			tTime[0]    = 0;
			tTime[1]    = 0;
			mPendingRet = new int [nBidder+1];
			vPendingRet = new int [nBidder+1];
			Timer mTimer;
			mTimer.start();

			//MINER
			Miner *miner = new Miner();
			miner ->mainMiner();

			//VALIDATOR
			int acceptCount = 0, rejectCount = 0;
			for(int nval = 0; nval < numValidator; nval++) {

				Validator *validator = new Validator(0);
				validator ->mainValidator();

				//State Validation
				bool flag = stateVal();
				if(flag == true) rejectCount++;
				else acceptCount++;
			}
			if(nBlock > 0) {
				totalRejCont += rejectCount;
				if(maxAccepted < acceptCount ) maxAccepted = acceptCount;
			}

			mTimer.stop();

			//! total valid AUs among total AUs executed 
			//! by miner and varified by Validator.
			int vAUs = numAUs-aCount[0];
			if(nBlock > 0)
			file_opr.writeOpt(nBidder, nThread, numAUs, tTime,
			                  mTTime, vTTime, aCount, vAUs, mItrT, vItrT);

			listAUs.clear();
			delete miner;
		}
	
		// to get total avg miner and validator
		// time after number of totalRun runs.
		float tAvgMinerT = 0;
		float tAvgValidT = 0;
		auto mit = mItrT.begin();
		auto vit = vItrT.begin();
		for(int j = 1; j < NumBlock; j++) {
			tAvgMinerT = tAvgMinerT + *mit;
			tAvgValidT = tAvgValidT + *vit;
			mit++;
			vit++;
		}
		tAvgMinerT = tAvgMinerT/(NumBlock-1);
		tAvgValidT = tAvgValidT/(NumBlock-1);

		tMiner  += tAvgMinerT;
		tVal    += tAvgValidT;
		tReject += totalRejCont/(NumBlock-1);
		
		if(tMaxAcc < maxAccepted) tMaxAcc = maxAccepted;
	}
	cout<<"Avg Miner     Time in microseconds = "<<tMiner/nItertion;;
	cout<<"\nAvg Validator Time in microseconds = "<<tVal/nItertion;;
	cout<<"\n--------------------------------";
	cout<<"\nAvg Validator Accepted a   Block   = "
		<<(numValidator-(tReject/nItertion));
	cout<<"\nAvg Validator Rejcted  a   Block   = "
		<<tReject/nItertion;
	cout<<"\nMax Validator Accepted any Block   = "<<tMaxAcc;
	cout<<"\n"<<endl;

	return 0;
}
