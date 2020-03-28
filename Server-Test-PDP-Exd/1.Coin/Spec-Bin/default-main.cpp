#include <iostream>
#include <thread>
#include "Util/Timer.cpp"
#include "Contract/Coin.cpp"
#include "Util/FILEOPR.cpp"
#include <unistd.h>
#include <list>
#include <vector>
#include <atomic>
#include <condition_variable>


#define MAX_THREADS 128
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
double lemda;              //! % of edges to be removed from BG by malicious Miner.
double tTime[2];           //! total time taken by miner and validator algorithm.
Coin   *coin;              //! smart contract.
int    *aCount;            //! aborted transaction count.
float_t*mTTime;            //! time taken by each miner Thread to execute AUs (Transactions).
float_t*vTTime;            //! time taken by each validator Thread to execute AUs (Transactions).
vector<string>  listAUs;   //! holds AUs to be executed on smart contract: "listAUs" index+1 represents AU_ID.
vector<string>  seqBin;    //! holds sequential Bin AUs.
vector<string>  concBin;   //! holds concurrent Bin AUs.
std::atomic<int>currAU;    //! used by miner-thread to get index of Atomic Unit to execute.
std::atomic<int>vCount;    //! # of valid AU.
std::atomic<int>eAUCount;  //! used by validator threads to keep track of how many valid AUs executed by validator threads.
mutex concLock, seqLock;   //! Lock used to access seq and conc bin.
float seqTime[2];          //! Used to store seq exe time.
int MinerState[M_SharedObj];
int ValidatorState[M_SharedObj];




/*************************BARRIER CODE BEGINS**************************/
std::mutex mtx;
std::mutex pmtx; // to print in concurrent scene
std::condition_variable cv;
bool launch = false;

void wait_for_launch()
{
	std::unique_lock<std::mutex> lck(mtx);
	while (!launch) cv.wait(lck);
}

void shoot()
{
	std::unique_lock<std::mutex> lck(mtx);
	launch = true;
	cv.notify_all();
}
/*************************BARRIER CODE ENDS*****************************/





/*************************MINER CODE BEGINS*****************************/
/*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
!    Class "Miner" create & run "n" miner-thread concurrently           !
!"concMiner()" called by miner-thread to perfrom oprs of respective AUs !
! Thread 0 is considered as minter-thread (smart contract deployer)     !
!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
class Miner
{
	public:
	Miner(int minter_id) {
		vCount     = 0;
		currAU     = 0;
		mTTime     = new float_t [nThread];//! array index -> respective tid.
		aCount     = new int [nThread];
		for( int i = 0; i < nThread; i++ ) {
			mTTime[i] = 0;
			aCount[i] = 0;
		}
		//! id of the contract creater is "minter_id".
		coin = new Coin(SObj, minter_id);
	}


	//!---------------------------------------------------- 
	//!!!!!!!! MAIN MINER:: CREATE MINER THREADS !!!!!!!!!!
	//!----------------------------------------------------
	void mainMiner()
	{
		thread T[nThread];
		int bal = InitBalance;
		//! initialization of account with fixed
		//! ammount; mint() is assume to be serial.
		for(int sid = 1; sid <= SObj; sid++)
			coin->mint_m(0, sid, bal); //! 0 is contract deployer.

		//!---------------------------------------------------------
		//!!!!!!!!!!          Concurrent Phase            !!!!!!!!!!
		//!!!!!!!!!!    Create 'nThread' Miner threads    !!!!!!!!!!
		//!---------------------------------------------------------
		//! create "nThread" miner threads.
		for( int i = 0; i < nThread; i++ ) T[i] = thread(concMiner, i, numAUs);
		for( auto &th : T) th.join ( );

		//!------------------------------------------
		//!!!!!!!!!   Sequential Phase     !!!!!!!!!!
		//!------------------------------------------
//		coin->allUnlock();		
//		seqTime[0] = 0;
		Timer SeqTimer;
		auto start = SeqTimer._timeStart();
		seqBinExe();
		seqTime[0] += SeqTimer._timeStop( start );
//		coin->allUnlock();
		finalState(); //! print the final state of the shared objects.
	}


	//!------------------------------------------------------------------------
	//!!!!!!!!!!                 Concurrent Phase                    !!!!!!!!!!
	//! This function is executed by all the miner threads. The thread        !
	//! executes the transaction and adds them in SEQUENTIAL and CONCURRENT   !
	//! bin based on the return value of the AU function called by the thread.!
	//!------------------------------------------------------------------------
	static void concMiner(int t_ID, int numAUs)
	{
		Timer Ttimer;
		int  curInd = currAU++;
		auto start  = Ttimer._timeStart();
		while(curInd < numAUs)
		{
			//! get the AU to execute, which is of string type.
			istringstream ss(listAUs[curInd]);
			string tmp;
			ss >> tmp; //! AU_ID to Execute.
			int AU_ID = stoi(tmp);
			ss >> tmp; //! Function Name (smart contract).
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
				else if(v == false){
					//cout<<"Account "<<s_id<<" not found\n";
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
			curInd = currAU++;
		}
		mTTime[t_ID] = mTTime[t_ID] + Ttimer._timeStop( start );
	}


	//!------------------------------------------
	//!!!!!!!!!   Sequential Phase     !!!!!!!!!!
	//!------------------------------------------
	void seqBinExe( )
	{
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
			if(tmp.compare("get_bal") == 0) {
				ss >> tmp; //! get balance of SObj/id.
				int s_id = stoi(tmp);
				int bal  = 0;
				bool v = coin->get_bal_m(s_id, &bal, t_ID, false);
			}
			if(tmp.compare("send") == 0){
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
			count++;
		}
	}


	//!-------------------------------------------------
	//!FINAL STATE OF ALL THE SHARED OBJECT. Once all  |
	//!AUs executed. we are geting this using get_bel()|
	//!-------------------------------------------------
	void finalState()
	{
		for(int sid = 1; sid <= SObj; sid++) {
			int bal = 0;
			bool v = coin->get_bal_m(sid, &bal, 0, false);
			if(v != false) MinerState[sid] = bal;
		}
	}
	~Miner() { };
};
/*************************MINER CODE ENDS******************************/







/************************VALIDATOR CODE BEGINS**********************/
/*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
! Class "validator" create & run "n" validator-thread concurrently  !
! based on conc and seq bin given by miner operations of respective !
! AUs. Thread 0 is considered as minter-thread (contract deployer)  !
!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
class Validator
{
	public:
	Validator() {
		//! array index location represents respective thread id.
		eAUCount = 0;
		vTTime   = new float_t[nThread];
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
		int bal = InitBalance;
		thread T[nThread];

		//! initialization of account with fixed ammount;
		//! mint() function is assume to be serial.
		for(int sid = 1; sid <= SObj; sid++) 
			bool r = coin->mint(0, sid, bal); //! 0 is contract deployer.

		//!---------------------------------------------------------
		//!!!!!!!!!!          Concurrent Phase            !!!!!!!!!!
		//!!!!!!!!!!  Create 'nThread' Validator threads  !!!!!!!!!!
		//!---------------------------------------------------------
		for(int i = 0; i < nThread; i++)
			T[i] = thread(concValidator, i);
		shoot(); //notify all threads to begin the worker();
		for(auto& th : T) th.join ( );

		//!------------------------------------------
		//!!!!!!!!!   Sequential Phase     !!!!!!!!!!
		//!------------------------------------------
//		seqTime[1] = 0;
		Timer SeqTimer;
		auto start = SeqTimer._timeStart();
		seqBinExe();
		seqTime[1] += SeqTimer._timeStop( start );

		//! print the final state of the SObjs by validator.
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
			if(tmp.compare("get_bal") == 0) {
				ss >> tmp; //! get balance of SObj/id.
				int s_id = stoi(tmp);
				int bal  = 0;
				bool v = coin->get_bal(s_id, &bal);
			}
			if( tmp.compare("send") == 0 )
			{
				ss >> tmp; //! Sender ID.
				int s_id = stoi(tmp);
				ss >> tmp; //! Reciver ID.
				int r_id = stoi(tmp);
				ss >> tmp; //! Ammount to send.
				int amt  = stoi(tmp);
				bool v   = coin->send(s_id, r_id, amt);
			//	if(v == false)
			//		cout<<"\nConcBin:Sender don't have sufficent balance to send.\n";
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
		while(count < seqBin.size() )
		{
			//! get the AU to execute, which is of string type.
			istringstream ss(seqBin[count]);
			string tmp;
			ss >> tmp; //! AU_ID to Execute.
			int AU_ID = stoi(tmp);
			ss >> tmp; //! Function Name (smart contract).
			if(tmp.compare("get_bal") == 0) {
				ss >> tmp; //! get balance of SObj/id.
				int s_id = stoi(tmp);
				int bal  = 0;
				bool v = coin->get_bal(s_id, &bal);
			}
			if( tmp.compare("send") == 0 ) {
				ss >> tmp; //! Sender ID.
				int s_id = stoi(tmp);
				ss >> tmp; //! Reciver ID.
				int r_id = stoi(tmp);
				ss >> tmp; //! Ammount to send.
				int amt  = stoi(tmp);
				bool v   = coin->send(s_id, r_id, amt);
	//			if(v == false)
	//			cout<<"\nSeqBin:Sender don't have sufficent balance to send.\n";
			}
			count++;
		}
	}

	//!-------------------------------------------------
	//!FINAL STATE OF ALL THE SHARED OBJECT. Once all  |
	//!AUs executed. Geting this using get_bel()   |
	//!-------------------------------------------------
	void finalState()
	{
		for(int sid = 1; sid <= SObj; sid++) {
			int bal = 0;
			bool v  = coin->get_bal(sid, &bal);
			ValidatorState[sid] = bal;
		}
	}

	~Validator() { };
};
/*************************VALIDATOR CODE ENDS***************************/


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

	MinerState[s_id]  = 1000;
	MinerState[r_id]  = 1000;
	MinerState[r_id1] = 1000;
	amtAB = 950;
	trns1 = to_string(AU_ID1)+" send "+to_string(s_id)+" "
			+to_string(r_id)+" "+to_string(amtAB);
	listAUs[AU_ID1-1] =  trns1;
	amtAC = 100;
	trns2 = to_string(AU_ID2)+" send "+to_string(s_id)+" "
			+to_string(r_id1)+" "+to_string(amtAC);
	listAUs[AU_ID2-1] =  trns2;
	MinerState[s_id]  -= amtAB;
	MinerState[r_id]  += amtAB;
	MinerState[r_id1] += amtAC;
	
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




bool stateVal() {
	//State Validation
	bool flag = false;
//	cout<<"\n"<<pl<<"SObject \tMiner \t\tValidator"<<endl;
	for(int sid = 1; sid <= SObj; sid++) {
//		cout<<sid<<" \t \t"<<MinerState[sid]
//			<<" \t\t"<<ValidatorState[sid]<<endl;
		if(MinerState[sid] != ValidatorState[sid]) flag = true;
	}
	return flag;
}





/*************************MAIN FUN CODE BEGINS*********************************/
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

	//! list holds the avg time taken by miner and 
	//! Validator thread s for multiple consecutive runs.
	list<float>mItrT;
	list<float>vItrT;
	int totalRejCont = 0; //number of validator rejected the blocks;
	int maxAccepted  = 0;
	int totalRun     = NumBlock; //at least 2

	FILEOPR file_opr;

	//! read from input file:: SObj = #SObj; nThread = #threads;
	//! numAUs = #AUs; λ =  % of edges to be removed from BG by malicious Miner.
	file_opr.getInp(&SObj, &nThread, &numAUs, &lemda);
	//! max shared object error handling.
	if(SObj > M_SharedObj) {
		SObj = M_SharedObj;
		cout<<"Max number of Shared Object can be "<<M_SharedObj<<"\n";
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
			//! generates AUs (i.e. trans to be executed by miner & validator).
			file_opr.genAUs(numAUs, SObj, FUN_IN_CONT, listAUs);
			tTime[0]  = 0, tTime[1] = 0;
			Blockvalt = 0;

			Timer mTimer;
			mTimer.start();//Main timer

			//MINER
			Miner *miner = new Miner(0);
			miner ->mainMiner();

	//		printBins();
			if(lemda != 0) bool rv = addMFS(NumOfDoubleSTx);

			//VALIDATOR
				float valt      = 0;
				int acceptCount = 0, rejectCount = 0;
				for(int nval = 0; nval < numValidator; nval++)
				{
					valt = 0;
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
				for(int i = 1; i <= SObj; i++) ValidatorState[i] = 0;
	
			mTimer.stop();

			//total valid AUs among total AUs executed
			//by miner and varified by Validator.
			int vAUs = seqBin.size() + concBin.size();
			if(nBlock > 0)//skip first run
				file_opr.writeOpt(SObj, nThread, numAUs, tTime, mTTime,
					              vTTime, aCount, vAUs, mItrT, vItrT, 0);

			for(int i = 1; i <= SObj; i++) {
				MinerState[i]     = 0;
				ValidatorState[i] = 0;
			}
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
