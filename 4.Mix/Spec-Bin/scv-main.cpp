#include <iostream>
#include <thread>
#include <atomic>
#include <list>
#include "Util/Timer.cpp"
#include "Util/FILEOPR.cpp"
#include "Contract/MixSCV.cpp"
#include <unistd.h>
#include <list>
#include <vector>
#include <atomic>
#include <condition_variable>
#include <set>
#include <algorithm>

#define CoinIdStart 5000
#define VoterIdStart 10000 //Proposal Id is -Ve in I/P file
#define AuctionIdStart 1


#define MAX_THREADS 128
#define maxCoinAObj 5000  //maximum coin contract \account\ object.
#define CFCount 3         //# methods in coin contract.

#define maxPropObj 1000   //maximum ballot contract \proposal\ shared object.
#define maxVoterObj 40000 //maximum ballot contract \voter\ shared object.
#define BFCount 5         //# methods in ballot contract.

#define maxBidrObj 5000   //maximum simple auction contract \bidder\ shared object.
#define maxbEndT 600      //maximum simple auction contract \bidding time out\ duration.
#define AFConut 6         //# methods in simple auction contract.
#define pl "===================================================\n"
#define MValidation true   //! true or false
#define InitBalance 1000
#define malMiner true      //! set the flag to make miner malicious.
#define NumOfDoubleSTx 2   //! # double-spending Tx for malicious final state by Miner, multiple of 2.


using namespace std;
using namespace std::chrono;

int NumBlock     = 26;    //! at least two blocks, the first run is warmup run.
int numValidator = 50;

//! INPUT FROM INPUT FILE.
int nThread;    //! # of concurrent threads;
int numAUs;     //! # Atomic Unites to be executed.
int CoinSObj;   //! # Coin Shared Object.
int nProposal;  //! # Ballot Proposal Shared Object.
int nVoter;     //! # Ballot Voter Shared Object.
int nBidder;    //! # Ballot Voter Shared Object.
int bidEndT;    //! time duration for auction.
int lemda;      //!  % of edges to be removed from BG by malicious Miner.

double tTime[2];           //! total time taken by miner & validator.
int    *aCount;            //! Invalid transaction count.
float_t*mTTime;            //! time taken by each miner Thread to execute AUs.
float_t*vTTime;            //! time taken by each validator Thread to execute AUs.
vector<string>listAUs;     //! holds AUs to be executed on smart contract: "listAUs" index+1 represents AU_ID.
vector<string>seqBin;      //! holds sequential Bin AUs.
vector<string>concBin;     //! holds concurrent Bin AUs.
std::atomic<int>currAU;    //! used by miner-thread to get index of Atomic Unit to execute.
std::atomic<int>vCount;    //! # of valid AU.
std::atomic<int>eAUCount;  //! used by validator threads to keep track of how many valid AUs executed by validator threads.
mutex concLock, seqLock;   //! Lock used to access seq and conc bin.
float_t seqTime[2];        //! Used to store seq exe time.
std::atomic<bool>mm;       //! miner is malicious, this is used by validators.
string *proposalNames;     //! proposal names for ballot contract.
//! CONTRACT OBJECTS;
Coin   *coin;            //! coin contract miner object.
Ballot *ballot;          //! ballot contract for miner object.
SimpleAuction *auction;  //! simple auction contract for miner object.

int *mCoinState;
int *vCoinState;
int *mBallotProState;
int *vBallotProState;
int *mBallotVotState;
int *vBallotVotState;
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
	Miner(int contDeployer)
	{
		//! initialize the counter used to execute the numAUs
		currAU = 0;
		vCount = 0;
		proposalNames = new string[nProposal+1];
		for(int x = 0; x <= nProposal; x++)
			proposalNames[x] = "X"+to_string(x+1);

		//! index location represents respective thread id.
		mTTime = new float_t [nThread];
		aCount = new int [nThread];
		for(int i = 0; i < nThread; i++) {
			mTTime[i] = 0;
			aCount[i] = 0;
		}
		//! id of the contract creater is "minter_id = 5000".
		coin = new Coin( CoinSObj, 5000 );
		//! Id of the contract creater is \chairperson = 10000\.
		ballot = new Ballot( proposalNames, 10000, nVoter, nProposal);
		//! fixed beneficiary id to 0, it can be any unique address/id.
		auction = new SimpleAuction(bidEndT, 0, nBidder);
	}

	//!---------------------------------------------------- 
	//!!!!!!!! MAIN MINER:: CREATE MINER THREADS !!!!!!!!!!
	//!----------------------------------------------------
	void mainMiner()
	{
		Timer mTimer;
		thread T[nThread];
		//! initialization of account with
		//! fixed ammount; mint() is serial.
		int bal = InitBalance;
		for(int sid = 1; sid <= CoinSObj; sid++) {
			//! 5000 is contract deployer.
			coin->mint_m(5000, CoinIdStart+sid, bal);
		}
		//! Give \`voter\` the right to vote on this ballot.
		//! giveRightToVote_m() is serial.
		for(int voter = 1; voter <= nVoter; voter++) {
			//! 10000 is chairperson.
			ballot->giveRightToVote(10000, VoterIdStart+voter);
		}

		//!---------------------------------------------------------
		//!!!!!!!!!!          Concurrent Phase            !!!!!!!!!!
		//!!!!!!!!!!    Create 'nThread' Miner threads    !!!!!!!!!!
		//!---------------------------------------------------------
		//! Start clock to get time taken by miner algorithm.
		double s = mTimer.timeReq();
		for(int i = 0; i < nThread; i++)
			T[i] = thread(concMiner, i, numAUs);
		shoot();
		for(auto& th : T) th.join();
		tTime[0] = mTimer.timeReq() - s;


		//!------------------------------------------
		//!!!!!!!!!   Sequential Phase     !!!!!!!!!!
		//!------------------------------------------
//		coin->allUnlock();
//		ballot->allUnlock();
//		auction->allUnlock();
//		seqTime[0] = 0;
		Timer SeqTimer;
		auto start = SeqTimer._timeStart();
		seqBinExe();
		seqTime[0] += SeqTimer._timeStop( start );
//		coin->allUnlock();
//		ballot->allUnlock();
//		auction->allUnlock();
		//! print the final state of the shared objects.
		finalState();
	}


	//!--------------------------------------------------------
	//! THE FUNCTION TO BE EXECUTED BY ALL THE MINER THREADS. !
	//!--------------------------------------------------------
	static void concMiner( int t_ID, int numAUs)
	{
		//barrier to synchronise all threads for a coherent launch
		wait_for_launch();
		Timer thTimer;
		//! get the current index, and increment it.
		int curInd = currAU++;
		//! statrt clock to get time taken by this.AU
		auto start = thTimer._timeStart();
		while(curInd < numAUs)
		{
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

			if(tmp.compare("get_bal") == 0) {
				ss >> tmp; //! get balance of SObj/id.
				int s_id = stoi(tmp);
				int bal  = 0;
				int v = coin->get_bal_m(s_id, &bal, t_ID, true);
				//! if fun retrun -1 add the AU to seq Bin.
				if(v == -1) {
					vCount++;
					seqLock.lock();
					seqBin.push_back(listAUs[curInd]);
					seqLock.unlock();
				}
				else {
					concLock.lock();
					concBin.push_back(listAUs[curInd]);
					concLock.unlock();
				}
			}
			if(tmp.compare("send") == 0) {
				ss >> tmp; //! Sender ID.
				int s_id  = stoi(tmp);
				ss >> tmp; //! Reciver ID.
				int r_id  = stoi(tmp);
				ss >> tmp; //! Ammount to send.
				int amt   = stoi(tmp);
				int v = coin->send_m(t_ID, s_id, r_id, amt, true);
				if(v == 0) {
					//! invalid AU: sender does't 
					//! have sufficent balance to send.
//					cout<<"\nConcBIn:Sender don't have sufficent balance to send.\n";
				}
				//! if fun retrun -1 add the AU to seq Bin.
				else if(v == -1) {
					vCount++;
					seqLock.lock();
					seqBin.push_back(listAUs[curInd]);
					seqLock.unlock();
				}
				else {
					concLock.lock();
					concBin.push_back(listAUs[curInd]);
					concLock.unlock();
				}
			}
			if(tmp.compare("vote") == 0) {
				ss >> tmp;
				int vID = stoi(tmp);//! voter ID
				ss >> tmp;
				int pID = stoi(tmp);//! proposal ID
				int PID = -pID; 
				int v = ballot->vote_m(vID, PID, true);
				//! if fun retrun -1 add the AU to seq Bin.
				if(v == -1) {
					vCount++;
					seqLock.lock();
					seqBin.push_back(listAUs[curInd]);
					seqLock.unlock();
				}
				else if(v == 0){
//					cout<<"Error!!! Invalid AU\n";
				}
				else {
					concLock.lock();
					concBin.push_back(listAUs[curInd]);
					concLock.unlock();
				}	
			}
			if(tmp.compare("delegate") == 0) {
				ss >> tmp;
				int sID = stoi(tmp);//! Sender ID
				ss >> tmp;
				int rID = stoi(tmp);//! Reciver ID
				int v = ballot->delegate_m(sID, rID, true);
				//! if fun retrun -1 add the AU to seq Bin.
				if(v == -1) {
					vCount++;
					seqLock.lock();
					seqBin.push_back(listAUs[curInd]);
					seqLock.unlock();
				}
				else if(v == 0){
					//cout<<"Error!!! Invalid AU\n";
				}
				else {
					concLock.lock();
					concBin.push_back(listAUs[curInd]);
					concLock.unlock();
				}
			}
			//! get the current index to execute, and increment it.
			curInd = currAU++;
		}
		mTTime[t_ID] = mTTime[t_ID] + thTimer._timeStop(start);
	}


	//!------------------------------------------
	//!!!!!!!!!   Sequential Phase     !!!!!!!!!!
	//!------------------------------------------
	void seqBinExe( )
	{
		int t_ID  = 0;
		int count = 0;
		while(count < seqBin.size())
		{
			//! get the AU to execute, which is of string type.
			istringstream ss(seqBin[count]);
			string tmp;
			ss >> tmp; //! AU_ID to Execute.
			int AU_ID = stoi(tmp);
			ss >> tmp; //! Function Name (smart contract).
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
			if(tmp.compare("get_bal") == 0) {
				ss >> tmp; //! get balance of SObj/id.
				int s_id = stoi(tmp);
				int bal  = 0;
				bool v = coin->get_bal_m(s_id, &bal, t_ID, false);
			}
			if(tmp.compare("send") == 0) {
				ss >> tmp; //! Sender ID.
				int s_id  = stoi(tmp);
				ss >> tmp; //! Reciver ID.
				int r_id  = stoi(tmp);
				ss >> tmp; //! Ammount to send.
				int amt   = stoi(tmp);
				int v = coin->send_m(t_ID, s_id, r_id, amt, false);
				if(v == false) {
					//! invalid AU: sender does't 
					//! have sufficent balance to send.
//					cout<<"\nSeqBIn: Sender don't have sufficent balance to send.\n";
				}
			}
			if(tmp.compare("vote") == 0) {
				ss >> tmp;
				int vID = stoi(tmp);//! voter ID
				ss >> tmp;
				int pID = stoi(tmp);//! proposal ID
				int PID = -pID; 
				int v = ballot->vote_m(vID, PID, false);
				if(v == 0){
//					seqBin.erase(seqBin.begin()+count);
//					cout<<"Error!!! Invalid AU:: "<< seqBin[count]<<endl;
				}				
			}
			if(tmp.compare("delegate") == 0) {
				ss >> tmp;
				int sID = stoi(tmp);//! Sender ID
				ss >> tmp;
				int rID = stoi(tmp);//! Reciver ID
				int v = ballot->delegate_m(sID, rID, false);
				if(v == 0){
//					seqBin.erase(seqBin.begin()+count);
//					cout<<"Error!!! Invalid AU:: "<< seqBin[count]<<endl;
				}				
			}
			count++;
		}
	}

	//!-------------------------------
	//!FINAL STATE OF ALL THE SHARED |
	//!OBJECT. Once all AUs executed.|
	//!-------------------------------
	void finalState()
	{
		//coin state
		for(int sid = 1; sid <= CoinSObj; sid++) {
			int bal = 0;
			coin->get_bal_m(CoinIdStart+sid, &bal, 0, false);
			mCoinState[sid] = bal;
		}

		for(int id = 1; id <= nVoter; id++) //for voter state
			ballot->state_m(VoterIdStart+id, true, mBallotVotState);

		for(int id = 1; id <= nProposal; id++) 
			ballot->state_m(id, false, mBallotProState);//for Proposal state

		auction->state_m(&mHBidder, &mHBid, mPendingRet);//Auction state
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
	Validator() {
		//! int the execution counter used by validator threads.
		eAUCount = 0;
		//! array index location => thread id.
		vTTime   = new float_t[nThread];
		for(int i = 0; i < nThread; i++) vTTime[i] = 0;
	};

	//!----------------------------------------
	//! create n concurrent validator threads | 
	//! to execute valid AUs.                 |
	//!----------------------------------------
	void mainValidator()
	{
//		for(int i = 0; i < nThread; i++) vTTime[i] = 0;
		eAUCount = 0;

		Timer vTimer;
		thread T[nThread];
		auction->reset(bidEndT);
		ballot->reset();
		coin->reset(); 

		//! initialization of account with fixed ammount;
		//! mint_val() function is serial.
		int bal = InitBalance;
		for(int sid = 1; sid <= CoinSObj; sid++) 
		{
			//! 5000 is contract deployer.
			bool v = coin->mint(5000, CoinIdStart+sid, bal);
		}

		//! giveRightToVote() function is serial.
		for(int voter = 1; voter <= nVoter; voter++) 
		{
			//! 10000 is chairperson.
			ballot->giveRightToVote(10000, VoterIdStart+voter);
		}

		//!---------------------------------------------------------
		//!!!!!!!!!!          Concurrent Phase            !!!!!!!!!!
		//!!!!!!!!!!  Create 'nThread' Validator threads  !!!!!!!!!!
		//!---------------------------------------------------------
		//!Start clock
		double s = vTimer.timeReq();
		for(int i = 0; i<nThread; i++)
			T[i] = thread(concValidator, i);
		shoot(); //notify all threads to begin the worker();
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
		//! print the final state of the shared objects by validator.
		finalState();
	}

	//!------------------------------------------------------------
	//!!!!!!!          Concurrent Phase                    !!!!!!!!
	//!!!!!!  'nThread' Validator threads Executes this Fun !!!!!!!
	//!------------------------------------------------------------
	static void concValidator( int t_ID )
	{
		//barrier to synchronise all threads for a coherent launch
		wait_for_launch();
		Timer Ttimer;
		auto start = Ttimer._timeStart();
		int curInd = eAUCount++;
		while( curInd< concBin.size() && mm == false)
		{
			//! get the AU to execute, which is of string type.
			istringstream ss(concBin[curInd]);
			string tmp;
			ss >> tmp; //! AU_ID to Execute.
			int AU_ID = stoi(tmp);
			ss >> tmp; //! Function Name (smart contract).
			if(tmp.compare("bid") == 0) {
				ss >> tmp;
				int payable = stoi(tmp);//! payable
				ss >> tmp;
				int bID = stoi(tmp);//! Bidder ID
				ss >> tmp;
				int bAmt = stoi(tmp);//! Bidder value
				int v = auction->bid(payable, bID, bAmt);
			}
			if(tmp.compare("withdraw") == 0) {
				ss >> tmp;
				int bID = stoi(tmp);//! Bidder ID
				int v = auction->withdraw(bID);
			}
			if(tmp.compare("auction_end") == 0) {
				int v = auction->auction_end( );
			}
			if(tmp.compare("get_bal") == 0) {
				ss >> tmp; //! get balance of SObj/id.
				int s_id = stoi(tmp);
				int bal  = 0;
				//! get_bal() of smart contract.
				bool v = coin->get_bal(s_id, &bal);
				if(v == -1)	mm = true;
			}
			if( tmp.compare("send") == 0 ) {
				ss >> tmp; //! Sender ID.
				int s_id = stoi(tmp);
				ss >> tmp; //! Reciver ID.
				int r_id = stoi(tmp);
				ss >> tmp; //! Ammount to send.
				int amt  = stoi(tmp);
				bool v   = coin->send(s_id, r_id, amt);
				if(v == -1)	mm = true;
			//	if(v == false)
			//		cout<<"\nConcBin:Sender don't have sufficent balance to send.\n";
			}
			if(tmp.compare("vote") == 0) {
				ss >> tmp;
				int vID = stoi(tmp);//! voter ID
				ss >> tmp;
				int pID = stoi(tmp);//! proposal ID
				int PID = -pID; 
				int v = ballot->vote(vID, PID);
			}
			if(tmp.compare("delegate") == 0) {
				ss >> tmp;
				int sID = stoi(tmp);//! Sender ID
				ss >> tmp;
				int rID = stoi(tmp);//! Reciver ID
				int v = ballot->delegate(sID, rID);
			}
			curInd = eAUCount++;
		}
		//!stop timer to get time taken by this thread
		vTTime[t_ID] = vTTime[t_ID] + Ttimer._timeStop( start );
	}


	//!------------------------------------------
	//!!!!!!!!!   Sequential Phase     !!!!!!!!!!
	//!------------------------------------------
	void seqBinExe( )
	{
		int t_ID = 0;
		int count = 0;
		while(count < seqBin.size() && mm == false)
		{
			//! get the AU to execute, which is of string type.
			istringstream ss(seqBin[count]);
			string tmp;
			ss >> tmp; //! AU_ID to Execute.
			int AU_ID = stoi(tmp);
			ss >> tmp; //! Function Name (smart contract).
			if(tmp.compare("bid") == 0) {
				ss >> tmp;
				int payable = stoi(tmp);//! payable
				ss >> tmp;
				int bID = stoi(tmp);//! Bidder ID
				ss >> tmp;
				int bAmt = stoi(tmp);//! Bidder value
				int v = auction->bid(payable, bID, bAmt);
			}
			if(tmp.compare("withdraw") == 0) {
				ss >> tmp;
				int bID = stoi(tmp);//! Bidder ID
				int v = auction->withdraw(bID);
			}
			if(tmp.compare("auction_end") == 0) {
				int v = auction->auction_end( );
			}
			if(tmp.compare("get_bal") == 0) {
				ss >> tmp; //! get balance of SObj/id.
				int s_id = stoi(tmp);
				int bal  = 0;
				//! get_bal() of smart contract.
				bool v = coin->get_bal(s_id, &bal);
				if(v == -1)	mm = true;
			}
			if( tmp.compare("send") == 0 ) {
				ss >> tmp; //! Sender ID.
				int s_id = stoi(tmp);
				ss >> tmp; //! Reciver ID.
				int r_id = stoi(tmp);
				ss >> tmp; //! Ammount to send.
				int amt  = stoi(tmp);
				bool v   = coin->send(s_id, r_id, amt);
				if(v == -1)	mm = true;
			//	if(v == false)
			//		cout<<"\nConcBin:Sender don't have sufficent balance to send.\n";
			}
			if(tmp.compare("vote") == 0) {
				ss >> tmp;
				int vID = stoi(tmp);//! voter ID
				ss >> tmp;
				int pID = stoi(tmp);//! proposal ID
				int PID = -pID; 
				int v = ballot->vote(vID, PID);
			}
			if(tmp.compare("delegate") == 0) {
				ss >> tmp;
				int sID = stoi(tmp);//! Sender ID
				ss >> tmp;
				int rID = stoi(tmp);//! Reciver ID
				int v = ballot->delegate(sID, rID);
			}
			count++;
		}
	}

	//!-------------------------------
	//!FINAL STATE OF ALL THE SHARED |
	//!OBJECT. Once all AUs executed.|
	//!-------------------------------
	void finalState()
	{
		//coin state
		for(int sid = 1; sid <= CoinSObj; sid++) {
			int bal = 0;
			bool v  = coin->get_bal(CoinIdStart+sid, &bal);
			vCoinState[sid] = bal;
		}

		for(int id = 1; id <= nVoter; id++) 
			ballot->state(VoterIdStart+id, true, vBallotVotState);

		for(int id = 1; id <= nProposal; id++) 
			ballot->state(id, false, vBallotProState);//for Proposal state
		
		auction->state(&vHBidder, &vHBid, vPendingRet); //auction state
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


//!--------------------------------------------------------------------------
//! atPoss:: from which double-spending Tx to be stored at end of the list. !
//! add malicious final state with double-spending Tx                       !
//!--------------------------------------------------------------------------
bool addMFS(int atPoss)
{
	istringstream ss(listAUs[atPoss-2]);
	string trns1;
	string trns2;
	ss >> trns1; //! AU_ID to Execute.
	int AU_ID1 = stoi(trns1);
	ss >> trns1;//function name
	ss >> trns1; //! Sender ID.
	int s_id = stoi(trns1);
	ss >> trns1; //! Reciver ID.
	int r_id = stoi(trns1);
	ss >> trns1; //! Ammount to send.
	int amtAB  = stoi(trns1);

	istringstream ss1(listAUs[atPoss-1]);
	ss1 >> trns2; //! AU_ID to Execute.
	int AU_ID2 = stoi(trns2);
	ss1 >> trns2;//function name
	ss1 >> trns2; //! Sender ID.
	int s_id1 = stoi(trns2);
	ss1 >> trns2; //! Reciver ID.
	int r_id1 = stoi(trns2);
	ss1 >> trns2; //! Ammount to send.
	int amtAC  = stoi(trns2);


	mCoinState[s_id-CoinIdStart]  = 1000;
	mCoinState[r_id-CoinIdStart]  = 1000;
	mCoinState[r_id1-CoinIdStart] = 1000;
	amtAB = 950;
	trns1 = to_string(AU_ID1)+" send "+to_string(s_id)+" "
			+to_string(r_id)+" "+to_string(amtAB);
	listAUs[AU_ID1-1] =  trns1;
	amtAC = 100;
	trns2 = to_string(AU_ID2)+" send "+to_string(s_id)+" "
			+to_string(r_id1)+" "+to_string(amtAC);
	listAUs[AU_ID2-1] =  trns2;
	mCoinState[s_id-CoinIdStart]  -= amtAB;
	mCoinState[r_id-CoinIdStart]  += amtAB;
	mCoinState[r_id1-CoinIdStart] += amtAC;
	
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



void printBins( )
{
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



/*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
/*!!!!!!!!     State Validation    !!!!!!!!!!*/
/*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
bool stateVal() {
	bool flag = false;
//	cout<<"\n"<<pl<<"Object \tMiner \t\tValidator"<<endl;
	for(int sid = 1; sid <= CoinSObj; sid++) {
//		cout<<sid<<" \t \t"<<mCoinState[sid]
//			<<" \t\t"<<vCoinState[sid]<<endl;
		if(mCoinState[sid] != vCoinState[sid]) flag = true;
	}
//	cout<<"\n"<<pl<<"Prop ID \tMiner \t\tValidator"<<endl;
	for(int pid = 0; pid < nProposal; pid++) {
//		cout<<pid+1<<" \t \t"<<mBallotProState[pid]
//			<<" \t\t"<<vBallotProState[pid]<<endl;
		if(mBallotProState[pid] != vBallotProState[pid]) flag = true;
	}
//	cout<<"\n"<<pl<<"Voter ID \tMiner \t\tValidator"<<endl;
	for(int vid = 0; vid < nVoter; vid++) {
//		cout<<vid+1<<" \t \t"<<mBallotVotState[vid]
//			<<" \t\t"<<vBallotVotState[vid]<<endl;
		if(mBallotVotState[vid] != vBallotVotState[vid]) flag = true;
	}
	
	if(mHBidder != vHBidder || mHBid != vHBid) flag = true;
//	cout<<pl<<"     Miner Auction Winer "+to_string(mHBidder)
//			+" |  Amount "+to_string(mHBid);
//	cout<<"\n Validator Auction Winer "+to_string(vHBidder)
//			+" |  Amount "+to_string(vHBid);
//	cout<<"\n"<<pl;
	return flag;
}



/**********************MAIN FUN CODE BEGINS***************************/
/*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
/*!!!!!!!!!!!!!!!   main() !!!!!!!!!!!!!!!!!!*/
/*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
int main(int argc, char *argv[])
{
	cout<<pl<<"Speculative Bin Miner and Concurrent Validator\n";
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

	//! list holds the avg time taken by miner and validator
	//!  threads for multiple consecutive runs of the algorithm.
	list<float>mItrT;
	list<float>vItrT;
	int totalRejCont = 0; //number of validator rejected the blocks;
	int maxAccepted  = 0;
	int totalRun     = NumBlock; //at least 2
	FILEOPR file_opr;
	
	//! read from input file::
	int input[8] = {0};
	file_opr.getInp(input);
	nThread   = input[0]; // # of threads; should be one for serial;
	numAUs    = input[1]; // # of AUs or Transactions;
	CoinSObj  = input[2]; // # Coin Account Shared Object.
	nProposal = input[3]; // # Ballot Proposal Shared Object.
	nVoter    = input[4]; // # Ballot Voter Shared Object.
	nBidder   = input[5]; // # Auction Bidder Shared Object.
	bidEndT   = input[6]; // # Auction Bid End Time.
	lemda     = input[7];

	if(CoinSObj > maxCoinAObj) {
		CoinSObj = maxCoinAObj;
		cout<<"Max number of Coin Shared Object can be "<<maxCoinAObj<<"\n";
	}
	if(nProposal > maxPropObj) {
		nProposal = maxPropObj;
		cout<<"Max number of Proposal Shared Object can be "<<maxPropObj<<"\n";
	}	
	if(nVoter > maxVoterObj) {
		nVoter = maxVoterObj;
		cout<<"Max number of Voter Shared Object can be "<<maxVoterObj<<"\n";
	}
	if(nBidder > maxBidrObj) {
		nBidder = maxBidrObj;
		cout<<"Max number of Bid Shared Object can be "<<maxBidrObj<<"\n";
	}
	if(bidEndT > maxbEndT) {
		bidEndT = maxbEndT;
		cout<<"Max Bid End Time can be "<<maxbEndT<<"\n";
	}

	//used for state validation
	mCoinState      = new int [CoinSObj];
	vCoinState      = new int [CoinSObj];
	mBallotProState = new int [nProposal];
	vBallotProState = new int [nProposal];
	mBallotVotState = new int [nVoter];
	vBallotVotState = new int [nVoter];
	mPendingRet     = new int [nBidder+1];
	vPendingRet     = new int [nBidder+1];

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
			mm = new std::atomic<bool>;
			mm = false;
	 		//! generates AUs (i.e. trans to be executed by miner & validator).
			file_opr.genAUs(input, CFCount, BFCount, AFConut, listAUs);
			tTime[0]  = 0, tTime[1] = 0;
			Blockvalt = 0;

			//MINER
			Miner *miner = new Miner(0);
			miner ->mainMiner();

			if(lemda != 0) bool rv = addMFS(NumOfDoubleSTx);
	//		printBins();

			//VALIDATOR
				//Set Counter for malicious miner detection.
				coin->setCounterFlag(true);

				float valt      = 0;
				int acceptCount = 0, rejectCount = 0;
				for(int nval = 0; nval < numValidator; nval++)
				{
					valt = 0;
					for(int p = 0; p < nProposal; p++)vBallotProState[p]= 0;
					for(int v = 0; v < nVoter; v++)   vBallotVotState[v]= 0;
					for(int c = 0; c < CoinSObj; c++) vCoinState[c]     = 0;

					Validator *validator = new Validator();
					validator ->mainValidator();

					//State Validation
					bool flag = stateVal();
					if(flag == true) rejectCount++;
					else acceptCount++;
					mm = false;

					int counterv = 0, counterfv = 0;
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

			//! total valid AUs among List-AUs executed 
			//! by Miner & varified by Validator.
			int vAUs =  seqBin.size() + concBin.size();
			if(nBlock > 0)
				file_opr.writeOpt(input, nThread, numAUs, tTime, mTTime,
					              vTTime, aCount, vAUs, mItrT, vItrT, 1);

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


	cout<<"Avg Miner     Time in microseconds  = "
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
/*************************MAIN FUN CODE ENDS***********************************/
