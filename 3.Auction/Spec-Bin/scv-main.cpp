#include <iostream>
#include <thread>
#include "Util/Timer.cpp"
#include "Contract/SimpleAuctionSCV.cpp"
#include "Util/FILEOPR.cpp"
#include <unistd.h>
#include <list>
#include <vector>
#include <atomic>
#include <condition_variable>
#include <set>
#include <algorithm>


#define maxThreads 128
#define maxBObj 5000
#define maxbEndT 5000 //millseconds
#define funInContract 6
#define pl "===================================================\n"
#define MValidation true  //! true or false
#define malMiner true     //! set the flag to make miner malicious.
#define NumOfDoubleSTx 2  //! # double-spending Tx for malicious final state by Miner.

using namespace std;
using namespace std::chrono;

int NumBlock     = 26;    //! at least two blocks, the first run is warmup run.
int numValidator = 50;
int beneficiary  = 0;     //! fixed beneficiary id to 0, it can be any unique address/id.
int    nBidder   = 2;     //! nBidder: number of bidder shared objects. default is 2.
int    nThread   = 1;     //! nThread: total number of concurrent threads; default is 1.
int    numAUs;            //! numAUs: total number of Atomic Unites to be executed.
double lemda;             //! λ: random delay seed.
int    bidEndT;           //! time duration for auction.
double tTime[2];          //! total time taken by miner and validator algorithm respectively.
SimpleAuction *auction;   //! smart contract for miner.
int    *aCount  = NULL;   //! aborted transaction count.
float_t *mTTime = NULL;   //! time taken by each miner Thread to execute AUs (Transactions).
float_t *vTTime = NULL;   //! time taken by each validator Thread to execute AUs (Transactions).
float_t *gTtime = NULL;   //! time taken by each miner Thread to add edges and nodes in the conflict graph.
vector<string>listAUs;    //! holds AUs to be executed on smart contract: "listAUs" index+1 represents AU_ID.
vector<string>seqBin;     //! holds sequential Bin AUs.
vector<string>concBin;    //! holds concurrent Bin AUs.
std::atomic<int>currAU;   //! used by miner-thread to get index of Atomic Unit to execute.
std::atomic<int>vCount;   //! # of valid AU node added in graph (invalid AUs will not be part of the graph & conflict list).
std::atomic<int>eAUCount; //! used by validator threads to keep track of how many valid AUs executed by validator threads.
mutex concLock, seqLock;  //! Lock used to access seq and conc bin.
float_t seqTime[2];       //! Used to store seq exe time.
std::atomic<bool>mm;      //! miner is malicious, this is used by validators.

//! STATE DATA
int mHBidder;
int mHBid;
int vHBidder;
int vHBid;
int *mPendingRet;
int *vPendingRet;



/*************************BARRIER CODE BEGINS****************************/
std::mutex mtx;
std::mutex pmtx; // to print in concurrent scene
std::condition_variable cv;
bool launch = false;

void wait_for_launch() {
	std::unique_lock<std::mutex> lck(mtx);
	while (!launch) cv.wait(lck);
}

void shoot() {
	std::unique_lock<std::mutex> lck(mtx);
	launch = true;
	cv.notify_all();
}
/*************************BARRIER CODE ENDS*****************************/





/*************************MINER CODE BEGINS*****************************/
/*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
!    Class "Miner" create & run "n" miner-thread concurrently           !
!"concMiner()" called by miner-thread to perfrom oprs of respective AUs !
!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
class Miner
{
	public:
	Miner( )
	{
		//! initialize the counter used to execute the numAUs
		currAU = 0;
		vCount = 0;
		//! index location represents respective thread id.
		mTTime = new float_t [nThread];
		gTtime = new float_t [nThread];
		aCount = new int [nThread];
		for(int i = 0; i < nThread; i++)  {
			mTTime[i] = 0;
			gTtime[i] = 0;
			aCount[i] = 0;
		}
		auction = new SimpleAuction(bidEndT, beneficiary, nBidder);
	}

	//!-------------------------------------------- 
	//!!!!!! MAIN MINER:: CREATE MINER THREADS !!!!
	//!--------------------------------------------
	void mainMiner()
	{
		Timer mTimer;
		thread T[nThread];
		//!---------------------------------------------------------
		//!!!!!!!!!!          Concurrent Phase            !!!!!!!!!!
		//!!!!!!!!!!    Create 'nThread' Miner threads    !!!!!!!!!!
		//!---------------------------------------------------------
		double s = mTimer.timeReq();
		for(int i = 0; i < nThread; i++)
			T[i] = thread(concMiner, i, numAUs);
		for(auto& th : T) th.join();
		tTime[0] = mTimer.timeReq() - s;
		//!------------------------------------------
		//!!!!!!!!!   Sequential Phase     !!!!!!!!!!
		//!------------------------------------------
//		seqTime[0] = 0;
		Timer SeqTimer;
//		auction->allUnlock();
		auto start = SeqTimer._timeStart();
		seqBinExe();
		seqTime[0] += SeqTimer._timeStop( start );
//		auction->allUnlock();

		//! print the final state of the shared objects.
		finalState();
	}

	//!-----------------------------------------------------------------
	//!!!!!!!!!!               Concurrent Phase               !!!!!!!!!!
	//! The function to be executed by all the miner threads. Thread   !
	//! executes the transaction concurrently from Concurrent Bin      !
	//!-----------------------------------------------------------------
	static void concMiner(int t_ID, int numAUs) {
		Timer thTimer;
		//! get the current index, and increment it.
		int curInd = currAU++;
		//! statrt clock to get time taken by this.AU
		auto start = thTimer._timeStart();
		while(curInd < numAUs) {
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
				int v = auction->bid_m(payable, bID, bAmt, true);
				//! if fun retrun -1 add the AU to seq Bin.
				if(v == -1) {
					vCount++;
					seqLock.lock();
					seqBin.push_back(listAUs[curInd]);
					seqLock.unlock();
				}
				else if(v == 1) {
					concLock.lock();
					concBin.push_back(listAUs[curInd]);
					concLock.unlock();
				}
				else {
					vCount++;
					seqLock.lock();
					seqBin.push_back(listAUs[curInd]);
					seqLock.unlock();
				}
			}
			if(tmp.compare("withdraw") == 0) {
				ss >> tmp;
				int bID = stoi(tmp);//! Bidder ID
				int v = auction->withdraw_m(bID, true);
				//! if fun retrun -1 add the AU to seq Bin.
				if(v == -1) {
					vCount++;
					seqLock.lock();
					seqBin.push_back(listAUs[curInd]);
					seqLock.unlock();
				}
				else if(v == 1) {
					concLock.lock();
					concBin.push_back(listAUs[curInd]);
					concLock.unlock();
				}
				else {
					vCount++;
					seqLock.lock();
					seqBin.push_back(listAUs[curInd]);
					seqLock.unlock();
				}
			}
			if(tmp.compare("auction_end") == 0) {
				int v = auction->auction_end_m( );
				seqLock.lock();
				seqBin.push_back(listAUs[curInd]);
				seqLock.unlock();
			}
			//! get the current index to execute, and increment it.
			curInd = currAU++;
		}
		mTTime[t_ID] += thTimer._timeStop(start);
	}


	//!------------------------------------------
	//!!!!!!!!!   Sequential Phase     !!!!!!!!!!
	//!------------------------------------------
	void seqBinExe( ) {
		int t_ID  = 0;
		int count = 0;
		while(count < seqBin.size()) {
			//! get the AU to execute, which is of string type.
			istringstream ss(seqBin[count]);
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
				int v = auction->bid_m(payable, bID, bAmt, false);
			}
			if(tmp.compare("withdraw") == 0) {
				ss >> tmp;
				int bID = stoi(tmp);//! Bidder ID
				int v = auction->withdraw_m(bID, false);
			}
			if(tmp.compare("auction_end") == 0) {
				int v = auction->auction_end_m( );
			}
			count++;
		}
	}


	//!-------------------------------------------------
	//!Final state of all the shared object. Once all  |
	//!AUs executed. We are geting this using state()  |
	//!-------------------------------------------------
	void finalState() {	
		auction->state_m(&mHBidder, &mHBid, mPendingRet);//Entire state
	}
	~Miner() { };
};
/********************MINER CODE ENDS*********************************/







/********************VALIDATOR CODE BEGINS***************************/
/*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
! Class "Validator" create & run "n" validator-Thread concurrently   !
! based on CONC and SEQ BIN given by miner operations of respective  !
! AUs. Thread 0 is considered as smart contract deployer.            !
!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
class Validator
{
	public:
	Validator()
	{
		//! int the execution counter used by validator threads.
		eAUCount = 0;
		//! array index location represents respective thread id.
		vTTime = new float_t [nThread];
		for(int i = 0; i < nThread; i++) vTTime[i] = 0;
	};

	//!----------------------------------------
	//! create n concurrent validator threads | 
	//! to execute valid AUs.                 |
	//!----------------------------------------
	void mainValidator()
	{
		for(int i = 0; i < nThread; i++) vTTime[i] = 0;
		eAUCount = 0;
		Timer vTimer;
		thread T[nThread];
		auction->reset(bidEndT);
		//!---------------------------------------------------------
		//!!!!!!!!!!          Concurrent Phase            !!!!!!!!!!
		//!!!!!!!!!!  Create 'nThread' Validator threads  !!!!!!!!!!
		//!---------------------------------------------------------
		double s = vTimer.timeReq();
		for(int i = 0; i<nThread; i++) T[i] = thread(concValidator, i);
		shoot();
		for(auto& th : T) th.join( );
		tTime[1] = vTimer.timeReq() - s;
		//!------------------------------------------
		//!!!!!!!!!   Sequential Phase     !!!!!!!!!!
		//!------------------------------------------
//		seqTime[1] = 0;
		Timer SeqTimer;
		auto start = SeqTimer._timeStart();
		seqBinExe();
		seqTime[1] += SeqTimer._timeStop( start );

		//!print the final state of the shared objects by validator.
		finalState();
	}

	//!------------------------------------------------------------
	//!!!!!!!          Concurrent Phase                    !!!!!!!!
	//!!!!!!  'nThread' Validator threads Executes this Fun !!!!!!!
	//!------------------------------------------------------------
	static void concValidator( int t_ID ) {
		//barrier to synchronise all threads for a coherent launch
		wait_for_launch();
		Timer Ttimer;
		auto start = Ttimer._timeStart();
		int curInd = eAUCount++;
		while( curInd< concBin.size() && mm == false) {
			//! get the AU to execute, which is of string type.
			istringstream ss(concBin[curInd]);
			string tmp;
			ss >> tmp; //! AU_ID to Execute.
			int AU_ID = stoi(tmp);
			ss >> tmp;
			if(tmp.compare("bid") == 0) {
				ss >> tmp;
				int payable = stoi(tmp);//! payable
				ss >> tmp;
				int bID = stoi(tmp);//! Bidder ID
				ss >> tmp;
				int bAmt = stoi(tmp);//! Bidder value
				int v = auction->bid(payable, bID, bAmt);
				if(v == -1) mm = true;
			}
			if(tmp.compare("withdraw") == 0) {
				ss >> tmp;
				int bID = stoi(tmp);//! Bidder ID
				int v = auction->withdraw(bID);
				if(v == -1) mm = true;
			}
			if(tmp.compare("auction_end") == 0) {
				int v = auction->auction_end( );
			}
			curInd = eAUCount++;
		}
		//!stop timer to get time taken by this thread
		vTTime[t_ID] += Ttimer._timeStop( start );
	}


	//!------------------------------------------
	//!!!!!!!!!   Sequential Phase     !!!!!!!!!!
	//!------------------------------------------
	void seqBinExe( ) {
		int t_ID = 0;
		int count = 0;
		while(count < seqBin.size() && mm == false) {
			//! get the AU to execute, which is of string type.
			istringstream ss(seqBin[count]);
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
				int v = auction->bid(payable, bID, bAmt);
				if(v == -1) mm = true;
			}
			if(tmp.compare("withdraw") == 0) {
				ss >> tmp;
				int bID = stoi(tmp);//! Bidder ID
				int v = auction->withdraw(bID);
				if(v == -1) mm = true;
			}
			if(tmp.compare("auction_end") == 0) {
				int v = auction->auction_end( );
			}
			count++;
		}
	}

	//!-------------------------------------------------
	//!Final state of all the shared object. Once all  |
	//!AUs executed. We are geting this using state()  |
	//!-------------------------------------------------
	void finalState() {
		auction->state(&vHBidder, &vHBid, vPendingRet);
	}

	~Validator() { };
};
/**********************VALIDATOR CODE ENDS***************************/


void eraseAUID(int id, bool Bin) {
	if(Bin == true){
		for(auto its = seqBin.begin(); its < seqBin.end(); its++) {
			istringstream ss(*its);
			string auid;
			ss >> auid;
			int A_ID = stoi(auid);
			if(A_ID == id){
				seqBin.erase(its);
				return;
			}
		}
	}
	else {
		for(auto it = concBin.begin(); it < concBin.end(); it++) {
			istringstream ss(*it);
			string auid;
			ss >> auid;
			int A_ID = stoi(auid);
			if(A_ID == id){
				concBin.erase(it);
				return;
			}
		}
	}
}



//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//! atPoss:: from which double-spending Tx to be stored at begining  !
//! of the list. Add malicious final state with double-spending Tx   !
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
bool addMFS(int atPoss) {
	istringstream ss(listAUs[atPoss-2]);
	string trns1;
	ss >> trns1; //! AU_ID to Execute.
	int AU_ID1 = stoi(trns1);
	ss >> trns1;//function name
	ss >> trns1; //! payable.
	int payable = stoi(trns1);
	ss >> trns1; //! bidder ID.
	int bidderID = stoi(trns1);
	ss >> trns1; //! Ammount to bid.
	int bidValue  = stoi(trns1);

	istringstream ss1(listAUs[atPoss-1]);
	string trns2;
	ss1 >> trns1; //! AU_ID to Execute.
	int AU_ID2 = stoi(trns1);
	ss1 >> trns1;//function name
	ss1 >> trns1; //! payable.
	int payable1 = stoi(trns1);
	ss1 >> trns1; //! bidderID.
	int bidderID1 = stoi(trns1);
	ss1 >> trns1; //! Ammount to bid.
	int bidValue1  = stoi(trns1);

	bidValue = 999;
	trns1 = to_string(AU_ID1)+" bid "+to_string(payable)+" "
	       +to_string(bidderID)+" "+to_string(bidValue);
	listAUs[AU_ID1-1] =  trns1;

	bidValue1 = 1000;
	trns2 = to_string(AU_ID2)+" bid "+to_string(payable1)+" "
	       +to_string(bidderID1)+" "+to_string(bidValue1);
	listAUs[AU_ID2-1] =  trns1;

	mHBidder = bidderID1;
	mHBid    = bidValue;

	//! add the confliciting AUs in conc bin and remove them from seq bin. Add
	//! one of the AU from seq bin to conc Bin and remove that AU from seq bin.
	eraseAUID(AU_ID1, true);
	eraseAUID(AU_ID2, true);
	eraseAUID(AU_ID1, false);
	eraseAUID(AU_ID2, false);
	concBin.insert(concBin.begin(), trns1);
	concBin.insert(concBin.begin()+1, trns2);
	return true;
}


void printBins( ) {
	int concCount = 0, seqCount = 0;
	cout<<endl<<"=====================\n"
		<<"Concurrent Bin AUs\n=====================";
	for(int i = 0; i < concBin.size(); i++) {
		cout<<"\n"<<concBin[i];
		concCount++;
	}
	cout<<endl;
	cout<<endl<<"=====================\n"
		<<"Sequential Bin AUs\n=====================";
	for(int i = 0; i < seqBin.size(); i++) {
		cout<<"\n"<<seqBin[i];
		seqCount++;
	}
	cout<<"\n=====================\n"
		<<"Conc AU Count "<< concCount 
		<<"\nSeq AU Count "<<seqCount
		<<"\n=====================\n";
}



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




/**********************MAIN FUN CODE BEGINS***************************/
/*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
/*!!!!!!!!          main()         !!!!!!!!!!*/
/*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
int main(int argc, char *argv[])
{
	cout<<pl<<"Speculative Bin Miner and Smart Concurrent Validator\n";
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
	int  tReject  = 0;
	int tMaxAcc   = 0;
	float tSeqMT  = 0;
	float tSeqVT  = 0;

	//! list holds the avg time taken by miner and Validator
	//! thread s for multiple consecutive runs.
	list<float>mItrT;
	list<float>vItrT;
	int totalDepInG  = 0; //to get total number of dependencies in graph;
	int totalRejCont = 0; //number of validator rejected the blocks;
	int maxAccepted  = 0;
	int totalRun     = NumBlock; //at least 2

	FILEOPR file_opr;

	//! read from input file:: nBidder = #nBidder; nThread = #threads;
	//! numAUs = #AUs; λ = random delay seed.
	file_opr.getInp(&nBidder, &bidEndT, &nThread, &numAUs, &lemda);

	if(nBidder > maxBObj) {
		nBidder = maxBObj;
		cout<<"Max number Bidder Shared Object can be "<<maxBObj<<"\n";
	}

	float valTime;
	for(int itr = 0; itr < nItertion; itr++)
	{
		seqTime[0]    = seqTime[1] = 0;
		totalRejCont  = 0;
		maxAccepted   = 0;
		valTime       = 0;

		float Blockvalt;
		for(int nBlock = 0; nBlock < NumBlock; nBlock++)
		{
			file_opr.genAUs(numAUs, nBidder, funInContract, listAUs);
			tTime[0]  = 0, tTime[1] = 0;
			Blockvalt = 0;
			mPendingRet = new int [nBidder+1];
			vPendingRet = new int [nBidder+1];
			mm = new std::atomic<bool>;
			mm = false;
			Timer mTimer;
			mTimer.start();

			//MINER
			Miner *miner = new Miner();
			miner ->mainMiner();

			if(lemda != 0) bool rv = addMFS(NumOfDoubleSTx);

	//		printBins();

			//VALIDATOR
				float valt      = 0;
				int acceptCount = 0, rejectCount = 0;
				for(int nval = 0; nval < numValidator; nval++)
				{
					valt = 0;
					Validator *validator = new Validator();
					validator ->mainValidator();

					bool flag = stateVal();
					if(flag == true) rejectCount++;
					else acceptCount++;
					mm = false;

					int counterv = 0;
					for( int x = 0; x < nThread; x++ ){
						if(vTTime[x] != 0) {
							valt += vTTime[x];
							counterv++;
						}
					}
					if(nBlock > 0) Blockvalt  += valt/counterv;
				}
				if(nBlock > 0 && malMiner == true) {
					totalRejCont += rejectCount;
					if(maxAccepted < acceptCount ) maxAccepted = acceptCount;
				}

			//! total valid AUs among total AUs executed 
			//! by miner and varified by Validator.
			int vAUs =  seqBin.size() + concBin.size();
			if(nBlock > 0)
			file_opr.writeOpt(nBidder, nThread, numAUs, tTime, mTTime, vTTime,
			                  aCount, vAUs, mItrT, vItrT, 1);

			concBin.clear();
			seqBin.clear();
			listAUs.clear();
			delete miner;
			miner = NULL;

			valTime  += Blockvalt/numValidator;
		}
		//to get total avg miner and validator
		//time after number of totalRun runs.
		float tAvgMinerT = 0, tAvgValidT = 0;
		int  cnt = 0;
		auto mit = mItrT.begin();
		auto vit = vItrT.begin();
		for(int j = 1; j < totalRun; j++){
			tAvgMinerT = tAvgMinerT + *mit;
			if(*vit != 0){
				tAvgValidT = tAvgValidT + *vit;
				cnt++;
			}
			mit++;
			vit++;
		}
		tMiner   += tAvgMinerT/(NumBlock-1);
		tVal     += valTime/(NumBlock-1);
		tReject  += totalRejCont /(NumBlock-1);
		tSeqMT   += seqTime[0];
		tSeqVT   += seqTime[1];

		if(tMaxAcc  < maxAccepted) tMaxAcc = maxAccepted;
		mItrT.clear();
		vItrT.clear();
	}
	//Miner Sequential Phase Time
	float avgMStime = tSeqMT/(NumBlock*nItertion);
	//Validator Sequential Phase Time
	float avgVStime = tSeqVT/(nItertion*NumBlock*numValidator);

	cout<<  "Avg Miner     Time in microseconds  = "
	    <<(tMiner/nItertion)+avgMStime;
	cout<<"\nAvg Validator Time in microseconds  = "
	    <<(tVal/nItertion)+avgVStime;
	cout<<"\n-----------------------------";
	cout<<"\nMiner     Seq Time in microseconds  = "<<avgMStime
		<<"\nValidator Seq Time in microseconds  = "<<avgVStime;
	cout<<"\n-----------------------------";
	cout<<"\nAvg Validator Accepted a   Block    = "
		<<(numValidator-(tReject/nItertion));
	cout<<"\nAvg Validator Rejcted  a   Block    = "
		<<tReject/nItertion;
	cout<<"\nMax Validator Accepted any Block    = "<<tMaxAcc;
	cout<<"\n"<<endl;

	mItrT.clear();
	vItrT.clear();
	delete mTTime;
	delete vTTime;
	return 0;
}
/*********************MAIN FUN CODE ENDS***********************/
