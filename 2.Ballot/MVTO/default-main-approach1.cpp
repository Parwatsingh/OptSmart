#include <iostream>
#include <thread>
#include "Util/Timer.cpp"
#include "Contract/Ballot.cpp"
#include "Graph/Lockfree/Graph.cpp"
#include "Util/FILEOPR.cpp"

#define maxThreads 128
#define maxPObj 1000
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

float tTime[3];            //! total time taken by miner and validator algorithm.
Ballot *ballot;            //! smart contract.
Graph  *cGraph;            //! conflict grpah generated by miner to be given to validator.
int    *aCount;            //! aborted transaction count.
float_t*mTTime;            //! time taken by each miner Thread to execute AUs (Transactions).
float_t*vTTime;            //! time taken by each validator Thread to execute AUs (Transactions).
float_t*fvTTime;           //! time taken by each validator Thread to execute AUs (Transactions).
float_t *gTtime;           //! time taken by each miner Thread to add edges and nodes in the conflict graph.
vector<string>listAUs;     //! holds AUs to be executed on smart contract: "listAUs" index+1 represents AU_ID.
vector<int>concBin;        //! holds AU_IDs to be executed on smart contract concurrently in first phase.
std::atomic<int>currAU;    //! used by miner-thread to get index of Atomic Unit to execute.
std::atomic<int>gNodeCount;//! # of valid AU node added in graph (invalid AUs will not be part of the graph & conflict list).
std::atomic<int>eAUCount;  //! used by validator threads to keep track of how many valid AUs executed by validator threads.
std::atomic<int>*status;   //! used by pool threads:: -1 = thread join; 0 = wait; 1 = execute AUs given in ref[].
Graph::Graph_Node **Gref;  //! used by pool threads:: graph node (AU) reference to be execute by respective Pool thread.
std::atomic<int>*mAUT;     //! array to map AUs to Trans id (time stamp); mAUT[index] = TransID, index+1 = AU_ID.
Graph  *nValBG;            //! used to store graph of respective n validators.

//State Data
int *mPState;
int *vPState;
int *fvPState;
int *mVState;
int *vVState;
int *fvVState;
string *pNames;


std::mutex cb;


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
! Thread 0 is considered as minter-thread (smart contract deployer)     !
!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
class Miner
{
	public:
	Miner(int chairperson)
	{
		cGraph = new Graph();
		//! initialize the counter used to execute the numAUs to
		//! 0, and graph node counter to 0 (number of AUs added
		//! in graph, invalid AUs will not be part of the grpah).
		currAU     = 0;
		gNodeCount = 0;

		//! index location represents respective thread id.
		mTTime = new float_t [nThread];
		gTtime = new float_t [nThread];
		aCount = new int [nThread];
		
		pNames = new string[nProposal+1];
		for(int x = 0; x <= nProposal; x++) {
			pNames[x] = "X"+to_string(x+1);
		}
		
		for(int i = 0; i < nThread; i++) {
			mTTime[i] = 0;
			gTtime[i] = 0;
			aCount[i] = 0;
		}
		
		//! Id of the contract creater is \chairperson = 0\.
		ballot = new Ballot( pNames, chairperson, nVoter, nProposal);
	}


	//!------------------------------------------------------------------------- 
	//!!!!!!! MAIN MINER:: CREATE (MINER + GRAPH CONSTRUCTION) THREADS !!!!!!!!!
	//!-------------------------------------------------------------------------
	void mainMiner()
	{
		Timer mTimer;
		thread T[nThread];

		//! Give \`voter\` the right to vote on this ballot.
		//! giveRightToVote_m() is serial.
		for(int voter = 1; voter <= nVoter; voter++) {
			//! 0 is chairperson.
			ballot->giveRightToVote_m(0, voter);
		}

		//!!!!!!!!!!    Create nThread Miner threads      !!!!!!!!!!
		double start = mTimer.timeReq();
		for(int i = 0; i < nThread; i++)
			T[i] = thread(concMiner, i, numAUs, cGraph);
		for(auto& th : T) th.join();//! miner thread join
		tTime[0] = mTimer.timeReq() - start;

		//! print conflict grpah.
//		cGraph->print_BG(cGraph);

		//! print the final state of the shared objects.
		finalState();
//		ballot->winningProposal_m();
//		string winner;
//		ballot->winnerName_m(&winner);
	}

	//!--------------------------------------------------------
	//! The function to be executed by all the miner threads. !
	//!--------------------------------------------------------
	static void concMiner( int t_ID, int numAUs, Graph *cGraph)
	{
		Timer thTimer;

		//! flag is used to add valid AUs in Graph.
		//! (invalid AU: senders doesn't have
		//! sufficient balance to send).
		bool flag = true;

		//! get the current index, and increment it.
		int curInd = currAU++;

		//! statrt clock to get time taken by this.AU
		auto start = thTimer._timeStart();
		
		while(curInd < numAUs)
		{
			//! trns_id of STM_BTO_trans that 
			//! successfully executed this AU.
			int t_stamp;

			//! trans_ids with which
			//! this AU.trans_id is conflicting.
			list<int>conf_list;
			conf_list.clear();
			//! get the AU to execute,
			//! which is of string type.
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
				int v = ballot->vote_m(vID, pID, &t_stamp, conf_list);
				while( v != 1 ) {
					aCount[t_ID]++;
					v = ballot->vote_m(vID, pID, &t_stamp, conf_list);					
					if(v == -1) {
						//! invalid AU
						cb.lock();
//						concBin.erase(remove(concBin.begin(), concBin.end(), AU_ID), concBin.end());
						remove(concBin.begin(), concBin.end(), AU_ID);
						cb.unlock();
						flag = false;
						break;                                    
					}
				}
			}
			if(tmp.compare("delegate") == 0) {
				ss >> tmp;
				int sID = stoi(tmp);//! Sender ID
				ss >> tmp;
				int rID = stoi(tmp);//! Reciver ID
				//! execute again if tryCommit fails
				int v = ballot->delegate_m(sID, rID, &t_stamp, conf_list);	
				while( v != 1 ){
					aCount[t_ID]++;
					v = ballot->delegate_m(sID, rID, &t_stamp, conf_list);
					if(v == -1) {
						//! invalid AU:
						cb.lock();
//						concBin.erase(remove(concBin.begin(), concBin.end(), AU_ID), concBin.end());
						remove(concBin.begin(), concBin.end(), AU_ID);
						cb.unlock();
						flag = false;
						break;                                    
					}
				}
			}
			//! graph construction for committed AUs.
			if (flag == true) {
				mAUT[AU_ID-1] = t_stamp;
				
				//! increase graph node 
				//! counter (Valid AU executed)
				gNodeCount++;
				
				//! get respective trans conflict list using lib fun
				//list<int>conf_list = lib->get_conf(t_stamp);

				//!::::::::::::::::::::::::::::::::::
				//! Remove all the time stamps from :
				//! conflict list, added because of :
				//! initilization and creation of   :
				//! shared object in STM memory.    :
				//!::::::::::::::::::::::::::::::::::
				for(int y = 1; y <= (2*nVoter+nProposal+1); y++)
					conf_list.remove(y);
				
				//! statrt clock to get time taken by this.thread 
				//! to add edges and node to conflict grpah.
				auto gstart = thTimer._timeStart();

				//!------------------------------------------
				//! conf_list come from contract fun using  !
				//! pass by argument of get_bel() and send()!
				//!------------------------------------------
				//! when AU_ID conflict is empty.
				if(conf_list.begin() == conf_list.end()) {
//					Graph:: Graph_Node *tempRef;
//					cGraph->add_node(AU_ID, t_stamp, &tempRef);
				}
				else
				{
					cb.lock();
//					concBin.erase(remove(concBin.begin(), concBin.end(), AU_ID), concBin.end());
					remove(concBin.begin(), concBin.end(), AU_ID);
					cb.unlock();
					for(auto it = conf_list.begin(); it != conf_list.end(); it++) {
						int i = 0;
						//! get conf AU_ID in map table
						//! given conflicting tStamp.
						while(*it != mAUT[i]) i = (i+1)%numAUs;

						//! index start with 
						//! 0 => index+1 respresent AU_ID.
						//! cAUID = index+1, 
						//! cTstamp = mAUT[i] with this.AU_ID
						int cAUID   = i+1;
						int cTstamp = mAUT[i];

						if(cTstamp < t_stamp)//! edge from cAUID to AU_ID.
							cGraph->add_edge(cAUID, AU_ID, cTstamp, t_stamp);

						if(cTstamp > t_stamp) //! edge from AU_ID to cAUID.
							cGraph->add_edge(AU_ID, cAUID, t_stamp, cTstamp);

						cb.lock();
//						concBin.erase(remove(concBin.begin(), concBin.end(), cAUID), concBin.end());
						remove(concBin.begin(), concBin.end(), cAUID);
						cb.unlock();
					}
				}
				gTtime[t_ID] += thTimer._timeStop(gstart);
			}
			//! reset flag for next AU.
			flag = true;
			//! get the current index to execute, and increment it.
			curInd = currAU++;
			conf_list.clear();
		}
		mTTime[t_ID] += thTimer._timeStop(start);
	}

	//!-------------------------------------------------
	//!FINAL STATE OF ALL THE SHARED OBJECT. Once all  |
	//!AUs executed. we are geting this using state_m()|
	//!-------------------------------------------------
	void finalState() {
		for(int id = 1; id <= nVoter; id++) 
			ballot->state_m(id, true, mVState);//for voter state

		for(int id = 1; id <= nProposal; id++) 
			ballot->state_m(id, false, mPState);//for Proposal state
	}
	~Miner() { };
};
/********************MINER CODE ENDS*********************************/




/*************************Dec.CODE BEGINS****************************/
/*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
! Class "Validator" Create & run "n" validator-thread concurrently   !
! based on conflict grpah! given by miner. concValidator() called    !
! by validator-thread to perfrom operations of respective AUs.       !
! Thread 0 is considered as minter-thread (smart contract deployer). !
!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
class Validator
{
	public:
	Validator() {
		//! int the execution counter used by validator threads.
		eAUCount = 0;
		//! array index location represents respective thread id.
		vTTime = new float_t [nThread];
		for(int i = 0; i < nThread; i++) vTTime[i] = 0;
	};


	/*!---------------------------------------
	| create n concurrent validator threads  |
	| to execute valid AUs in conflict graph.|
	----------------------------------------*/
	void mainValidator()
	{
		eAUCount = 0;
		ballot->reset();

		Timer vTimer;
		thread cT[nThread];
		thread T[nThread];

		//! giveRightToVote() function is serial.
		for(int voter = 1; voter <= nVoter; voter++)  //! 0 is chairperson.
			ballot->giveRightToVote(0, voter);

		//!Create "nThread" threads for Phase-1: ConcBin Phase
		double start = vTimer.timeReq();
		for(int i = 0; i < nThread; i++) cT[i] = thread(concBinValidator, i);
		shoot(); //notify all threads to begin;
		for(auto& th : cT) th.join ( );

		eAUCount = concBin.size()-1;
		//!Create "nThread" threads for Phase-2: BG Phase
		for(int i = 0; i < nThread; i++) T[i] = thread(concValidator, i);
		shoot(); //notify all threads to begin;
		for(auto& th : T) th.join ( );
		tTime[1] += vTimer.timeReq() - start;

		//!print the final state of the shared objects by validator.
		finalState();
//		ballot->winningProposal();
//		string winner;
//		ballot->winnerName(&winner);
	}




	//!------------------------------------------------------------
	//!!!!!!!     Concurrent Phase-1: ConBin Phase         !!!!!!!!
	//!!!!!!  'nThread' Validator threads Executes this Fun !!!!!!!
	//!------------------------------------------------------------
	static void concBinValidator( int t_ID )
	{
		//barrier to synchronise all threads for a coherent launch
		wait_for_launch();
		Timer Ttimer;
		auto start = Ttimer._timeStart();
		int curInd = eAUCount++;
		while( curInd < concBin.size() )
		{
			//! get the AU to execute, which is of string type.
			int AU_id_t = concBin[curInd];

//			string str = "\nThread "+to_string(t_ID)+" executing AU "+listAUs[AU_id_t];
//			cout<<str<<endl;

			istringstream ss(listAUs[AU_id_t-1]);
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



	//!------------------------------------------------------------
	//!!!!!!!      Concurrent Phase-2: BG Phase            !!!!!!!!
	//!!!!!!  'nThread' Validator threads Executes this Fun !!!!!!!
	//!------------------------------------------------------------
	static void concValidator( int t_ID ) {
		//barrier to synchronise all threads for a coherent launch.
		wait_for_launch();
		Timer thTimer;
		//!statrt clock to get time taken by this thread.
		auto start = thTimer._timeStart();
		list<Graph::Graph_Node*>buffer;
		auto itr = buffer.begin();
		Graph:: Graph_Node *verTemp;
		while( true )
		{
			//!uncomment this to remove the effect of local buffer optimization.
			//buffer.clear();

			//! all Graph Nodes (Valid AUs executed)
			if(eAUCount == gNodeCount ) break;
			//!-----------------------------------------
			//!!!<< AU execution from local buffer. >>!!
			//!-----------------------------------------
			for(itr = buffer.begin(); itr != buffer.end(); itr++)
			{
				Graph::Graph_Node* temp = *itr;
				if(temp->in_count == 0)
				{
					//! expected in_degree is 0 then vertex can be executed,
					//! if not claimed by other thread.
					int expected = 0;
					if(atomic_compare_exchange_strong(
								&(temp->in_count), &expected, -1 ) == true)
					{
						//! num of Valid AUs executed is eAUCount+1.
						eAUCount++;

						//! get AU to execute, which is of string type;
						//! listAUs index statrt with 0 ==> -1.
						istringstream ss( listAUs[(temp->AU_ID) - 1]);
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
						}
						if(tmp.compare("delegate") == 0) {
							ss >> tmp;
							int sID = stoi(tmp);//! Sender ID
							ss >> tmp;
							int rID = stoi(tmp);//! Reciver ID
							int v = ballot->delegate(sID, rID);
						}
						//!-----------------------------------------
						//!change indegree of out edge nodes (node !
						//! having incomming edge from this node). !
						//!-----------------------------------------						
						Graph::EdgeNode *eTemp = temp->edgeHead->next;
						while( eTemp != temp->edgeTail)
						{
							Graph::Graph_Node* refVN =
										(Graph::Graph_Node*)eTemp->ref;

							refVN->in_count--;
							if(refVN->in_count == 0 )//!insert into local buffer.
								buffer.push_back(refVN);
							eTemp = eTemp->next;
						}
						delete eTemp;
					}
				}
			}
			//! reached to end of local buffer; clear the buffer.
			buffer.clear();
			//!-----------------------------------------------------
			//!!!<< AU execution by traversing conflict grpah  >>!!!
			//!-----------------------------------------------------
			verTemp = nValBG->verHead->next;
			while(verTemp != nValBG->verTail)
			{
				if(verTemp->in_count == 0)
				{
					//! expected in_degree is 0 then vertex can be executed,
					//! if not claimed by other thread.
					int expected = 0;
					if(atomic_compare_exchange_strong( 
								&(verTemp->in_count), &expected, -1 ) == true)
					{
						//! num of Valid AUs executed is eAUCount+1.
						eAUCount++;
						//! get AU to execute, which is of string type;
						//! listAUs index statrt with 0 => -1.
						istringstream ss(listAUs[(verTemp->AU_ID)-1]);
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
						}
						if(tmp.compare("delegate") == 0) {
							ss >> tmp;
							int sID = stoi(tmp);//! Sender ID
							ss >> tmp;
							int rID = stoi(tmp);//! Reciver ID
							int v = ballot->delegate(sID, rID);
						}
						//!-----------------------------------------
						//!change indegree of out edge nodes (node !
						//! having incomming edge from this node). !
						//!-----------------------------------------
						Graph::EdgeNode *eTemp = verTemp->edgeHead->next;
						while( eTemp != verTemp->edgeTail)
						{
							Graph::Graph_Node* refVN = 
												(Graph::Graph_Node*)eTemp->ref;
							refVN->in_count--;
							if(refVN->in_count==0) //!insert into local buffer.
								buffer.push_back( refVN );
							eTemp = eTemp->next;
						}
					}
				}
				verTemp = verTemp->next;
			}
		}
		buffer.clear();
		//!stop timer to get time taken by this thread
		vTTime[t_ID] += thTimer._timeStop(start);
	}

	//!-------------------------------------------------
	//!Final state of all the shared object. Once all  |
	//!AUs executed. We are geting this using state()  |
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
/*************************  Dec. CODE ENDS  **************************/



/************************ FORK CODE BEGINS **************************/
/*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
! Class "Validator" create 1 master thread and & "n" slave validator !
! thread to execute AUs concurrently based on conflict grpah given   !
! by miner. "concValidator()" called by master-thread to creates     !
! "N" slave Thread to perfrom operations of respective AUs.          !
!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
class ForkValidator
{
	public:
	ForkValidator()
	{
		//! int the execution counter used by validator threads.
		eAUCount = 0;
		status = new std::atomic<int>[nThread+1];
		Gref   = new Graph::Graph_Node*[nThread+1];		
		//! array index location represents respective thread id.
		fvTTime = new float_t [nThread+1];
		int i = 0;
		for( ; i < nThread; i++) {
			status[i] = 0;
			Gref[i]   = NULL;
			fvTTime[i] = 0;	
		}
		fvTTime[i] = 0;
	};


	//!-------------------------------------------------
	//! CREATE MASTER THREADS: CREATES n WORKER        !
	//! THREADs TO EXECUTE VALID AUs IN CONFLICT GRAPH.!
	//!-------------------------------------------------
	void mainValidator()
	{
		int i = 0;
		for( ; i < nThread; i++) {
			status[i] = 0;
			Gref[i]   = NULL;
		}
		eAUCount  = 0;
		Timer vTimer;
		thread T[nThread];
		thread cT[nThread];
		ballot->reset();
	
		//! giveRightToVote() function is serial.
		for(int voter = 1; voter <= nVoter; voter++) //! 0 is chairperson.
			ballot->giveRightToVote(0, voter);

		//!Create "nThread" threads for Phase-1: ConcBin Phase
		double start = vTimer.timeReq();
		for(int i = 1; i <= nThread; i++) cT[i-1] = thread(concBinValidator, i);
		shoot(); //notify all threads to begin;
		for(auto& th : cT) th.join ( );

		eAUCount = concBin.size()-1;
		//! Master thread create n validator threads for Phase-2: BG Phase
		thread master = thread(concValidator, 0 );
		master.join();
		tTime[2] += vTimer.timeReq() - start;

		//!print the final state of the shared objects by validator.
		finalState();
//		ballot->winningProposal();
//		string winner;
//		ballot->winnerName(&winner);
	}

	//!------------------------------------------------------------
	//!!!!!!!     Concurrent Phase-1: ConBin Phase         !!!!!!!!
	//!!!!!!  'nThread' Validator threads Executes this Fun !!!!!!!
	//!------------------------------------------------------------
	static void concBinValidator( int t_ID )
	{
		//barrier to synchronise all threads for a coherent launch
		wait_for_launch();
		Timer Ttimer;
		auto start = Ttimer._timeStart();
		int curInd = eAUCount++;
		while( curInd < concBin.size() )
		{
			//! get the AU to execute, which is of string type.
			int AU_id_t = concBin[curInd];

//			string str = "\nThread "+to_string(t_ID)+" executing AU "+listAUs[AU_id_t];
//			cout<<str<<endl;

			istringstream ss(listAUs[AU_id_t-1]);
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
		fvTTime[t_ID] += Ttimer._timeStop( start );
	}





	//!-------------------------------------------------------------
	//!!!             Concurrent Phase-2: BG Phase                !!!
	//!!  'nThread' Worker and a Master threads Executes this Fun  !!
	//!--------------------------------------------------------------
	static void concValidator( int t_ID )
	{
		Timer thTimer;
		//!statrt clock to get time taken by this thread.
		auto start = thTimer._timeStart();
		//! ONLY MASTER THREAD WILL EXECUTE IT.
		if(t_ID == 0)
		{
			thread POOL[nThread+1];
			bool tCratFlag = true;//! POOL thread creation flag.
			Graph::Graph_Node *mVItr;
			while(true) {
				if(tCratFlag == true) {
					//! Creating n POOL Threads
					for(int i = 1; i <= nThread; i++) {
						POOL[i] = thread(concValidator, i);
					}
					tCratFlag = false;
				}
				//! All Valid AUs executed.
				if(eAUCount == gNodeCount) {
					for(int i = 1; i <= nThread; i++) {
						//! -1 = threads can join now.
						status[i] = -1;
					}
					
					//! POOL thread join.
					for(int i = 1; i <= nThread; i++) {
						POOL[i].join( );
					}
					break;
				}
				mVItr = nValBG->verHead->next;
				while(mVItr != nValBG->verTail) {
					if(mVItr->in_count == 0) {
						for(int i = 1; i <= nThread; i++) {
							//! 0 = thread is available.
							if(status[i] == 0) {
								//! assigning node ref for
								//! thread in pool to execute.
								Gref[i] = mVItr;
								//! 1 = ref is available to execute.
								status[i] = 1;
								break;
							}
						}
					}
					mVItr = mVItr->next;
				}
			}
		}
		//! EXECCUTED BY nThread WORKER THREADS.
		else
		{
			while(true)
			{
				if(status[t_ID] == -1 || eAUCount == gNodeCount) 
					break;//! All task done.
				
				if(status[t_ID] == 1)//! Task available to work on.
				{
					Graph::Graph_Node *verTemp;
					verTemp      = Gref[t_ID];
					if(verTemp->in_count == 0) {
						if(verTemp->in_count < 0) {
							status[t_ID] = 0;
						}
						else
						{
							//! expected in_degree is 0 then
							//! vertex can be executed if 
							//! not claimed by other thread.
							int expected = 0;
							if(atomic_compare_exchange_strong( 
								&(verTemp->in_count), &expected, -1 ) == true)
							{
								verTemp->in_count = -1;
								//! get AU to execute, which is of string type;
								//! listAUs index statrt with 0 => -1.
								istringstream ss(listAUs[(verTemp->AU_ID)-1]);
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
								}
								if(tmp.compare("delegate") == 0) {
									ss >> tmp;
									int sID = stoi(tmp);//! Sender ID
									ss >> tmp;
									int rID = stoi(tmp);//! Reciver ID
									int v = ballot->delegate(sID, rID);
								}
								Graph::EdgeNode *eTemp = verTemp->edgeHead->next;
								while( eTemp != verTemp->edgeTail) {
									Graph::Graph_Node* refVN = 
												(Graph::Graph_Node*)eTemp->ref;
									refVN->in_count--;
									eTemp = eTemp->next;
								}
								//! num of Valid AUs executed is eAUCount+1.
								eAUCount++;
							}
						}
					}
					//Gref[t_ID]   = NULL;
					status[t_ID] = 0;
				}
			}
		}
		fvTTime[t_ID] += thTimer._timeStop(start);
	}


	//!-------------------------------------------------
	//!FINAL STATE OF ALL THE SHARED OBJECT. Once all  |
	//!AUs executed. we are geting this using state()  |
	//!-------------------------------------------------
	void finalState()
	{
		for(int id = 1; id <= nVoter; id++) 
			ballot->state(id, true, fvVState);//for voter state

		for(int id = 1; id <= nProposal; id++) 
			ballot->state(id, false, fvPState);//for Proposal state
	}

	~ForkValidator() { };
};
/******************** FORK SCV CODE ENDS *********************************/




//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//! atPoss:: from which double-spending Tx to be stored at begining       !
//! of the list. Add malicious final state with double-spending Tx        !
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
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
	int r_id = stoi(trns1);

	istringstream ss1(listAUs[atPoss-1]);
	ss1 >> trns1; //! AU_ID to Execute.
	int AU_ID2 = stoi(trns1);
	ss1 >> trns1;//function name
	ss1 >> trns1; //! Voter ID.
	int s_id1 = stoi(trns1);
	ss1 >> trns1; //! Proposal ID.
	int r_id1 = stoi(trns1);

	
	Graph:: Graph_Node *tempRef;
	if(mAUT[AU_ID1-1] != 0) {
		int ts = mAUT[AU_ID1-1]+1;
		cGraph->add_node(AU_ID2, ts, &tempRef);
		mAUT[AU_ID2-1] = ts;
		gNodeCount++;
	}
	else {
		int ts = mAUT[AU_ID2-1]+1;
		cGraph->add_node(AU_ID1, ts, &tempRef);
		mAUT[AU_ID1-1] = ts;
		gNodeCount++;
	}
	
	mPState[r_id-1]  = 1;
	mPState[r_id1-1] = 1;
	mVState[s_id1-1] = 1;
	return true;
}


/*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
/*!!!!!!!!     State Validation    !!!!!!!!!!*/
/*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
bool stateVal(bool f) {
		//State Validation
	if(f == false) {
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
	else {
		bool flag = false;
	//	cout<<"\n"<<pl<<"Proposal \tMiner \t\tValidator"<<endl;
		for(int pid = 0; pid < nProposal; pid++) {
	//		cout<<pid+1<<" \t \t"<<mPState[pid]
	//			<<" \t\t"<<vPState[pid]<<endl;
			if(mPState[pid] != fvPState[pid])
				flag = true;
		}
	//	cout<<"\n"<<pl<<"Voter ID \tMiner \t\tValidator"<<endl;
		for(int vid = 0; vid < nVoter; vid++) {
	//		cout<<vid+1<<" \t \t"<<mVState[vid]
	//			<<" \t\t"<<vVState[vid]<<endl;
			if(mVState[vid] != fvVState[vid])
				flag = true;
		}
		return flag;
	}
}





/*************************MAIN FUN CODE BEGINS*********************************/
/*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
/*!!!!!!!!          main()         !!!!!!!!!!*/
/*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
int main(int argc, char *argv[])
{
	cout<<pl<<"MVTO Miner and Concurrent Validator - Approach1\n";
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

	float tMiner  = 0, tVal     = 0, ftVal     = 0;
	int tReject   = 0, ftReject = 0, tMaxAcc   = 0;
	int ftMaxAcc  = 0, tDepInG  = 0, tInDegAUs = 0;
	float cbcTime = 0; //concurrent bin construction time.

	//! list holds the avg time taken by miner and Validator
	//! thread s for multiple consecutive runs.
	list<float>mItrT;
	list<float>vItrT;
	list<float>fvItrT;

	int totalInDegAUs= 0; //to get total number of AUs with non-zero In-degree;
	int totalDepInG  = 0; //to get total number of dependencies in graph;
	int totalRejCont = 0; //number of validator rejected the blocks;
	int ftotalRejCont= 0; //number of validator rejected the blocks;
	int maxAccepted  = 0;
	int fmaxAccepted = 0;
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
	fvPState  = new int [nProposal];
	mVState   = new int [nVoter];
	vVState   = new int [nVoter];
	fvVState  = new int [nVoter];

	float valTime, fvalTime;
	for(int itr = 0; itr < nItertion; itr++)
	{
		totalRejCont  = 0;
		maxAccepted   = 0;
		ftotalRejCont = 0;
		fmaxAccepted  = 0;
		totalDepInG   = 0;
		tInDegAUs     = 0;
		valTime       = 0;
		fvalTime      = 0;
		
		float Blockvalt, Blockfvalt;
		for(int nBlock = 0; nBlock < NumBlock; nBlock++)
		{
			Blockvalt  = 0;
			Blockfvalt = 0;
			//! generates AUs (i.e. trans to be executed by miner & validator).
			file_opr.genAUs(numAUs, nVoter, nProposal, funInContract, listAUs);
			//! index+1 represents respective AU id, and
			//! mAUT[index] represents "time stamp (commited trans)".
			mAUT = new std::atomic<int>[numAUs];
			for(int i = 0; i< numAUs; i++) mAUT[i] = 0;
			tTime[0] = 0, tTime[1] = 0;

			for(int ii = 1; ii <= numAUs; ii++) concBin.push_back(ii);

			//MINER
			Miner *miner = new Miner(0);//0 is contract deployer id
			miner ->mainMiner();

			Timer ccbTimer;
			auto start = ccbTimer._timeStart();
			auto ip = unique(concBin.begin(), concBin.end());
			concBin.resize(std::distance(concBin.begin(), ip));
			cbcTime += ccbTimer._timeStop( start );

			//Function to add malicious trans and final state by Miner
			if(lemda != 0) bool rv = addMFS(NumOfDoubleSTx);

			//give dependenices in the graph.
			if(nBlock > 0) {
				totalDepInG   += cGraph->print_grpah();
				totalInDegAUs += cGraph->inDegAUs(cGraph);
			}

			//VALIDATOR
			float valt       = 0, fvalt        = 0;
			int acceptCount  = 0, rejectCount  = 0;
			int facceptCount = 0, frejectCount = 0;

			for(int nval = 0; nval < numValidator; nval++)
			{
				valt  = 0; fvalt = 0;
				for(int p = 0; p < nProposal; p++)
					vPState[p] = fvPState[p] = 0;

				for(int v = 0; v < nVoter; v++)
					vVState[v] = fvVState[v] = 0;

				Validator *validator      = new Validator();
				ForkValidator *fvalidator = new ForkValidator();
				nValBG = NULL;
				nValBG = new Graph;
				cGraph->copy_BG(nValBG);
				validator ->mainValidator();

				cGraph->copy_BG(nValBG);
				fvalidator->mainValidator();

				//State Validation for Decentralized Validator
				bool flag = stateVal(false);
				if(flag == true) rejectCount++;
				else acceptCount++;
				
				//State Validation for ForkJoin Validator
				flag = stateVal(true);
				if(flag == true) frejectCount++;
				else facceptCount++;

				int counterv = 0, counterfv = 0;
				for( int x = 0; x < nThread; x++ ){
					if(vTTime[x] != 0) {
						valt += vTTime[x];
						counterv++;
					}
					if(fvTTime[x] != 0) {
						fvalt += fvTTime[x];
						counterfv++;
					}
				}
				if(nBlock > 0) Blockvalt  += valt/counterv;
				if(nBlock > 0) Blockfvalt += fvalt/counterfv;
			}
			if(nBlock > 0 && malMiner == true) {
				totalRejCont += rejectCount;
				if(maxAccepted < acceptCount ) maxAccepted = acceptCount;
				
				ftotalRejCont += frejectCount;
				if(fmaxAccepted < facceptCount ) fmaxAccepted = facceptCount;
			}

			int abortCnt = 0;
			for( int iii = 0; iii < nThread; iii++ ) {
				abortCnt = abortCnt + aCount[iii];
//				vTTime[iii]  = vTTime[iii] / numValidator;
//				fvTTime[iii] = fvTTime[iii] / numValidator;
			}
	//		if(nBlock > 0)cout<<"\nNumber of STM Transaction Aborted "<<abortCnt;


			float_t gConstT = 0;
			for(int ii = 0; ii < nThread; ii++) gConstT += gTtime[ii];
	//		cout<<"Avg Grpah Time= "<<gConstT/nThread<<" microseconds";

			//! total valid AUs among total AUs executed 
			//! by miner and varified by Validator.
			int vAUs = gNodeCount;
			if(nBlock > 0)
			file_opr.writeOpt(nProposal, nVoter, nThread, numAUs, tTime, mTTime,
			                  vTTime, fvTTime, aCount, vAUs, mItrT, vItrT, fvItrT, 0);

			for(int p = 0; p < nProposal; p++) mPState[p] = 0;
			for(int v = 0; v < nVoter; v++) mVState[v] = 0;

			concBin.clear();
			listAUs.clear();
			delete miner;
			miner  = NULL;
			delete cGraph;
			cGraph = NULL;

			valTime  += Blockvalt/numValidator;
			fvalTime += Blockfvalt/numValidator;
		}
		//! to get total avg miner and validator
		//! time after number of totalRun runs.
		float tAvgMinerT = 0, tAvgValidT = 0, tAvgfValidT = 0;
		int  cnt = 0, cnt1 = 0;
		auto mit = mItrT.begin();
		auto vit = vItrT.begin();
		auto fvt = fvItrT.begin();
		for(int j = 1; j < totalRun; j++){
			tAvgMinerT = tAvgMinerT + *mit;
			if(*vit != 0){
				tAvgValidT = tAvgValidT + *vit;
				cnt++;
			}
			if(*fvt != 0){
				tAvgfValidT = tAvgfValidT + *fvt;
				cnt1++;
			}
			mit++;
			vit++;
			fvt++;
		}
		tMiner   += tAvgMinerT/(NumBlock-1);
		tVal     += valTime/(NumBlock-1);
		ftVal    += fvalTime/(NumBlock-1);
		tReject  += totalRejCont /(NumBlock-1);
		ftReject += ftotalRejCont/(NumBlock-1);
		tDepInG  += totalDepInG  /(NumBlock-1);
		tInDegAUs+= totalInDegAUs/(NumBlock-1);

		if(tMaxAcc  < maxAccepted)   tMaxAcc = maxAccepted;
		if(ftMaxAcc < fmaxAccepted) ftMaxAcc = fmaxAccepted;
		mItrT.clear();
		vItrT.clear();
		fvItrT.clear();
	}
	cbcTime = cbcTime/(NumBlock*nItertion);
	cout<<"ConcBin Construct  Time in microseconds  = "<<cbcTime;
	cout<<"\nAvg Miner          Time in microseconds  = "<<cbcTime+(tMiner/nItertion);
	cout<<"\nAvg Dec  Validator Time in microseconds  = "<<tVal/nItertion;
	cout<<"\nAvg Fork Validator Time in microseconds  = "<<ftVal/nItertion;
	cout<<"\n-----------------------------\n";
	cout<<"Avg Dependencies in Graph                = "<<tDepInG/nItertion;
	cout<<"\nAvg Number of BG Vertices                = "<<tInDegAUs/nItertion;
	cout<<"\n-----------------------------\n";
	cout<<"Avg Dec  Validator Accepted a   Block    = "
		<<(numValidator-(tReject/nItertion));
	cout<<"\nAvg Dec  Validator Rejcted  a   Block    = "
		<<tReject/nItertion;
	cout<<"\nMax Dec  Validator Accepted any Block    = "<<tMaxAcc;
	cout<<"\n-----------------------------\n";
	cout<<"Avg Fork Validator Accepted a   Block    = "
		<<(numValidator-(ftReject/nItertion));
	cout<<"\nAvg Fork Validator Rejcted  a   Block    = "
		<<ftReject/nItertion;
	cout<<"\nMax Fork Validator Accepted any Block    = "<<ftMaxAcc;
	cout<<"\n"<<endl;

	delete mTTime;
	delete vTTime;
	delete fvTTime;
	delete aCount;
	return 0;
}
/*************************MAIN FUN CODE ENDS***********************************/
