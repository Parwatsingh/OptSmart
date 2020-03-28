#include <iostream>
#include <thread>
#include <atomic>
#include <list>
#include "Util/Timer.cpp"
#include "Contract/Coin.cpp"
#include "Util/FILEOPR.cpp"

#define MAX_THREADS 1
#define M_SharedObj 5000
#define FUN_IN_CONT 3
#define pl "===================================================\n"
#define InitBalance 1000
#define MValidation true   //! true or false
#define malMiner true      //! set the flag to make miner malicious.
#define NumOfDoubleSTx 2   //! # double-spending Tx for malicious final state by Miner, multiple of 2.

using namespace std;
using namespace std::chrono;

int NumBlock     = 26;     //! at least two blocks, the first run is warmup run.
int numValidator = 50;

int    SObj    = 2;        //! SObj: number of shared objects; at least 2, to send & recive.
int    nThread = 1;        //! nThread: total number of concurrent threads; default is 1.
int    numAUs;             //! numAUs: total number of Atomic Unites to be executed.
double lemda;              //! λ:  % of edges to be removed from BG by malicious Miner.
double tTime[2];           //! total time taken by miner and validator algorithm.
Coin   *coin;              //! smart contract miner.
Coin   *coinV;              //! smart contract validator.
int    *aCount;            //! Invalid transaction count.
float_t*mTTime;            //! time taken by each miner Thread to execute AUs (Transactions).
float_t*vTTime;            //! time taken by each validator Thread to execute AUs (Transactions).
vector<string>listAUs;     //! holds AUs to be executed on smart contract: "listAUs" index+1 represents AU_ID.
std::atomic<int>currAU;    //! used by miner-thread to get index of Atomic Unit to execute.

//State Data
int *mState;
int *vState;


/*************************MINER CODE BEGINS***********************/
/*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
!    Class "Miner" CREATE & RUN "1" miner-THREAD                  !
!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
class Miner
{
	public:
	Miner(int minter_id)
	{
		//! initialize the counter.
		currAU = 0;
		//! index location represents respective thread id.
		mTTime = new float_t [nThread];
		aCount = new int [nThread];
		for(int i = 0; i < nThread; i++) {
			mTTime[i] = 0;
			aCount[i] = 0;
		}
		//! id of the contract creater is "minter_id".
		coin = new Coin( SObj, minter_id );
	}

	//!----------------------------- 
	//!!!!!!!! MAIN MINER !!!!!!!!!!
	//!-----------------------------
	void mainMiner()
	{
		Timer lTimer;
		thread T[nThread];
		//! initialization of account with fixed ammount;
		//! mint() function is serial.
		int bal = InitBalance, total = 0;
		for(int sid = 1; sid <= SObj; sid++) {
			//! 0 is contract deployer.
			bool v = coin->mint(0, sid, bal);
			total  = total + bal;
		}

		//!---------------------------------------------------
		//!!!!!!!!!!    CREATE 1 MINER THREADS      !!!!!!!!!!
		//!---------------------------------------------------
		double start = lTimer.timeReq();
		for(int i = 0; i < nThread; i++) T[i] = thread(concMiner, i, numAUs);
		for(auto& th : T) th.join();
		tTime[0] = lTimer.timeReq() - start;

		//! print the final state of the shared objects.
		finalState();
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
			if( tmp.compare("get_bal") == 0 ) {
				ss >> tmp;
				int s_id = stoi(tmp);
				int bal  = 0;
				//! get_bal() of smart contract.
				bool v = coin->get_bal(s_id, &bal);
			}
			if( tmp.compare("send") == 0 ) {
				ss >> tmp;
				int s_id = stoi(tmp);
				ss >> tmp;
				int r_id = stoi(tmp);
				ss >> tmp;
				int amt = stoi(tmp);
				bool v  = coin->send(s_id, r_id, amt);
				if(v == false) aCount[t_ID]++;
			}
			//! get the current index to execute, and increment it.
			curInd = currAU++;
		}
		mTTime[t_ID] +=  thTimer._timeStop(start);
	}

	//!------------------------------------------
	//!Final state of all the shared object.    |
	//! Once all AUs executed.                  |
	//!------------------------------------------
	void finalState() {
		for(int sid = 1; sid <= SObj; sid++) {
			int bal = 0;
			//! get_bal() of smart contract.
			bool v = coin->get_bal(sid, &bal);
			mState[sid] = bal;
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
	Validator(int minter_id)
	{
		//! initialize the counter to 0.
		currAU = 0;
		//! index location represents respective thread id.
		vTTime = new float_t [nThread];
		for(int i = 0; i < nThread; i++) vTTime[i] = 0;
		//! id of the contract creater is "minter_id".
		coinV = new Coin( SObj, minter_id );
	}

	//!-------------------------------------------------------- 
	//!! MAIN Validator:: CREATE ONE VALIDATOR THREADS !!!!!!!!
	//!--------------------------------------------------------
	void mainValidator()
	{
		Timer lTimer;
		thread T[nThread];
		//! initialization of account with fixed ammount;
		//! mint() function is serial.
		int bal = InitBalance, total = 0;
		for(int sid = 1; sid <= SObj; sid++) {
			//! 0 is contract deployer.
			bool v = coinV->mint(0, sid, bal);
			total  = total + bal;
		}

		//!-----------------------------------------------------
		//!!!!!!!!!!    CREATE 1 VALIDATOR THREAD       !!!!!!!!
		//!-----------------------------------------------------
		double start = lTimer.timeReq();
		for(int i = 0; i < nThread; i++) T[i] = thread(concValidator, i, numAUs);
		for(auto& th : T) th.join();
		tTime[1] = lTimer.timeReq() - start;
	
		//! print the final state of the shared objects.
		finalState();
	}

	//!------------------------------------------------------
	//! THE FUNCTION TO BE EXECUTED BY A VALIDATOR THREAD.  !
	//!------------------------------------------------------
	static void concValidator( int t_ID, int numAUs)
	{
		for(int i = 0; i < nThread; i++) vTTime[i] = 0;
		currAU = 0;
		Timer tTimer;
		//! get the current index, and increment it.
		int curInd = currAU++;

		//! statrt clock to get time taken by this.AU
		auto start = tTimer._timeStart();
		while( curInd < numAUs ) {
			//!get the AU to execute, which is of string type.
			istringstream ss(listAUs[curInd]);
			string tmp;
			ss >> tmp;//! AU_ID to Execute.
			int AU_ID = stoi(tmp);
			ss >> tmp;//! Function Name (smart contract).
			if( tmp.compare("get_bal") == 0 ) {
				ss >> tmp;//! get balance of SObj/id.
				int s_id = stoi(tmp);
				int bal  = 0;
				//! get_bal() of smart contract.
				bool v = coinV->get_bal(s_id, &bal);
			}
			if( tmp.compare("send") == 0 ) {
				ss >> tmp;
				int s_id = stoi(tmp);
				ss >> tmp;
				int r_id = stoi(tmp);
				ss >> tmp;
				int amt = stoi(tmp);
				bool v  = coinV->send(s_id, r_id, amt);
			}			
			//! get the current index to execute, and increment it.
			curInd = currAU++;
		}
		vTTime[t_ID] += tTimer._timeStop(start);
	}

	//!------------------------------------------
	//! Final state of all the shared object.   |
	//! Once all AUs executed.                  |
	//!------------------------------------------
	void finalState() {
		for(int sid = 1; sid <= SObj; sid++) {
			int bal = 0;
			//! get_bal() of smart contract.
			bool v = coinV->get_bal(sid, &bal);
			vState[sid] = bal;
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
//	cout<<"\n"<<pl<<"SObject \tMiner \t\tValidator"<<endl;
	for(int sid = 1; sid <= SObj; sid++) {
//		cout<<sid<<" \t \t"<<mState[sid]
//			<<" \t\t"<<vState[sid]<<endl;
		if(mState[sid] != vState[sid]) flag = true;
	}
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

	//! read from input file:: SObj = #SObj; nThread = #threads;
	//! numAUs = #AUs; λ =  % of edges to be removed from BG by malicious Miner.
	file_opr.getInp(&SObj, &nThread, &numAUs, &lemda);

	//!------------------------------------------------------------------
	//! Num of threads should be 1 for serial so we are fixing it to 1, !
	//! Whatever be # of threads in inputfile, it will be always one.   !
	//!------------------------------------------------------------------
	nThread = 1;
	if(SObj > M_SharedObj) {
		SObj = M_SharedObj;
		cout<<"Max number of Shared Object can be "<<M_SharedObj<<"\n";
	}

	mState = new int [SObj];
	vState = new int [SObj];

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
			file_opr.genAUs(numAUs, SObj, FUN_IN_CONT, listAUs);
			Timer mTimer;
			mTimer.start();

			//MINER
			Miner *miner = new Miner(0);
			miner ->mainMiner();

			//VALIDATOR
			int acceptCount = 0, rejectCount = 0;
			for(int nval = 0; nval < numValidator; nval++)
			{
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
			for(int i = 1; i <= SObj; i++) vState[i] = 0;
			mTimer.stop();

			//total valid AUs among List-AUs executed
			//by Miner & varified by Validator.
			int vAUs = numAUs - aCount[0];
			if(nBlock > 0)
			file_opr.writeOpt(SObj, nThread, numAUs, tTime, mTTime,
			                  vTTime, aCount, vAUs, mItrT, vItrT);

			for(int i = 1; i <= SObj; i++) {
				mState[i]     = 0;
				vState[i] = 0;
			}
			listAUs.clear();
			delete miner;
			miner = NULL;
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
