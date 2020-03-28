#include <iostream>
#include <thread>
#include <list>
#include <atomic>
#include "Util/Timer.cpp"
#include "Contract/Ballot.cpp"
#include "Util/FILEOPR.cpp"

#define maxThreads 128
#define maxPObj 5000
#define maxVObj 40000
#define funInContract 5
#define pl "===================================================\n"
#define MValidation true   //! true or false
#define malMiner true      //! set the flag to make miner malicious.
#define NumOfDoubleSTx 2   //! # double-spending Tx for malicious final state by Miner, multiple of 2.

using namespace std;
using namespace std::chrono;

int NumBlock     = 26;     //! at least two blocks, the first run is warmup run.
int numValidator = 50;
int    nProposal = 2;      //! nProposal: number of proposal shared objects;
int    nVoter    = 1;      //! nVoter: number of voter shared objects;
int    nThread   = 1;      //! nThread: total number of concurrent threads; default is 1.
int    numAUs;             //! numAUs: total number of Atomic Unites to be executed.
double lemda;              //! λ: random delay seed.
double totalTime[2];       //! total time taken by miner and validator algorithm.
Ballot *ballot;            //! smart contract for miner.
Ballot *ballot_v;          //! smart contract for validator.
int    *aCount;            //! aborted transaction count.
float_t*mTTime;            //! time taken by each miner Thread to execute AUs (Transactions).
float_t*vTTime;            //! time taken by each validator Thread to execute AUs (Transactions).
vector<string>listAUs;     //! holds AUs to be executed on smart contract: "listAUs" index+1 represents AU_ID.
std::atomic<int>currAU;    //! used by miner-thread to get index of Atomic Unit to execute.
std::atomic<int>eAUCount;  //! used by validator threads to keep track of how many valid AUs executed by validator threads.

//State Data
int *mProposalState;
int *vProposalState;
int *mVoterState;
int *vVoterState;
string *proposalNames;


/*************************MINER CODE BEGINS***********************/
/*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
!    Class "Miner" CREATE & RUN "1" miner-THREAD                  !
!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
class Miner
{
	public:
	Miner(int chairperson)
	{
		//! initialize the counter used to execute the numAUs to
		//! 0, and graph node counter to 0 (number of AUs added
		//! in graph, invalid AUs will not be part of the grpah).
		currAU     = 0;
		//! index location represents respective thread id.
		mTTime = new float_t [nThread];
		aCount = new int [nThread];
		proposalNames = new string[nProposal+1];
		for(int x = 0; x <= nProposal; x++)
			proposalNames[x] = "X"+to_string(x+1);
		for(int i = 0; i < nThread; i++) {
			mTTime[i] = 0;
			aCount[i] = 0;
		}
		//! Id of the contract creater is \chairperson = 0\.
		ballot = new Ballot( proposalNames, chairperson, nVoter, nProposal);
	}

	//!----------------------------- 
	//!!!!!!!! MAIN MINER !!!!!!!!!!
	//!-----------------------------
	void mainMiner()
	{
		Timer mTimer;
		thread T[nThread];
		//! Give \`voter\` the right to vote on this ballot.
		//! giveRightToVote_m() is serial.
		for(int voter = 1; voter <= nVoter; voter++) {
			//! 0 is chairperson.
			ballot->giveRightToVote(0, voter);
		}

		//!---------------------------------------------------
		//!!!!!!!!!!    CREATE 1 MINER THREADS      !!!!!!!!!!
		//!---------------------------------------------------
		double start = mTimer.timeReq();
		for(int i = 0; i < nThread; i++) T[i] = thread(concMiner, i, numAUs);
		for(auto& th : T) th.join();
		totalTime[0] = mTimer.timeReq() - start;

		//! print the final state of the shared objects.
		finalState();
//		ballot->winningProposal_m();
//		string winner;
//		ballot->winnerName(&winner);
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
			if(tmp.compare("vote") == 0) {
				ss >> tmp;
				int vID = stoi(tmp);//! voter ID
				ss >> tmp;
				int pID = stoi(tmp);//! proposal ID
				int v = ballot->vote(vID, pID);
				if(v != 1) aCount[0]++;
			}
			if(tmp.compare("delegate") == 0) {
				ss >> tmp;
				int sID = stoi(tmp);//! Sender ID
				ss >> tmp;
				int rID = stoi(tmp);//! Reciver ID
				int v = ballot->delegate(sID, rID);
				if(v != 1) aCount[0]++;
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
		for(int id = 1; id <= nVoter; id++) 
			ballot->state(id, true, mVoterState);//for voter state
		for(int id = 1; id <= nProposal; id++) 
			ballot->state(id, false, mProposalState);//for Proposal state
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
		currAU     = 0;
		//! index location represents respective thread id.
		vTTime = new float_t [nThread];
		aCount = new int [nThread];
		for(int i = 0; i < nThread; i++) {
			vTTime[i] = 0;
			aCount[i] = 0;
		}
		//! Id of the contract creater is \chairperson = 0\.
		ballot_v = new Ballot( proposalNames, chairperson, nVoter, nProposal);
	}

	//!-------------------------------------------------------- 
	//!! MAIN Validator:: CREATE ONE VALIDATOR THREADS !!!!!!!!
	//!--------------------------------------------------------
	void mainValidator()
	{
		for(int i = 0; i < nThread; i++) {
			vTTime[i] = 0;
			aCount[i] = 0;
		}
		currAU = 0;
		Timer vTimer;
		thread T[nThread];
		//! giveRightToVote() function is serial.
		for(int voter = 1; voter <= nVoter; voter++) {
			//! 0 is chairperson.
			ballot_v->giveRightToVote(0, voter);
		}

		//!--------------------------------------
		//!!!!! CREATE ONE VALIDATOR THREAD  !!!!
		//!--------------------------------------
		double start = vTimer.timeReq();
		for(int i = 0; i<nThread; i++) T[i] = thread(concValidator, i);
		for(auto& th : T) th.join( );
		totalTime[1] = vTimer.timeReq() - start;

		//!print the final state of the shared objects by validator.
		finalState();
//		ballot->winningProposal();
//		string winner;
//		ballot->winnerName(&winner);
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
			if(tmp.compare("vote") == 0) {
				ss >> tmp;
				int vID = stoi(tmp);//! voter ID
				ss >> tmp;
				int pID = stoi(tmp);//! proposal ID
				int v = ballot_v->vote(vID, pID);
				if(v != 1) aCount[0]++;
			}
			if(tmp.compare("delegate") == 0) {
				ss >> tmp;
				int sID = stoi(tmp);//! Sender ID
				ss >> tmp;
				int rID = stoi(tmp);//! Reciver ID
				int v = ballot_v->delegate(sID, rID);
				if(v != 1) aCount[0]++;
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
		for(int id = 1; id <= nVoter; id++) 
			ballot_v->state(id, true, vVoterState);//for voter state
		for(int id = 1; id <= nProposal; id++) 
			ballot_v->state(id, false, vProposalState);//for Proposal state
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
//	cout<<"\n"<<pl<<"Proposal \tMiner \t\tValidator"<<endl;
	for(int pid = 0; pid < nProposal; pid++) {
//		cout<<pid+1<<" \t \t"<<mProposalState[pid]
//			<<" \t\t"<<vProposalState[pid]<<endl;
		if(mProposalState[pid] != vProposalState[pid])
			flag = true;
	}
//	cout<<"\n"<<pl<<"Voter ID \tMiner \t\tValidator"<<endl;
	for(int vid = 0; vid < nVoter; vid++) {
//		cout<<vid+1<<" \t \t"<<mVoterState[vid]
//			<<" \t\t"<<vVoterState[vid]<<endl;
		if(mVoterState[vid] != vVoterState[vid])
			flag = true;
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

	//! read from input file:: nProposal = #numProposal; nThread = #threads;
	//! numAUs = #AUs; λ = random delay seed.
	file_opr.getInp(&nProposal, &nVoter, &nThread, &numAUs, &lemda);


	if(nProposal > maxPObj) {
		nProposal = maxPObj;
		cout<<"Max number of Proposal Shared Object can be "<<maxPObj<<"\n";
	}
	if(nVoter > maxVObj) {
		nVoter = maxVObj;
		cout<<"Max number of Proposal Shared Object can be "<<maxVObj<<"\n";
	}

	mProposalState  = new int [nProposal];
	vProposalState  = new int [nProposal];
	mVoterState     = new int [nVoter];
	vVoterState     = new int [nVoter];

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
			file_opr.genAUs(numAUs, nVoter, nProposal, funInContract, listAUs);
			Timer mTimer;
			mTimer.start();

			//MINER
			Miner *miner = new Miner(0);
			miner ->mainMiner();

			//VALIDATOR
			int acceptCount = 0, rejectCount = 0;
			for(int nval = 0; nval < numValidator; nval++)
			{
				for(int p = 0; p < nProposal; p++) vProposalState[p] = 0;
				for(int v = 0; v < nVoter; v++) vVoterState[v] = 0;

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
			file_opr.writeOpt(nProposal, nVoter, nThread, numAUs, totalTime,
			                  mTTime, vTTime, aCount, vAUs, mItrT, vItrT);

	//		cout<<"\n\nc CPU time:  " << main_timer->cpu_ms_total() <<" ms \n";
	//		cout<<"c Real time: "    << main_timer->real_ms_total()<<" ms \n";
	//		cout<<"\n===================== Execution "<<numItr+1
	//			<<" Over =====================\n"<<endl;

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
