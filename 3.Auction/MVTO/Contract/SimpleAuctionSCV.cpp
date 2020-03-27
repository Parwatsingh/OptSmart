#include "SimpleAuctionSCV.h"

//==============================================================================
//===========================Validator SC Fun===================================
//==============================================================================
//! RESETING State
void SimpleAuction::reset(int _biddingTime)
{
	beneficiaryAmount = 0;
	start = std::chrono::system_clock::now();
	auctionEnd = _biddingTime;
	
	auto pr = pendingReturnsV.begin();
	for(; pr != pendingReturnsV.end(); pr++) pr->value = 0;
}


/*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
/*!         Bid on the auction with the value sent together with this      !*/
/*! transaction. The value will only be refunded if the auction is not won.!*/
/*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
int SimpleAuction::bid( int payable, int bidderID, int bidValue )
{
	// No arguments are necessary, all information is already part of trans
	// -action. The keyword payable is required for the function to be able
	// to receive Ether. Revert the call if the bidding period is over.
	
	auto end     = high_resolution_clock::now();
	double_t now = duration_cast<milliseconds>( end - start ).count();

	if( now > auctionEnd)
	{
//		cout<<"\nAuction already ended.";
		return 0;
	}
	// If the bid is not higher, send the money back.
	if( bidValue <= highestBid)
	{
//		cout<<"\nThere already is a higher bid.";
		return 0;
	}
	if (highestBid != 0) 
	{
		// Sending back the money by simply using highestBidder.send(highestBid)
		// is a security risk because it could execute an untrusted contract.
		// It is always safer to let recipients withdraw their money themselves.
		auto pr = pendingReturnsV.begin();
		for(; pr != pendingReturnsV.end(); pr++)
		{
			if( pr->ID == highestBidder)
			{
				pthread_mutex_lock(&(pr->accLock));
				if(pr->rCount == 0 && pr->wCount == 0)
				{
					pr->wCount = 1;
					pthread_mutex_unlock(&(pr->accLock));
					break;
				}
				else
				{
					pthread_mutex_unlock(&(pr->accLock));
					return -1;//miner is malicious not given proper dependencies.
				}
			}
		}
		if(pr == pendingReturnsV.end() && pr->ID != highestBidder){
//			cout<<"\nError:: Bidder "+to_string(highestBidder)+" not found.\n";
			return 0;
		}
		pthread_mutex_lock(&(pr->accLock));
		pr->value  = highestBid;
		pr->wCount = 0;
		pthread_mutex_unlock(&(pr->accLock));
	}
	//HighestBidIncreased(bidderID, bidValue);
	if( bidValue > highestBid)
	{
		highestBidder = bidderID;
		highestBid    = bidValue;
	}
	return 1;
}

/*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
/*! Withdraw a bid that was overbid. !*/
/*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
int SimpleAuction::withdraw(int bidderID)
{
	auto pr = pendingReturnsV.begin();
	for(; pr != pendingReturnsV.end(); pr++)
	{
		if( pr->ID == bidderID)
		{
			pthread_mutex_lock(&(pr->accLock));
			if(pr->rCount == 0 && pr->wCount == 0)
			{
				pr->wCount = 1;
				pthread_mutex_unlock(&(pr->accLock));
				break;
			}
			else
			{
				pthread_mutex_unlock(&(pr->accLock));
				return -1;//miner is malicious not given proper dependencies.
			}
		}
	}
	if(pr == pendingReturnsV.end() && pr->ID != bidderID) {
//		cout<<"\nError:: Bidder "+to_string(bidderID)+" not found.\n";
		return 0;
	}

	int amount = pr->value;
	if (amount > 0) 
	{
		// It is important to set this to zero because the recipient
		// can call this function again as part of the receiving call
		// before `send` returns.
		pr->value = 0;
		if ( !send(bidderID, amount) )
		{
			// No need to call throw here, just reset the amount owing.
			pr->value = amount;
			pthread_mutex_lock(&(pr->accLock));
			pr->wCount = 0;
			pthread_mutex_unlock(&(pr->accLock));
			return 0;
		}
	}
	pthread_mutex_lock(&(pr->accLock));
	pr->wCount = 0;
	pthread_mutex_unlock(&(pr->accLock));
	return 1;
}

/*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
/*        This fun can also be impelemted      !*/
/* as method call to other smart contract. we  !*/
/* assume this fun always successful in send.  !*/
/*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
int SimpleAuction::send(int bidderID, int amount)
{
//	bidderAcount[bidderID] += amount;
	return 1;
}

/*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
/*          End the auction and send the highest bid to the beneficiary.    !*/
/*!_________________________________________________________________________!*/
/*! It's good guideline to structure fun that interact with other contracts !*/
/*! (i.e. they call functions or send Ether) into three phases: 1.checking  !*/
/*! conditions, 2.performing actions (potentially changing conditions), 3.  !*/
/*! interacting with other contracts. If these phases mixed up, other cont- !*/
/*! -ract could call back into current contract & modify state or cause     !*/
/*! effects (ether payout) to be performed multiple times. If fun called    !*/
/*! internally include interaction with external contracts, they also have  !*/ 
/*! to be considered interaction with external contracts.                   !*/
/*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
bool SimpleAuction::auction_end()
{
	// 1. Conditions
	auto end     = high_resolution_clock::now();
	double_t now = duration_cast<milliseconds>( end - start ).count();
	if(now < auctionEnd) return false;
	if(!ended) return true;
	// 2. Effects
	ended = true;
	// 3. Interaction
	///beneficiary.transfer(highestBid);
	beneficiaryAmount = highestBid;
	return true;
}


void SimpleAuction::AuctionEnded( )
{
	cout<<"\n======================================";
	cout<<"\n| Auction Winer ID "+to_string(highestBidder)
			+" |  Amount "+to_string(highestBid);
	cout<<"\n======================================\n";	
}



void SimpleAuction::state(int* hBidder, int *hBid, int *vPendingRet)
{
	*hBidder = highestBidder;
	*hBid    = highestBid;
	auto pr  = pendingReturnsV.begin();
	for(; pr != pendingReturnsV.end(); pr++) vPendingRet[pr->ID] = pr->value;
}


//==============================================================================
//===============================Miner SC Fun===================================
//==============================================================================
/*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
/*! MINER:: Bid on the auction with the value sent together with this      !*/
/*! transaction. The value will only be refunded if the auction is not won.!*/
/*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
int SimpleAuction::bid_m( int payable, int bidderID, int bidValue, 
							int *ts, list<int> &cList)
{
	auto end = high_resolution_clock::now();
	double_t now = duration_cast<milliseconds>( end - start ).count();

	if( now > auctionEnd) return -1;//! Auction already ended.

	voidVal* tb = new voidVal( sizeof(int) );
	L_txlog* txlog;
	OPN_STATUS ops;
	OPN_STATUS txs     = ABORT;
	int* highestBid    = new int;
	int* highestBidder = new int;
	*highestBid        = 0;
	*highestBidder     = 0;
	txlog              = lib->begin();
	*ts                = txlog->L_tx_id; //return time_stamp to caller.
	
	//! highestBid SObj id is maxBiderID+3. 
	ops = lib->tx_read(txlog, maxBiderID+3, highestBid, tb);
	if(ABORT == ops)
		return 0;//AU aborted.
	*highestBid = (*(int*)tb->val);

	//! highestBidder SObj id is maxBiderID+2.
	ops = lib->tx_read(txlog, maxBiderID+2, highestBidder, tb);
	if(ABORT == ops) return 0;//AU aborted.
	*highestBidder = (*(int*)tb->val);

	if( bidValue <= *highestBid )
		return -1;//! invalid AUs: There already is a higher bid.

	// If the bid is no longer higher, send the money back to old bidder.
	if (*highestBid != 0) 
	{
		(*(int*)tb->val) = *highestBid;
		lib->tx_write(txlog, *highestBidder, *highestBid, tb);//highestBidder

	}
	// increase the highest bid.
	(*(int*)tb->val) = bidderID;//highestBidder
	lib->tx_write(txlog, maxBiderID+2, bidderID, tb);//highestBidder

	(*(int*)tb->val) = bidValue;//highestBid
	lib->tx_write(txlog, maxBiderID+3, bidValue, tb);//highestBid

	txs = lib->tryCommit(txlog, cList, tb);
	if(ABORT == txs) return 0;//AU aborted.
	else return 1;//bid successfully done; AU execution successful.
}

/*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
/*! MINER:: Withdraw a bid that was overbid. !*/
/*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
int SimpleAuction::withdraw_m(int bidderID, int *ts, list<int> &cList)
{
	voidVal* tb = new voidVal( sizeof(int) );
	L_txlog* txlog;
	OPN_STATUS ops;
	OPN_STATUS txs = ABORT;
	int* bidderVal = new int;
	*bidderVal     = 0;
	txlog          = lib->begin();
	*ts            = txlog->L_tx_id; //return time_stamp to caller.
	
	ops = lib->tx_read(txlog, bidderID, bidderVal, tb);
	if(ABORT == ops) return 0;//AU aborted.

//	int amount = *bidderVal;
	int amount = (*(int*)tb->val);
	if(amount > 0) 
	{
		//pendingReturns[bidderID] = 0;
		bidderVal = 0;
		lib->tx_write(txlog, bidderID, 0, tb);
		
		if(!send(bidderID, amount))
		{
			// No need to call throw here, just reset the amount owing.
			*bidderVal = amount;
			(*(int*)tb->val) = amount;
			lib->tx_write(txlog, bidderID, *bidderVal, tb);
			txs = lib->tryCommit(txlog, cList, tb);
			if(ABORT == txs) return 0;//AU aborted.
			else return -1;//AU invalid.
		}
	}
	txs = lib->tryCommit(txlog, cList, tb);
	if(ABORT == txs) return 0;//AU aborted.
	else return 1;//withdraw successfully done; AU execution successful.
}


/*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
/* MINER:: End the auction and send the highest bid to the beneficiary. !*/
/*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
int SimpleAuction::auction_end_m(int *ts, list<int> &cList)
{
	auto end     = high_resolution_clock::now();
	double_t now = duration_cast<milliseconds>( end - start ).count();

	if(now < auctionEnd)
		return -1; //! Auction not yet ended.

	voidVal* tb = new voidVal( sizeof(int) );
	L_txlog* txlog;
	OPN_STATUS ops;
	OPN_STATUS txs = ABORT;
	int* endFlag   = new int;
	*endFlag       = 0;
	txlog          = lib->begin();
	*ts            = txlog->L_tx_id; //return time_stamp to caller.

	ops = lib->tx_read(txlog, maxBiderID+1, endFlag, tb);
	if(ABORT == ops) return 0;//AU aborted.

//	int ended = *endFlag;
	int ended = (*(int*)tb->val);
	if( !ended ) return 1; //! AuctionEnd has already been called.

	(*(int*)tb->val) = 0;
	lib->tx_write(txlog, maxBiderID+1, 0, tb);

	int* hBidder = new int;
	int* hBid    = new int;

	ops = lib->tx_read(txlog, maxBiderID+2, hBidder, tb);
	if(ABORT == ops) return 0;//AU aborted.
	*hBidder = (*(int*)tb->val);
	
	ops = lib->tx_read(txlog, maxBiderID+3, hBid, tb);
	if(ABORT == ops) return 0;//AU aborted.
	*hBid = (*(int*)tb->val);

	beneficiaryAmount = *hBid;

	txs = lib->tryCommit(txlog, cList, tb);
	if(ABORT == txs) return 0;//AU aborted.
	else return 1;//withdraw successfully done; AU execution successful.
}

bool SimpleAuction::AuctionEnded_m( )
{
	voidVal* tb  = new voidVal( sizeof(int) );
	voidVal* tb1 = new voidVal( sizeof(int) );
	L_txlog* txlog;
	OPN_STATUS ops;
	OPN_STATUS txs  = ABORT;
	int* hBid       = new int;
	int* hBider     = new int;
	*hBid           = 0;
	*hBider         = 0;
	txlog           = lib->begin();

	ops = lib->tx_read(txlog, maxBiderID+3, hBid, tb);//! highestBid SObj
	if(ABORT == ops) return false;//AU aborted.
	*hBid = (*(int*)tb->val);

	ops = lib->tx_read(txlog, maxBiderID+2, hBider, tb1);//! highestBidder SObj
	if(ABORT == ops) return false;//AU aborted.
//	*hBider = (*(int*)tb1->val);

	cout<<"\n======================================";
	cout<<"\n| Auction Winer ID "+to_string(*hBider)
			+" |  Amount "+to_string(*hBid);
	cout<<"\n======================================\n";	
	list<int> cList;
	txs = lib->tryCommit(txlog, cList, tb);
	if(ABORT == txs) return false;//AU aborted.
	else return true;//auction_end successfully done; AU execution successful.
}

void SimpleAuction::state_m(int *hBidder, int *hBid, int *mPendingRet)
{
	voidVal* tb  = new voidVal( sizeof(int) );
	voidVal* tb1 = new voidVal( sizeof(int) );
	L_txlog* txlog;
	OPN_STATUS ops;
	OPN_STATUS txs  = ABORT;
	int* _hBid       = new int;
	int* _hBider     = new int;
	*_hBid           = 0;
	*_hBider         = 0;
	txlog           = lib->begin();

	ops = lib->tx_read(txlog, maxBiderID+3, _hBid, tb);//! highestBid

	ops = lib->tx_read(txlog, maxBiderID+2, _hBider, tb1);//! highestBidder

	*hBid    = *_hBid;
	*hBidder = *_hBider;

	int* val      = new int;
	for(int b = 1; b <= maxBiderID; b++) {
		*(int*)tb->val = b;
		lib->tx_read(txlog, b, val, tb);
		mPendingRet[b] = *(int*)tb->val;
//		cout<<"Bidder ID "<<b<<" value to return "<< mPendingRet[b]<<endl;
	}
}
