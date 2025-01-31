#include <iostream>
#include <thread>
#include "Util/Timer.cpp"
#include "Contract/Coin.cpp"
#include "Graph/Lockfree/Graph.cpp"
#include "Util/FILEOPR.cpp"
#include <unistd.h>

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
Graph  *cGraph;            //! conflict graph generated by miner to be given to validator.
int    *aCount;            //! aborted transaction count.
float_t*mTTime;            //! time taken by each miner Thread to execute AUs (Transactions).
float_t*vTTime;            //! time taken by each validator Thread to execute AUs (Transactions).
float_t*fvTTime;           //! time taken by each validator Thread to execute AUs (Transactions).
vector<string>listAUs;     //! holds AUs to be executed on smart contract: "listAUs" index+1 represents AU_ID.
std::atomic<int>currAU;    //! used by miner-thread to get index of Atomic Unit to execute.
std::atomic<int>gNodeCount;//! # of valid AU node added in graph (invalid AUs will not be part of the graph & conflict list).
std::atomic<int>eAUCount;  //! used by validator threads to keep track of how many valid AUs executed by validator threads.
std::atomic<int>*mAUT;     //! array to map AUs to Trans id (time stamp); mAUT[index] = TransID, index+1 = AU_ID.
std::atomic<int>*status;   //! used by pool threads:: -1 = thread join; 0 = wait; 1 = execute AUs given in ref[].
Graph::Graph_Node **Gref;  //! used by pool threads:: graph node (AU) reference to be execute by respective Pool thread.
Graph  *nValBG;            //! used to store graph of respective n validators.
int *mState;
int *vState;
int *fvState;



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
	Miner(int minter_id)
	{
		//! init counter used to execute the numAUs to 0.
		//! init graph node counter to 0 (number of AUs 
		//! added in graph, invalid AUs are not part of the grpah).
		cGraph     = new Graph();
		currAU     = 0;
		gNodeCount = 0;
		mTTime     = new float_t [nThread];//! array index -> respective tid.
		aCount     = new int [nThread];
		for( int i = 0; i < nThread; i++ ) {
			mTTime[i] = 0;
			aCount[i] = 0;
		}
		//! id of the contract creater is "minter_id".
		coin = new Coin(SObj, minter_id);
	}


	//!------------------------------------------------------------------------- 
	//!!!!!!!! MAIN MINER:: CREATE MINER + GRAPH CONSTRUCTION THREADS !!!!!!!!!!
	//!-------------------------------------------------------------------------
	void mainMiner()
	{
		Timer Ltimer;
		thread T[nThread];
		int ts, bal = InitBalance;
		//! initialization of account with fixed
		//! ammount; mint() is assume to be serial.
		for(int sid = 1; sid <= SObj; sid++) 
			coin->mint_m(0, sid, bal, &ts);

		//!!!!!!!!!!    Create nThread Miner threads      !!!!!!!!!!
		double start = Ltimer.timeReq();//! start timer.
		for( int i = 0; i < nThread; i++ )
			T[i] = thread(concMiner, i, numAUs, cGraph);
		shoot();
		for( auto &th : T) th.join ( );
		tTime[0] = Ltimer.timeReq() - start;//! end timer: miner algorithm.

//		cGraph->print_grpah(); //! print conflict grpah generated by miner.
//		FILEOPR file_opr;
//		file_opr.pAUTrns(mAUT, numAUs); //! print AU_ID and Timestamp.
		finalState(); //! print the final state of the shared objects.
	}


	//!--------------------------------------------------------
	//! The function to be executed by all the miner threads. !
	//!--------------------------------------------------------
	static void concMiner( int t_ID, int numAUs, Graph *cGraph)
	{
		wait_for_launch();
		//! flag is used to add valid AUs in Graph (invalid AU: 
		//! senders does't have sufficent balance to send).
		//! get the current index, and increment it.
		//! statrt clock to get time taken by this transaction.
		Timer Ttimer;
		bool flag   = true;
		int  curInd = currAU++;
		auto start  = Ttimer._timeStart();
		while(curInd < numAUs)
		{
			//!tid of STM_OSTM_transaction that successfully executed this AU.
			//! trans_ids with which this AU.trans_id is conflicting.
			//! get the AU to execute, which is of string type.
			int t_stamp;
			list<int>conf_list;
			istringstream ss(listAUs[curInd]);
			string tmp;
			ss >> tmp; //! AU_ID to Execute.
			int AU_ID = stoi(tmp);
			ss >> tmp; //! Function Name (smart contract).
			if(tmp.compare("get_bal") == 0)
			{
				ss >> tmp; //! get balance of SObj/id.
				int s_id = stoi(tmp);
				int bal  = 0;
				//! get_bal() of smart contract.
				bool v = coin->get_bal_m(s_id, &bal, t_ID, &t_stamp, conf_list);
				while(v == false) //! execute again if tryCommit fails.
				{
					aCount[t_ID]++;
					v = coin->get_bal_m(s_id, &bal, t_ID, &t_stamp, conf_list);
				}
				mAUT[AU_ID-1] = t_stamp;
			}

			if(tmp.compare("send") == 0)
			{
				ss >> tmp; //! Sender ID.
				int s_id  = stoi(tmp);
				ss >> tmp; //! Reciver ID.
				int r_id  = stoi(tmp);
				ss >> tmp; //! Ammount to send.
				int amt   = stoi(tmp);
				int v =coin->send_m(t_ID, s_id, r_id, amt, &t_stamp, conf_list);
				while(v != 1 ) //! execute again if tryCommit fails.
				{
					aCount[t_ID]++;
					v =coin->send_m(t_ID, s_id, r_id, amt, &t_stamp, conf_list);
					if(v == -1) {
						//! invalid AU: sender does't 
						//! have sufficent balance to send.
						flag = false;
						break;                                    
					}
				}
				mAUT[AU_ID-1] = t_stamp;
			}

			//! graph construction for committed AUs.
			if (flag == true)
			{
				//! increase graph node counter (Valid AU executed).
				gNodeCount++;
				//! get respective tran conflict list using lib fun.
				//list<int>conf_list = lib->get_conf(t_stamp);

				//! IMP::delete time stamps in conflict list, which are added
				//! because of initilization of SObj by mnit() trycommit.
				for(int y = 0; y <= 2*SObj; y++) conf_list.remove(y);
				//!------------------------------------------
				//! conf_list come from contract fun using  !
				//! pass by argument of get_bel() and send()!
				//!------------------------------------------
				//!when conflist is empty.
				if(conf_list.begin() == conf_list.end()) {
					Graph:: Graph_Node *tempRef;
					cGraph->add_node(AU_ID, t_stamp, &tempRef);
				}

				for(auto it = conf_list.begin(); it != conf_list.end(); it++)
				{
					int i = 0;
					//! find the conf_AU_ID in map table
					//! given conflicting time-stamp.
					while(*it != mAUT[i]) i = (i+1)%numAUs; 

					//! because array index start with 0
					//! and index+1 respresent AU_ID.
					int cAUID   = i+1;
					//! conflicting AU_ID with this.AU_ID.
					int cTstamp = mAUT[i];
					//! edge from conf_AU_ID to AU_ID.
					if(cTstamp  < t_stamp)
						cGraph->add_edge(cAUID, AU_ID, cTstamp, t_stamp);
					//! edge from AU_ID to conf_AU_ID.
					if(cTstamp > t_stamp)
						cGraph->add_edge(AU_ID, cAUID, t_stamp, cTstamp);
				}
			}
			//! reset flag for next AU.
			//! get the current index to execute, and increment it.
			flag   = true;
			curInd = currAU++;
		}
		mTTime[t_ID] = mTTime[t_ID] + Ttimer._timeStop( start );
	}

	//!-------------------------------------------------
	//!FINAL STATE OF ALL THE SHARED OBJECT. Once all  |
	//!AUs executed. we are geting this using get_bel()|
	//!-------------------------------------------------
	void finalState()
	{
		list<int>cList;
		for(int sid = 1; sid <= SObj; sid++) {
			int bal = 0, ts;
			coin->get_bal_m(sid, &bal, 0, &ts, cList);
			mState[sid] = bal;
		}
	}
	~Miner() { };
};
/*************************MINER CODE ENDS*********************************/




/*************************Dec. SCV CODE BEGINS****************************/
/*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
! Class "Validator" Create & run "n" validator-thread concurrently   !
! based on conflict grpah! given by miner. concValidator() called    !
! by validator-thread to perfrom operations of respective AUs.       !
! Thread 0 is considered as minter-thread (smart contract deployer). !
!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
class Validator
{
	public:
	Validator()
	{
		//! array index location represents respective thread id.
		eAUCount = 0;
		vTTime   = new float_t[nThread];
		for(int i = 0; i < nThread; i++) vTTime[i] = 0;
	};

	/*!--------------------------------------
	| create n concurrent validator threads | 
	| to execute valid AUs in conf graph.   |
	---------------------------------------*/
	void mainValidator()
	{
		for(int i = 0; i < nThread; i++) vTTime[i] = 0;
		eAUCount = 0;
		coin->reset();

		Timer Ttimer;
		int bal = InitBalance;
		thread T[nThread];
		//! initialization of account with fixed ammount;
		//! mint() function is assume to be serial.
		for(int sid = 1; sid <= SObj; sid++) 
			coin->mint(0, sid, bal); //! 0 is contract deployer.

		//!Create "nThread" threads
		double start = Ttimer.timeReq(); //! start timer.
		for(int i = 0; i < nThread; i++)
			T[i] = thread(concValidator, i);
		shoot(); //notify all threads to begin the worker();
		for(auto& th : T) th.join ( );
		tTime[1] = Ttimer.timeReq() - start; //! stop timer

		finalState(); //! print the final state of the SObjs by validator.
	}

	//!--------------------------------------------------------
	//! The function to be executed by all Validator threads. !
	//!--------------------------------------------------------
	static void concValidator( int t_ID )
	{
		//barrier to synchronise all threads for a coherent launch
		wait_for_launch();
		Timer Ttimer;
		//! start timer to get time taken by this thread.
		auto start = Ttimer._timeStart();
		list<Graph::Graph_Node*>buffer;
		auto itr = buffer.begin();
		Graph:: Graph_Node *verTemp;
		while( true )
		{
			//!uncomment this to remove the effect of local buffer optimization.
//			buffer.clear();

			//! all Graph Nodes (Valid AUs executed).
			if(eAUCount == gNodeCount) break;

			//!------------------------------------------
			//!!!<< AU execution from local buffer. >>!!!
			//!------------------------------------------
			for(itr = buffer.begin(); itr != buffer.end(); itr++)
			{
				Graph::Graph_Node* temp = *itr;
				if(temp->in_count == 0)
				{
					//! expected in_degree is 0 then vertex can
					//! be executed if not claimed by other thread
					int expected = 0;
					if(atomic_compare_exchange_strong(
									&(temp->in_count), &expected, -1 ) == true)
					{
						eAUCount++; //! num of Valid AUs executed is eAUCount+1
						//! get the AU to execute, which is of
						//! string type; listAUs index statrt with 0
						istringstream ss(listAUs[(temp->AU_ID)-1]);
						string tmp;
						ss >> tmp; //! AU_ID to Execute.
						int AU_ID = stoi(tmp);
						ss >> tmp; //! Function Name (smart contract).
						if(tmp.compare("get_bal") == 0)
						{
							ss >> tmp;//! get balance of SObj/id.
							int s_id = stoi(tmp);
							int bal  = 0;
							//! get_bal() of smart contract.
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
						}
						
						//!-----------------------------------------
						//!change indegree of out edge nodes (node !
						//! having incomming edge from this node). !
						//!-----------------------------------------						
						Graph::EdgeNode *e_temp = temp->edgeHead->next;
						while( e_temp != temp->edgeTail)
						{
							Graph::Graph_Node* refVN =
									(Graph::Graph_Node*)e_temp->ref;
							refVN->in_count--;
							if(refVN->in_count == 0 )
							buffer.push_back(refVN);//! insert into local buffer.
							e_temp = e_temp->next;
						}
					}
				}
			}
			buffer.clear();//! reached to end of local buffer; clear the buffer.

			//!-----------------------------------------------------
			//!!!<< AU execution by traversing conflict grpah  >>!!!
			//!-----------------------------------------------------
			verTemp = nValBG->verHead->next;
			while(verTemp != nValBG->verTail)
			{
				if(verTemp->in_count == 0)
				{
					//! expected in_degree is 0 then vertex can be
					//! executed if not claimed by other thread
					int expected = 0;
					if(atomic_compare_exchange_strong(
							 &(verTemp->in_count), &expected, -1 ) == true)
					{
						eAUCount++; //! num of Valid AUs executed is eAUCount+1
						//get the AU to execute, which is of string
						//type; listAUs index statrt with 0
						istringstream ss( listAUs[(verTemp->AU_ID) -1 ]);
						string tmp;
						ss >> tmp; //! AU_ID to Execute.
						int AU_ID = stoi(tmp);
						ss >> tmp; //! Function Name (smart contract).
						if(tmp.compare("get_bal") == 0)
						{
							ss >> tmp; //! get balance of SObj/id.
							int s_id = stoi(tmp);
							int bal  = 0;
							//! get_bal() of smart contract.
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
						}
						
						//!-----------------------------------------
						//!change indegree of out edge nodes (node !
						//! having incomming edge from this node). !
						//!-----------------------------------------
						Graph::EdgeNode *e_temp = verTemp->edgeHead->next;

						while(e_temp != verTemp->edgeTail)
						{
							Graph::Graph_Node* refVN = 
											(Graph::Graph_Node*)e_temp->ref;
							refVN->in_count--;
							//! insert into local buffer.
							if(refVN->in_count == 0 ) buffer.push_back(refVN);
							e_temp = e_temp->next;
						}
					}
				}
				verTemp = verTemp->next;
			}
		}
		//!stop timer to get time taken by this thread
		vTTime[t_ID] = vTTime[t_ID] + Ttimer._timeStop( start );
	}


	//!-------------------------------------------------
	//!FINAL STATE OF ALL THE SHARED OBJECT. Once all  |
	//!AUs executed. Geting this using get_bel_val()   |
	//!-------------------------------------------------
	void finalState()
	{
		int total = 0;
		for(int sid = 1; sid <= SObj; sid++) {
			int bal = 0;
			coin->get_bal(sid, &bal);
			vState[sid] = bal;
		}
	}

	~Validator() { };
};
/********************** Dec. SCV CODE ENDS **************************/



/******************** FORK SCV CODE BEGINS **************************/
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
		eAUCount = 0;
		//! thread id represents index location.
		fvTTime = new float_t[nThread+1];
		status  = new std::atomic<int>[nThread+1];
		Gref    = new Graph::Graph_Node*[nThread+1];
		int i   = 0;
		for( ; i < nThread; i++) {
			status[i]  = 0;
			Gref[i]    = NULL;
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
		for(int i = 0; i < nThread; i++) fvTTime[i] = 0;
		eAUCount = 0;

		Timer Ttimer;
		int bal = InitBalance;

		//! initialization of account with fixed ammount;
		//! mint_val() function is assume to be serial.
		for(int sid = 1; sid <= SObj; sid++) 
			bool r = coin->mint(0, sid, bal); //! 0 is contract deployer.

		//!----------------------------------------------------
		//! MASTER THREAD CREATE n VALIDATOR THREADS          !
		//!----------------------------------------------------
		double start = Ttimer.timeReq();
		thread master = thread(concValidator, 0 );
		master.join();
		tTime[1] = Ttimer.timeReq() - start;

		//! print the final state of the shared objects by validator.
		finalState();
	}

	//!--------------------------------------------------------
	//! The function to be executed by all Validator threads. !
	//!--------------------------------------------------------
	static void concValidator( int t_ID )
	{
		Timer Ttimer;
		//! start timer to get time taken by this thread.
		auto start = Ttimer._timeStart();

		//! ONLY MASTER THREAD WILL EXECUTE IT.
		if(t_ID == 0)
		{
			thread POOL[nThread+1];
			bool tCratFlag = true;//! POOL thread creation flag.
			Graph::Graph_Node *mVItr;
			while(true)
			{
				if(tCratFlag == true)
				{
					//! Creating n POOL Threads
					for(int i = 1; i <= nThread; i++)
					{
						POOL[i] = thread(concValidator, i);
					}
					tCratFlag = false;
				}
				//! All Valid AUs executed.
				if(eAUCount == gNodeCount)
				{
					for(int i = 1; i <= nThread; i++)
					{
						//! -1 = threads can join now.
						status[i] = -1;
					}
					
					//! POOL thread join.
					for(int i = 1; i <= nThread; i++)
					{
						POOL[i].join( );
					}
					break;
				}
				mVItr = nValBG->verHead->next;
				while(mVItr != nValBG->verTail)
				{
					if(mVItr->in_count == 0)
					{
						for(int i = 1; i <= nThread; i++)
						{
							//! 0 = thread is available.
							if(status[i] == 0)
							{
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
					verTemp  = Gref[t_ID];
					if(verTemp->in_count == 0)
					{
						if(verTemp->in_count < 0)
						{
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

								//! AU_ID to Execute.
								ss >> tmp;
								int AU_ID = stoi(tmp);
								//! Function Name (smart contract).
								ss >> tmp;
								if( tmp.compare("get_bal") == 0 )
								{
									//! get balance of SObj/id.
									ss >> tmp;
									int s_id = stoi(tmp);
									int bal  = 0;

									//! get_bal() of smart contract.
									bool v = coin->get_bal(s_id, &bal);
								}
								if( tmp.compare("send") == 0 )
								{
									//! Sender ID.
									ss >> tmp;
									int s_id = stoi(tmp);

									//! Reciver ID.
									ss >> tmp;
									int r_id = stoi(tmp);
									//! Ammount to send.
									ss >> tmp;
									int amt  = stoi(tmp);
									bool v   = coin->send(s_id, r_id, amt);
								}
								Graph::EdgeNode *eTemp =verTemp->edgeHead->next;
								while( eTemp != verTemp->edgeTail)
								{
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
					status[t_ID] = 0;
				}
			}
		}
		fvTTime[t_ID] = fvTTime[t_ID] + Ttimer._timeStop( start );
	}


	//!-------------------------------------------------
	//!FINAL STATE OF ALL THE SHARED OBJECT. Once all  |
	//!AUs executed. Geting this using get_bel_val()   |
	//!-------------------------------------------------
	void finalState()
	{
		for(int sid = 1; sid <= SObj; sid++) 
		{
			int bal = 0, ts;
			bool v  = coin->get_bal(sid, &bal);
			fvState[sid] = bal;
		}
	}

	~ForkValidator() { };
};
/******************** FORK SCV CODE ENDS *********************************/







//!--------------------------------------------------------------------------
//! atPoss:: from which double-spending Tx to be stored at end of the list. !
//! add malicious final state with double-spending Tx                       !
//!--------------------------------------------------------------------------
bool addMFS(int atPoss)
{
	istringstream ss(listAUs[atPoss-2]);
	string trns1;
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
	ss1 >> trns1; //! AU_ID to Execute.
	int AU_ID2 = stoi(trns1);
	ss1 >> trns1;//function name
	ss1 >> trns1; //! Sender ID.
	int s_id1 = stoi(trns1);
	ss1 >> trns1; //! Reciver ID.
	int r_id1 = stoi(trns1);
	ss1 >> trns1; //! Ammount to send.
	int amtAC  = stoi(trns1);

	cGraph->remove_AU_Edge(cGraph, AU_ID1);
	cGraph->remove_AU_Edge(cGraph, AU_ID2);

	mState[s_id]  = 1000;
	mState[r_id]  = 1000;
	mState[r_id1] = 1000;
	
	amtAB = 950;
	trns1 = to_string(AU_ID1)+" send "+to_string(s_id)
			+" "+to_string(r_id)+" "+to_string(amtAB);
	listAUs[AU_ID1-1] =  trns1;
	
	amtAC = 100;
	trns1 = to_string(AU_ID2)+" send "+to_string(s_id)
			+" "+to_string(r_id1)+" "+to_string(amtAC);
	listAUs[AU_ID2-1] =  trns1;

	mState[s_id]  -= amtAB;
	mState[r_id]  += amtAB;
	mState[r_id1] += amtAC;
return true;
}



bool stateVal(bool f)
{
	//State Validation
	if(f == false) {
		bool flag = false;
	//	cout<<"\n"<<pl<<"SObject \tMiner \t\tValidator"<<endl;
		for(int sid = 1; sid <= SObj; sid++) {
	//		cout<<sid<<" \t \t"<<mState[sid]
	//			<<" \t\t"<<vState[sid]<<endl;
			if(mState[sid] != vState[sid]) flag = true;
		}
		return flag;
	}
	else {
		bool flag = false;
	//	cout<<"\n"<<pl<<"SObject \tMiner \t\tValidator"<<endl;
		for(int sid = 1; sid <= SObj; sid++) {
	//		cout<<sid<<" \t \t"<<mState[sid]
	//			<<" \t\t"<<vState[sid]<<endl;
			if(mState[sid] != fvState[sid]) flag = true;
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
	cout<<pl<<"MVOSTM Miner and Concurrent Validator\n";
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
	float ftVal   = 0;
	int tReject   = 0;
	int ftReject  = 0;
	int tMaxAcc   = 0;
	int ftMaxAcc  = 0;
	int tDepInG   = 0;
	int tInDegAUs = 0;

	//! list holds the avg time taken by miner and 
	//! Validator thread s for multiple consecutive runs.
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

	//! read from input file:: SObj = #SObj; nThread = #threads;
	//! numAUs = #AUs; λ =  % of edges to be removed from BG by malicious Miner.
	file_opr.getInp(&SObj, &nThread, &numAUs, &lemda);

	if(SObj > M_SharedObj) {
		SObj = M_SharedObj;
		cout<<"Max number of Shared Object can be "<<M_SharedObj<<"\n";
	}

	mState = new int [SObj];
	vState = new int [SObj];
	fvState= new int [SObj];

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
			file_opr.genAUs(numAUs, SObj, FUN_IN_CONT, listAUs);
			//! index+1 represents respective AU id, and
			//! mAUT[index] represents "time stamp (commited trans)".
			mAUT = new std::atomic<int>[numAUs];
			for(int i = 0; i< numAUs; i++) mAUT[i] = 0;
			tTime[0] = 0;
			tTime[1] = 0;
			Timer mTimer;
			mTimer.start();

			//MINER
			Miner *miner = new Miner(0);
			miner ->mainMiner();

			if(lemda != 0) bool rv = addMFS(NumOfDoubleSTx);

			//give dependenices in the graph.
			if(nBlock > 0) {
				totalDepInG   += cGraph->print_grpah();
				totalInDegAUs += cGraph->inDegAUs(cGraph);
			}
			//VALIDATOR
			if(MValidation == true)
			{
				float valt       = 0, fvalt        = 0;
				int acceptCount  = 0, rejectCount  = 0;
				int facceptCount = 0, frejectCount = 0;

				for(int nval = 0; nval < numValidator; nval++)
				{
					valt  = 0, fvalt = 0;
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
				for(int i = 1; i <= SObj; i++) {
					vState[i]  = 0;
					fvState[i] = 0;
				}
			}
			else {
				Validator *validator = new Validator();
				nValBG = new Graph;
				cGraph->copy_BG(nValBG);
				validator ->mainValidator();
				//State Validation
				bool flag = stateVal(false);
				if(flag == true) cout<<"\nBlock Rejected by Validator";
			}
			int abortCnt = 0;
			for( int iii = 0; iii < nThread; iii++ ) {
				abortCnt = abortCnt + aCount[iii];
			}
	//		if(nBlock > 0)cout<<"\nNumber of STM Transaction Aborted "<<abortCnt;


			mTimer.stop();

			//total valid AUs among total AUs executed
			//by miner and varified by Validator.
			int vAUs = gNodeCount;
			if(nBlock > 0)//skip first run
				file_opr.writeOpt(SObj, nThread, numAUs, tTime, mTTime, vTTime, 
					              fvTTime, aCount, vAUs, mItrT, vItrT, fvItrT, 0);

			for(int i = 1; i <= SObj; i++) {
				mState[i] = 0;
				vState[i] = 0;
				fvState[i]= 0;
			}
			listAUs.clear();
			delete miner;
			miner = NULL;
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
	cout<<"Avg Miner          Time in microseconds  = "<<tMiner/nItertion;
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
