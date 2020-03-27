#pragma once
#include <chrono>
#include "ostm-lib/ostm.cpp"
using namespace std;


class SimpleAuction
{
	public:
		std::atomic<int> maxBiderID;

		//! using OSTM protocol of STM library.
		voidVal* structVal = new voidVal( sizeof(int) );
		LinkedHashNode* tb = new LinkedHashNode(0, 0, structVal);
		OSTM* lib          = new OSTM(tb);

		// Parameters of the auction. Times are either absolute unix time-
		// -stamps (seconds since 1970-01-01) or time periods in seconds.
		std::chrono::time_point<std::chrono::system_clock> now, start;

		// beneficiary of the auction.
		int beneficiary;
		int beneficiaryAmount = 0;


		std::atomic<int> auctionEnd;

		// Current state of the auction (USED BY VALIDATOR).
		std::atomic<int> highestBidder;
		std::atomic<int> highestBid;
		struct PendReturn {
			int ID;
			int value;
		};

		struct PendReturnV {
			int ID;
			int value;
			int rCount;  //increase count by tx that is reading dataitem.
			int wCount;  //increase count by tx that is writing to dataitem.
			pthread_mutex_t accLock;//lock with each account id used at validator.
		};
		// Allowed withdrawals of previous bids.
		// mapping(address => uint) pendingReturns;
		list<PendReturn>pendingReturns;
		list<PendReturnV>pendingReturnsV;

		// Set to true at the end, disallows any change (USED BY VALIDATOR).
		std::atomic<bool> ended;

		// The following is a so-called natspec comment,recognizable by the 3
		// slashes. It will be shown when the user is asked to confirm a trans.
		// Create a simple auction with \`_biddingTime`\, seconds bidding
		// time on behalf of the beneficiary address \`_beneficiary`\.

		// CONSTRUCTOR.
		SimpleAuction( int _biddingTime, int _beneficiary, int numBidder)
		{
			maxBiderID = numBidder;

			//! USED BY VALIDATOR ONLY.
			beneficiary    = _beneficiary;
			highestBidder  = 0;
			highestBid     = 0;
			ended          = false;

			list<int> conf_list;
			trans_log* txlog;
			STATUS ops, txs;
			txlog = lib->begin();

			//t_insert(txlog, objId, key, value);
			//! \'end'\ in STM memory, USED BY MINERS.
			lib->t_insert(txlog, 0, maxBiderID+1, 0, tb);//end

			//! \'highestBidder'\ in STM memory, USED BY MINERS.
			lib->t_insert(txlog, 0, maxBiderID+2, 0, tb);//highestBidder

			//! \'highestBid'\ in STM memory, USED BY MINERS.
			lib->t_insert(txlog, 0, maxBiderID+3, 0, tb);//highestBid


			for(int b = 1; b <= numBidder; b++)
			{
				//! USED BY VALIDATORS.
				PendReturnV pret;
				pret.ID      = b;
				pret.value   = 0;
				pret.rCount  = 0;
				pret.wCount  = 0;
				pret.accLock = PTHREAD_MUTEX_INITIALIZER;//init the lock.
				pendingReturnsV.push_back(pret);

				//! initilize bidder pendingReturns[] value = 0 in STM Memory;
				//! \'pendingReturns[]'\ memory in STM memory, USED BY MINERS.
				lib->t_insert(txlog, 0, b, 0, tb);//highestBid
			}

			txs = lib->tryCommit(txlog, conf_list, tb);
			if(ABORT == txs) cout<<"\nError!!Failed to create Shared Object\n";

			start = std::chrono::system_clock::now();
			auctionEnd = _biddingTime;
		};



		//! STANDERED COIN CONTRACT FROM SOLIDITY CONVERTED IN C++ for validator
		// Bid on the auction with the value sent together with trans.
		// The value will only be refunded if the auction is not won.
		int bid( int payable, int bidderID, int bidValue );
		int withdraw(int bidderID);// Withdraw a bid that was overbid.
		// End the auction and send the highest bid to the beneficiary.
		bool auction_end();
		void AuctionEnded( );
		int send(int bidderID, int amount);
		void reset(int _biddingTime);
		void state(int *hBidder, int *hBid, int *vPendingRet);

		//! CONTRACT miner functions return TRUE/1 if Try_Commit returns true
		int bid_m( int payable, int bidderID, int bidValue, 
					int *ts, list<int> &cList);
		int withdraw_m(int bidderID, int *ts, list<int> &cList);
		int auction_end_m(int *ts, list<int> &cList);
		bool AuctionEnded_m( );
		int send_m(int bidderID, int amount);
		void state_m(int *hBidder, int *hBid, int *mPendingRet);

	~SimpleAuction()
	{
		delete lib;
		lib = NULL;
	};//destructor
};
