#include <iostream>
#include <thread>
#include "Util/Timer.cpp"
#include "Contract/SimpleAuction.cpp"
#include "Graph/Lockfree/Graph.cpp"
#include "Util/FILEOPR.cpp"

#define maxThreads 128
#define maxBObj 10000
#define maxbEndT 10000 //millseconds
#define funInContract 6
#define pl "===================================================\n"
#define MValidation true   //! true or false
#define malMiner true      //! set the flag to make miner malicious.
#define NumOfDoubleSTx 2   //! # double-spending Tx for malicious final state by Miner, multiple of 2.

using namespace std;
using namespace std::chrono;

int NumBlock     = 26;   //! at least two blocks, the first run is warmup run.
int numValidator = 50;
int beneficiary = 0;     //! fixed beneficiary id to 0, it can be any unique address/id.
int    nBidder  = 2;     //! nBidder: number of bidder shared objects.
int    nThread  = 1;     //! nThread: total number of concurrent threads; default is 1.
int    numAUs;           //! numAUs: total number of Atomic Unites to be executed.
double lemda;            //! λ: random delay seed.
int    bidEndT;          //! time duration for auction.
double tTime[3];         //! total time taken by miner and validator algorithm respectively.
Graph  *cGraph   = NULL; //! conflict grpah generated by miner to be given to validator.
int    *aCount   = NULL; //! aborted transaction count.
float_t *mTTime  = NULL; //! time taken by each miner Thread to execute AUs (Transactions).
float_t *vTTime  = NULL; //! time taken by each validator Thread to execute AUs (Transactions).
float_t *fvTTime = NULL; //! time taken by each validator Thread to execute AUs (Transactions).
float_t *gTtime  = NULL; //! time taken by each miner Thread to add edges and nodes in the Block Graph.
vector<string>   listAUs;      //! holds AUs to be executed on smart contract: "listAUs" index+1 = AU_ID.
std::atomic<int> currAU;       //! used by miner-thread to get index of Atomic Unit to execute.
std::atomic<int> gNodeCount;   //! # of valid AU node added in graph.
std::atomic<int> eAUCount;     //! used by validator threads to keep track of how many AUs executed.
std::atomic<int>*status;       //! used by pool threads:: -1 = thread join; 0 = wait; 1 = execute AUs.
Graph::Graph_Node **Gref;      //! used by pool threads:: graph node (AU) reference to execute.
std::atomic<int> *mAUT = NULL; //! map AUs to TransId (ts); mAUT[index] = TransID, index+1 = AU_ID.
Graph  *nValBG;                //! used to store graph of respective n validators.
SimpleAuction *auction = NULL; //! smart contract for miner.

//State Data
int mHBidder;
int mHBid;
int vHBidder;
int fvHBidder;
int vHBid;
int fvHBid;
int *mPendingRet;
int *vPendingRet;
int *fvPendingRet;



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
	Miner( )
	{
		cGraph = NULL;
		cGraph = new Graph();
		//! initialize the counter used to execute the numAUs to
		//! 0, and graph node counter to 0 (number of AUs added in
		//! graph, invalid AUs will not be a part of the grpah).
		currAU     = 0;
		gNodeCount = 0;
		//! index location represents respective thread id.
		mTTime = new float_t [nThread];
		gTtime = new float_t [nThread];
		aCount = new int [nThread];
		for(int i = 0; i < nThread; i++) {
			mTTime[i] = 0;
			gTtime[i] = 0;
			aCount[i] = 0;
		}
		auction = new SimpleAuction(bidEndT, beneficiary, nBidder);
	}

	//!------------------------------------------------------------------------- 
	//!!!!!!! MAIN MINER:: CREATE (MINER + GRAPH CONSTRUCTION) THREADS !!!!!!!!!
	//!-------------------------------------------------------------------------
	void mainMiner()
	{
		Timer mTimer;
		thread T[nThread];
		//!!!!!!!!!!    Create nThread Miner threads      !!!!!!!!!!
		double start = mTimer.timeReq();
		for(int i = 0; i < nThread; i++)
			T[i] = thread(concMiner, i, numAUs, cGraph);
		for(auto& th : T) th.join();
		tTime[0] = mTimer.timeReq() - start;

		//! print conflict grpah.
//		cGraph->print_BG();
		//! print the final state of the shared objects.
		finalState();
//		 auction->AuctionEnded_m( );
	}

	//!--------------------------------------------------------
	//! The function to be executed by all the miner threads. !
	//!--------------------------------------------------------
	static void concMiner( int t_ID, int numAUs, Graph *cGraph)
	{
		Timer thTimer;
		//! flag is used to add valid AUs in Graph.
		//! (invalid AU: senders doesn't have sufficient balance to send).
		bool flag = true;
		//! get the current index, and increment it.
		int curInd = currAU++;
		//! statrt clock to get time taken by this.AU
		auto start = thTimer._timeStart();
		while(curInd < numAUs) {
			int t_stamp;
			list<int>conf_list;
			conf_list.clear();
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
				int v = auction->bid_m(payable, bID, bAmt, &t_stamp, conf_list);
				while(v != 1) {
					aCount[0]++;
					v = auction->bid_m(payable, bID, bAmt, &t_stamp, conf_list);
					if(v == -1) {
						flag = false;//! invalid AU.
						break;                                    
					}
				}
			}
			if(tmp.compare("withdraw") == 0) {
				ss >> tmp;
				int bID = stoi(tmp);//! Bidder ID

				int v = auction->withdraw_m(bID, &t_stamp, conf_list);
				while(v != 1) {
					aCount[0]++;
					v = auction->withdraw_m(bID, &t_stamp, conf_list);
					if(v == -1) {
						flag = false;//! invalid AU.
						break;                                    
					}
				}
			}
			if(tmp.compare("auction_end") == 0) {
				int v = auction->auction_end_m(&t_stamp, conf_list);
				while(v != 1) {
					aCount[0]++;
					v = auction->auction_end_m(&t_stamp, conf_list);
					if(v == -1) {
						flag = false;//! invalid AU.
						break;                                    
					}
				}
			}
			//! graph construction for committed AUs.
			if (flag == true) {
				mAUT[AU_ID-1] = t_stamp;
				gNodeCount++;//! increase graph node counter (Valid AU executed)
				//! get respective trans conflict list using lib fun
				//list<int>conf_list = lib->get_conf(t_stamp);
				
				//!::::::::::::::::::::::::::::::::::::::::::::::::::::::::
				//! Remove time stamps from conflict list, added because  !
				//! of init and creation of shared object in STM memory   !
				//!::::::::::::::::::::::::::::::::::::::::::::::::::::::::
				for(int y = 0; y <= 1; y++) conf_list.remove(y);

				//! statrt clock to get time taken by this.thread 
				//! to add edges and node to conflict grpah.
				auto gstart = thTimer._timeStart();

				//!------------------------------------------
				//! conf_list come from contract fun using  !
				//! pass by argument of get_bel() and send()!
				//!------------------------------------------
				//! when AU_ID conflict is empty.
				if(conf_list.begin() == conf_list.end()) {
					Graph:: Graph_Node *tempRef;
					cGraph->add_node(AU_ID, t_stamp, &tempRef);
				}

				for(auto it = conf_list.begin(); it != conf_list.end(); it++) {
					int i = 0;
					//! get conf AU_ID in map table given conflicting tStamp.
					while(*it != mAUT[i]) i = (i+1)%numAUs;
					//! index start with 0 => index+1 respresent AU_ID.
					//! cAUID = index+1, cTstamp = mAUT[i] with this.AU_ID
					int cAUID   = i+1;
					int cTstamp = mAUT[i];
					if(cTstamp < t_stamp) //! edge from cAUID to AU_ID.
						cGraph->add_edge(cAUID, AU_ID, cTstamp, t_stamp);
					if(cTstamp > t_stamp) //! edge from AU_ID to cAUID.
						cGraph->add_edge(AU_ID, cAUID, t_stamp, cTstamp);
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
	//!Final state of all the shared object. Once all  |
	//!AUs executed. we are geting this using state_m()|
	//!-------------------------------------------------
	void finalState() {	
		auction->state_m(&mHBidder, &mHBid, mPendingRet);
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
	Validator()
	{
		//! int the execution counter used by validator threads.
		eAUCount = 0;
		//! array index location represents respective thread id.
		vTTime = new float_t [nThread];
		for(int i = 0; i < nThread; i++) vTTime[i] = 0;
	};


	/*!---------------------------------------
	| Create n concurrent validator threads  |
	| to execute valid AUs in conflict graph.|
	----------------------------------------*/
	void mainValidator()
	{
		Timer vTimer;
		thread T[nThread];
		auction->reset(bidEndT);
		//!!!!! Create nThread Validator threads !!!!
		double start = vTimer.timeReq();
		for(int i = 0; i<nThread; i++)
			T[i] = thread(concValidator, i);
		shoot(); //notify all threads to begin the worker();
		for(auto& th : T) th.join( );
		tTime[1] = vTimer.timeReq() - start;

		//!print the final state of the shared objects by validator.
		finalState();
//		auction->AuctionEnded( );
	}

	//!--------------------------------------------------------
	//! The function to be executed by all Validator threads. !
	//!--------------------------------------------------------
	static void concValidator( int t_ID )
	{
		//barrier to synchronise all threads for a coherent launch.
		wait_for_launch();
		Timer thTimer;
		auto start = thTimer._timeStart();
		list<Graph::Graph_Node*>buffer;
		auto itr = buffer.begin();
		Graph:: Graph_Node *verTemp;
		while( true )
		{
			//! uncomment this to remove the effect of local buffer optimization.
			//buffer.clear();

			//! all Graph Nodes (Valid AUs executed)
			if(eAUCount == gNodeCount ) break;
			//!-----------------------------------------
			//!!!<< AU EXECUTION FROM LOCAL buffer. >>!!
			//!-----------------------------------------
			
			for(itr = buffer.begin(); itr != buffer.end(); itr++) {
				Graph::Graph_Node* temp = *itr;
				if(temp->in_count == 0) {
					//! expected in_degree is 0 then vertex can be executed,
					//! if not claimed by other thread.
					int expected = 0;
					if( atomic_compare_exchange_strong
					  ( &(temp->in_count), &expected, -1 ) == true)
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
						if(tmp.compare("bid") == 0) {
							ss >> tmp;
							int payable = stoi(tmp);//! payable
							ss >> tmp;
							int bID = stoi(tmp);//! Bidder ID
							ss >> tmp;
							int bAmt = stoi(tmp);//! Bidder value
							bool v = auction->bid(payable, bID, bAmt);
						}
						if(tmp.compare("withdraw") == 0) {
							ss >> tmp;
							int bID = stoi(tmp);//! Bidder ID
							bool v = auction->withdraw(bID);
						}
						if(tmp.compare("auction_end") == 0) {
							bool v = auction->auction_end( );
						}

						//!------------------------------------------
						//! Change indegree of out edge nodes (node !
						//! having incomming edge from this node).  !
						//!------------------------------------------						
						Graph::EdgeNode *eTemp = temp->edgeHead->next;
						while( eTemp != temp->edgeTail) {
							Graph::Graph_Node* refVN = 
							      (Graph::Graph_Node*)eTemp->ref;

							refVN->in_count--;
							if(refVN->in_count == 0 ) //! insert into local buf.
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
			while(verTemp != nValBG->verTail) {
				if(verTemp->in_count == 0) {
					//! expected in_degree is 0 then vertex can be executed,
					//! if not claimed by other thread.
					int expected = 0;
					if( atomic_compare_exchange_strong
					  ( &(verTemp->in_count), &expected, -1 ) == true)
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
						if(tmp.compare("bid") == 0) {
							ss >> tmp;
							int payable = stoi(tmp);//! payable
							ss >> tmp;
							int bID = stoi(tmp);//! Bidder ID
							ss >> tmp;
							int bAmt = stoi(tmp);//! Bidder value
							bool v = auction->bid(payable, bID, bAmt);
						}
						if(tmp.compare("withdraw") == 0) {
							ss >> tmp;
							int bID = stoi(tmp);//! Bidder ID
							bool v = auction->withdraw(bID);
						}
						if(tmp.compare("auction_end") == 0) {
							bool v = auction->auction_end( );
						}
						
						//!------------------------------------------
						//! Change indegree of out edge nodes (node !
						//! having incomming edge from this node).  !
						//!------------------------------------------
						Graph::EdgeNode *eTemp = verTemp->edgeHead->next;
						while( eTemp != verTemp->edgeTail) {
							Graph::Graph_Node* refVN = 
							      (Graph::Graph_Node*)eTemp->ref;
							refVN->in_count--;
							if(refVN->in_count == 0 )//!insert into local buffer
								buffer.push_back( refVN );
							eTemp = eTemp->next;
						}
					}
				}
				verTemp = verTemp->next;
			}
			//sleep(.00009);
		}
		verTemp = NULL;
		delete verTemp;
		buffer.clear();
		vTTime[t_ID] += thTimer._timeStop(start);
	}


	//!-------------------------------------------------
	//!Final state of all the shared object. once all  |
	//!aus executed. We are geting this using state()  |
	//!-------------------------------------------------
	void finalState() {
		auction->state(&vHBidder, &vHBid, vPendingRet);
	}

	~Validator() { };
};
/************************* Dec. CODE ENDS  **************************/



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
		//! int the execution counter
		//! used by validator threads.
		eAUCount = 0;
		//! array index location => thread id.
		fvTTime = new float_t [nThread+1];
		status  = new std::atomic<int>[nThread+1];
		Gref    = new Graph::Graph_Node*[nThread+1];
		int i = 0;
		for( ; i < nThread; i++) {
			status[i] = 0;
			Gref[i]   = NULL;
			fvTTime[i] = 0;	
		}
		fvTTime[i] = 0;
	};

	//!-------------------------------------------------
	//! Master thread: creates n worker                !
	//! threads to execute valid AUs in conflict graph.!
	//!-------------------------------------------------
	void mainValidator()
	{
		for( int i = 0; i < nThread; i++) {
			status[i] = 0;
			Gref[i]   = NULL;
		}
		eAUCount  = 0;
		Timer Ttimer;
		auction->reset(bidEndT);
		//!--------------------------------------------
		//! MASTER THREAD CREATE n VALIDATOR THREADS  !
		//!--------------------------------------------
		double start = Ttimer.timeReq();
		thread master = thread(concValidator, 0 );
		master.join();
		tTime[2] = Ttimer.timeReq() - start;

		//!print the final state of the shared objects by validator.
		finalState();
//		auction->AuctionEnded( );
	}

	//!--------------------------------------------------------
	//! The function to be executed by all Validator threads. !
	//!--------------------------------------------------------
	static void concValidator( int t_ID )
	{
		Timer thTimer;
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
						else {
							//! expected in_degree is 0 then
							//! vertex can be executed if 
							//! not claimed by other thread.
							int expected = 0;
							if( atomic_compare_exchange_strong 
							  ( &(verTemp->in_count), &expected, -1 ) == true)
							{
								verTemp->in_count = -1;
								//! get AU to execute, which is of string type;
								//! listAUs index statrt with 0 => -1.
								istringstream ss(listAUs[(verTemp->AU_ID)-1]);
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
								}
								if(tmp.compare("withdraw") == 0) {
									ss >> tmp;
									int bID = stoi(tmp);//! Bidder ID
									bool v = auction->withdraw(bID);
								}
								if(tmp.compare("auction_end") == 0) {
									bool v = auction->auction_end( );
								}
								Graph::EdgeNode *eTemp= verTemp->edgeHead->next;
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
	//!Final state of all the shared object. once all  |
	//!aus executed. We are geting this using state()  |
	//!-------------------------------------------------
	void finalState() {	
		auction->state(&fvHBidder, &fvHBid, fvPendingRet);
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
	ss >> trns1; //function name
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
	ss1 >> trns1; //function name
	ss1 >> trns1; //! payable.
	int payable1 = stoi(trns1);
	ss1 >> trns1; //! bidderID.
	int bidderID1 = stoi(trns1);
	ss1 >> trns1; //! Ammount to bid.
	int bidValue1  = stoi(trns1);


	Graph:: Graph_Node *tempRef;
	int ts = 5000;
	cGraph->add_node(AU_ID1, ts, &tempRef);
	mAUT[AU_ID1-1] = ts;
	gNodeCount++;

	ts = 5001;
	cGraph->add_node(AU_ID2, ts, &tempRef);
	mAUT[AU_ID2-1] = ts;
	gNodeCount++;

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
	mPendingRet[bidderID1] = mHBid;

	return true;
}


/*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
/*!!!!!!!!     State Validation    !!!!!!!!!!*/
/*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
bool stateVal(bool f) {
	//State Validation
	if(f == false) {
		bool flag = false;
		if(mHBidder != vHBidder || mHBid != vHBid) flag = true;
	//	cout<<"\n============================================"
	//	    <<"\n     Miner Auction Winer "<<mHBidder
	//	    <<" |  Amount "<<mHBid;
	//	cout<<"\n Dec. Validator Auction Winer "<<to_string(vHBidder)
	//	    <<" |  Amount "<<to_string(vHBid);
	//	cout<<"\n============================================\n";
		return flag;
	}
	else {
		bool flag = false;
		if(mHBidder != fvHBidder || mHBid != fvHBid) flag = true;
	//	cout<<"\n============================================"
	//	    <<"\n     Miner Auction Winer "<<mHBidder
	//	    <<" |  Amount "<<mHBid;
	//	cout<<"\n fork Validator Auction Winer "<<to_string(vHBidder)
	//	    <<" |  Amount "<<to_string(vHBid);
	//	cout<<"\n============================================\n";
		return flag;
	}
}

/*************************MAIN FUN CODE BEGINS**********************/
/*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
/*!!!!!!!!          main()         !!!!!!!!!!*/
/*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
int main(int argc, char *argv[])
{
	cout<<pl<<"BTO Miner and Concurrent Validator\n";
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

	//! read from input file:: nBidder = #numProposal; nThread = #threads;
	//! numAUs = #AUs; λ = random delay seed.
	file_opr.getInp(&nBidder, &bidEndT, &nThread, &numAUs, &lemda);

	//! max Proposal shared object error handling.
	if(nBidder > maxBObj) {
		nBidder = maxBObj;
		cout<<"Max number of Proposal Shared Object can be "<<maxBObj<<"\n";
	}

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
			file_opr.genAUs(numAUs, nBidder, funInContract, listAUs);
			//! index+1 represents respective AU id, and
			//! mAUT[index] represents "time stamp (commited trans)".
			mAUT = new std::atomic<int>[numAUs];
			for(int i = 0; i< numAUs; i++) mAUT[i] = 0;
			tTime[0]  = 0, tTime[1] = 0;
			mPendingRet  = new int [nBidder+1];
			vPendingRet  = new int [nBidder+1];
			fvPendingRet = new int [nBidder+1];
			Timer mTimer;
			mTimer.start();

			//MINER
			Miner *miner = new Miner();
			miner ->mainMiner();

			if(lemda != 0) bool rv = addMFS(NumOfDoubleSTx);

			//give dependenices in the graph.
			if(nBlock > 0) {
				totalDepInG   += cGraph->print_grpah();
				totalInDegAUs += cGraph->inDegAUs(cGraph);
			}

			//VALIDATOR
			float valt = 0, fvalt = 0;
			int acceptCount  = 0, rejectCount  = 0;
			int facceptCount = 0, frejectCount = 0;
			for(int nval = 0; nval < numValidator; nval++)
			{
				valt  = 0; fvalt = 0;
				Validator     *validator  = new Validator();
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
			for( int i = 0; i < nThread; i++ ) abortCnt += aCount[i];
	//		if(nBlock > 0)cout<<"\n# of STM Transaction Aborted "<<abortCnt;

			float_t gConstT = 0;
			for(int ii = 0; ii < nThread; ii++) gConstT += gTtime[ii];
	//		cout<<"\nAvg Grpah Time= "<<gConstT/nThread<<" microseconds";
		
			//! total valid AUs among total AUs executed 
			//! by miner and varified by Validator.
			int vAUs = numAUs-aCount[0];
			if(nBlock > 0)
			file_opr.writeOpt(nBidder, nThread, numAUs, tTime, mTTime, vTTime,
			                  fvTTime, aCount, vAUs, mItrT, vItrT, fvItrT, 0);

			listAUs.clear();
			delete miner;
			miner = NULL;
			delete cGraph;
			auction = NULL;

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
