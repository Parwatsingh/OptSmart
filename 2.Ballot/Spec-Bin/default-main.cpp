#include <iostream>
#include <thread>
#include "Util/Timer.cpp"
#include "Contract/Ballot.cpp"
#include "Util/FILEOPR.cpp"
#include <unistd.h>
#include <list>
#include <vector>
#include <atomic>
#include <condition_variable>
#include <set>
#include <algorithm>


#define maxThreads 128
#define maxPObj 1000
#define maxVObj 40000
#define funInContract 5
#define pl "===================================================\n"
#define MValidation true  //! true or false
#define malMiner true     //! set the flag to make miner malicious.
#define NumOfDoubleSTx 2  //! # double-spending Tx for malicious final state by Miner, multiple of 2.

using namespace std;
using namespace std::chrono;

int NumBlock     = 26;    //! at least two blocks, the first run is warmup run.
int numValidator = 50;

int    nProposal = 2;     //! nProposal: number of proposal shared objects; default is 1.
int    nVoter    = 1;     //! nVoter: number of voter shared objects; default is 1.
int    nThread   = 1;     //! nThread: total number of concurrent threads; default is 1.
int    numAUs;            //! numAUs: total number of Atomic Unites to be executed.
double lemda;             //! λ: random delay seed.
float tTime[2];           //! total time taken by miner and validator algorithm.
Ballot *ballot;           //! smart contract.
int    *aCount;           //! aborted transaction count.
float_t *mTTime;          //! time taken by each miner Thread to execute AUs (Transactions).
float_t *vTTime;          //! time taken by each validator Thread to execute AUs (Transactions).
float_t *gTtime;          //! time taken by each miner Thread to add edges and nodes in the conflict graph.
vector<string>listAUs;    //! holds AUs to be executed on smart contract: "listAUs" index+1 represents AU_ID.
vector<string>seqBin;     //! holds sequential Bin AUs.
vector<string>concBin;    //! holds concurrent Bin AUs.
std::atomic<int>currAU;   //! used by miner-thread to get index of Atomic Unit to execute.
std::atomic<int>vCount;   //! # of valid AU node added in graph (invalid AUs will not be part of the graph & conflict list).
std::atomic<int>eAUCount; //! used by validator threads to keep track of how many valid AUs executed by validator threads.
mutex concLock, seqLock;  //! Lock used to access seq and conc bin.
float seqTime[2];         //! Used to store seq exe time.

//State Data
int *mPState;
int *vPState;
int *mVState;
int *vVState;
string *pNames;



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
	Miner(int chairperson)
	{
		//! initialize the counter used to execute the numAUs
		currAU = 0;
		vCount = 0;
		//! index location represents respective thread id.
		mTTime = new float_t [nThread];
		gTtime = new float_t [nThread];
		pNames = new string[nProposal+1];
		for(int x = 0; x <= nProposal; x++) 
			pNames[x]="X"+to_string(x+1);
		for(int i = 0; i < nThread; i++) {
			mTTime[i] = 0;
			gTtime[i] = 0;
		}
		//! Id of the contract creater is \chairperson = 0\.
		ballot = new Ballot( pNames, chairperson, nVoter, nProposal);
	}


	//!---------------------------------------------------- 
	//!!!!!!!! MAIN MINER:: CREATE MINER THREADS !!!!!!!!!!
	//!----------------------------------------------------
	void mainMiner()
	{
		Timer mTimer;
		thread T[nThread];
		ballot->reset_m();
		//! Give \`voter\` the right to vote on this ballot.
		//! giveRightToVote is serial.
		for(int voter = 1; voter <= nVoter; voter++)			
			ballot->giveRightToVote_m(0, voter);//! 0 is chairperson.

		//!---------------------------------------------------------
		//!!!!!!!!!!          Concurrent Phase            !!!!!!!!!!
		//!!!!!!!!!!    Create 'nThread' Miner threads    !!!!!!!!!!
		//!---------------------------------------------------------
		double s = mTimer.timeReq();
		for(int i = 0; i < nThread; i++) T[i] = thread(concMiner, i, numAUs);
		for(auto& th : T) th.join();//! miner thread join
		tTime[0] = mTimer.timeReq() - s;

		//!------------------------------------------
		//!!!!!!!!!   Sequential Phase     !!!!!!!!!!
		//!------------------------------------------
//		seqTime[0] = 0;
		Timer SeqTimer;
//		ballot->allUnlock();
		auto start = SeqTimer._timeStart();
		seqBinExe();
		seqTime[0] += SeqTimer._timeStop( start );
//		ballot->allUnlock();

		//! print the final state of the shared objects.
		finalState();
	}

	
	//!-----------------------------------------------------------------
	//!!!!!!!!!!               Concurrent Phase               !!!!!!!!!!
	//! The function to be executed by all the miner threads. Thread   !
	//! executes the transaction concurrently from Concurrent Bin      !
	//!-----------------------------------------------------------------
	static void concMiner( int t_ID, int numAUs)
	{
		Timer thTimer;
		//! get the current index, and increment it.
		int curInd = currAU++;
		auto start = thTimer._timeStart();
		while(curInd < numAUs)
		{
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
		mTTime[t_ID] += thTimer._timeStop(start);
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
			ss >> tmp;
			int AU_ID = stoi(tmp);
			ss >> tmp;
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


	//!-------------------------------------------------
	//!FINAL STATE OF ALL THE SHARED OBJECT. Once all  |
	//!AUs executed. we are geting this using state()  |
	//!-------------------------------------------------
	void finalState()
	{
		for(int id = 1; id <= nVoter; id++) 
			ballot->state_m(id, true, mVState);//for voter state

		for(int id = 1; id <= nProposal; id++) 
			ballot->state_m(id, false, mPState);//for Proposal state
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
		ballot->reset();
		//! giveRightToVote() function is serial. Id 0 is chairperson.
		for(int voter = 1; voter <= nVoter; voter++)
			ballot->giveRightToVote(0, voter);

		//!---------------------------------------------------------
		//!!!!!!!!!!          Concurrent Phase            !!!!!!!!!!
		//!!!!!!!!!!  Create 'nThread' Validator threads  !!!!!!!!!!
		//!---------------------------------------------------------
		double s = vTimer.timeReq();
		for(int i = 0; i<nThread; i++)	T[i] = thread(concValidator, i);
		shoot(); //notify all threads to begin the worker();
		for(auto& th : T) th.join( );
		tTime[1] = vTimer.timeReq() - s;

		//!------------------------------------------
		//!!!!!!!!!   Sequential Phase     !!!!!!!!!!
		//!------------------------------------------
//		seqTime[1] = 0;
		Timer SeqTimer;
		//! start timer to get time taken by this thread.
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
	static void concValidator( int t_ID )
	{
		//barrier to synchronise all threads for a coherent launch
		wait_for_launch();
		Timer Ttimer;
		auto start = Ttimer._timeStart();
		int curInd = eAUCount++;
		while( curInd < concBin.size() )
		{
			//! get the AU to execute, which is of string type.
			istringstream ss(concBin[curInd]);
			string tmp;
			ss >> tmp; //! AU_ID to Execute.
			int AU_ID = stoi(tmp);
			ss >> tmp; //! Function Name (smart contract).
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
		vTTime[t_ID] += Ttimer._timeStop( start );
	}


	//!------------------------------------------
	//!!!!!!!!!   Sequential Phase     !!!!!!!!!!
	//!------------------------------------------
	void seqBinExe( )
	{
//		cout<<"\nSequential Bin Execution::\n";
		int t_ID = 0;
		int count = 0;
		while(count < seqBin.size())
		{
			//! get the AU to execute, which is of string type.
			istringstream ss(seqBin[count]);
			string tmp;
			ss >> tmp; //! AU_ID to Execute.
			int AU_ID = stoi(tmp);
			ss >> tmp; //! Function Name (smart contract).
			if(tmp.compare("vote") == 0) {
				ss >> tmp;
				int vID = stoi(tmp);//! voter ID
				ss >> tmp;
				int pID = stoi(tmp);//! proposal ID
				int PID = -pID; 
				int v = ballot->vote(vID, PID);
//				if(v == 0) cout<<"Error!!! Invalid AU\n";
			}
			if(tmp.compare("delegate") == 0) {
				ss >> tmp;
				int sID = stoi(tmp);//! Sender ID
				ss >> tmp;
				int rID = stoi(tmp);//! Reciver ID
				int v = ballot->delegate(sID, rID);
//				if(v == 0) cout<<"Error!!! Invalid AU\n";
			}
			count++;
		}
	}

	//!-------------------------------------------------
	//!FINAL STATE OF ALL THE SHARED OBJECT. Once all  |
	//!AUs executed. we are geting this using state()  |
	//!-------------------------------------------------
	void finalState()
	{
		for(int id = 1; id <= nVoter; id++) 
			ballot->state(id, true, vVState);//for voter state

		for(int id = 1; id <= nProposal; id++) 
			ballot->state(id, false, vPState);//for Proposal state
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
bool addMFS(int atPoss)
{
	istringstream ss(listAUs[atPoss-2]);
	string trns1;
	ss >> trns1; //! AU_ID to Execute.
	int AU_ID1 = stoi(trns1);
	ss >> trns1;//function name
	ss >> trns1; //! Voter ID.
	int s_id = stoi(trns1);
	ss >> trns1; //! Proposal ID.
	int r_id = - stoi(trns1);

	istringstream ss1(listAUs[atPoss-1]);
	string trns2;
	ss1 >> trns2; //! AU_ID to Execute.
	int AU_ID2 = stoi(trns2);
	ss1 >> trns2;//function name
	ss1 >> trns2; //! Voter ID.
	int s_id1 = stoi(trns2);
	ss1 >> trns2; //! Proposal ID.
	int r_id1 = - stoi(trns2);

	mPState[r_id-1]  = 1;
	mPState[r_id1-1] = 1;
	mVState[s_id1-1]    = 1;


	trns1 = listAUs[atPoss-2];
	trns2 = listAUs[atPoss-1];

	//! add the confliciting AUs in conc bin and remove them from seq bin.
	//! Add one of the AU from seq bin to conc Bin and remove that AU from seq bin.
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
	//State Validation
	bool flag = false;
//	cout<<"\n"<<pl<<"Proposal \tMiner \t\tValidator"<<endl;
	for(int pid = 0; pid < nProposal; pid++) {
//		cout<<pid+1<<" \t \t"<<mPState[pid]
//			<<" \t\t"<<vPState[pid]<<endl;
		if(mPState[pid] != vPState[pid])
			flag = true;
	}
//	cout<<"\n"<<pl<<"Voter ID \tMiner \t\tValidator"<<endl;
	for(int vid = 0; vid < nVoter; vid++) {
//		cout<<vid+1<<" \t \t"<<mVState[vid]
//			<<" \t\t"<<vVState[vid]<<endl;
		if(mVState[vid] != vVState[vid])
			flag = true;
	}
	return flag;
}





/**********************MAIN FUN CODE BEGINS***************************/
/*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
/*!!!!!!!!          main()         !!!!!!!!!!*/
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

	//! list holds the avg time taken by miner and Validator
	//! thread s for multiple consecutive runs.
	list<float>mItrT;
	list<float>vItrT;
	int totalRejCont = 0; //number of validator rejected the blocks;
	int mWiningPro   = 0;
	int vWiningPro   = 0;
	int maxAccepted  = 0;
	int totalRun     = NumBlock; //at least 2

	FILEOPR file_opr;

	//! read from input file:: nProposal = #numProposal; nThread = #threads;
	//! numAUs = #AUs; λ = random delay seed.
	file_opr.getInp(&nProposal, &nVoter, &nThread, &numAUs, &lemda);
	if(nProposal > maxPObj) {
		nProposal = maxPObj;
		cout<<"Max number of Proposals can be "<<maxPObj<<"\n";
	}
	if(nVoter > maxVObj) {
		nVoter = maxVObj;
		cout<<"Max number of Voters can be "<<maxVObj<<"\n";
	}

	mPState   = new int [nProposal];
	vPState   = new int [nProposal];
	mVState   = new int [nVoter];
	vVState   = new int [nVoter];

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
			//! generates AUs (i.e. trans to be executed by miner & validator).
			file_opr.genAUs(numAUs, nVoter, nProposal, funInContract, listAUs);
			tTime[0]  = 0, tTime[1] = 0;
			Blockvalt = 0;

			for(int pid = 0; pid < nProposal; pid++) mPState[pid] = 0;
			for(int vid = 0; vid < nVoter; vid++) mVState[vid] = 0;

			Timer mTimer;
			mTimer.start();
			//MINER
			Miner *miner = new Miner(0);//0 is contract deployer id
			miner ->mainMiner();

			//! Add malicious AUs.
			if(lemda != 0) bool rv = addMFS(NumOfDoubleSTx);

	//		printBins();

			//VALIDATOR
				float valt      = 0;
				int acceptCount = 0, rejectCount = 0;
				for(int nval = 0; nval < numValidator; nval++)
				{
					valt = 0;
					for(int p = 0; p < nProposal; p++) vPState[p] = 0;
					for(int v = 0; v < nVoter; v++) vVState[v] = 0;

					Validator *validator = new Validator();
					validator ->mainValidator();

					//State Validation
					bool flag = stateVal();
					if(flag == true) rejectCount++;
					else acceptCount++;

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

			mTimer.stop();

			//total valid AUs among total AUs executed
			//by miner and varified by Validator.
			int vAUs = seqBin.size() + concBin.size();
			if(nBlock > 0)
			file_opr.writeOpt(nProposal, nVoter, nThread, numAUs, tTime, 
			                  mTTime, vTTime, aCount, vAUs, mItrT, vItrT, 0);

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
