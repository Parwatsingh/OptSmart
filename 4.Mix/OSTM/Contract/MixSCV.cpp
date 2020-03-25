#include "MixSCV.h"


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/*!!!FUNCTIONS FOR VALIDATOR !!!*/
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
//! RESETING TIMEPOIN FOR VALIDATOR.
void SimpleAuction::reset()
{
	beneficiaryAmount = 0;
	start = std::chrono::system_clock::now();

	list<PendReturn>::iterator pr = pendingReturns.begin();
	for(; pr != pendingReturns.end(); pr++)
		pr->value = 0;
}


/*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
/*! VALIDATOR:: Bid on the auction with the value sent together with this  !*/
/*! transaction. The value will only be refunded if the auction is not won.!*/
/*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
bool SimpleAuction::bid( int payable, int bidderID, int bidValue )
{
	// No arguments are necessary, all information is already part of trans
	// -action. The keyword payable is required for the function to be able
	// to receive Ether. Revert the call if the bidding period is over.

	auto end     = high_resolution_clock::now();
	double_t now = duration_cast<milliseconds>( end - start ).count();

	if( now > auctionEnd)
	{
//		cout<<"\nAuction already ended.";
		return false;
	}
	// If the bid is not higher, send the
	// money back.
	if( bidValue <= highestBid)
	{
//		cout<<"\nThere already is a higher bid.";
		return false;
	}
	if (highestBid != 0) 
	{
		// Sending back the money by simply using highestBidder.send(highestBid)
		// is a security risk because it could execute an untrusted contract.
		// It is always safer to let recipients withdraw their money themselves.
		//pendingReturns[highestBidder] += highestBid;
		
		list<PendReturn>::iterator pr = pendingReturns.begin();
		for(; pr != pendingReturns.end(); pr++)
		{
			if( pr->ID == highestBidder)
				break;
		}
		if(pr == pendingReturns.end() && pr->ID != highestBidder)
		{
			cout<<"\nError:: Bidder "+to_string(highestBidder)+" not found.\n";
		}
		pr->value = highestBid;
	}
	//HighestBidIncreased(bidderID, bidValue);
	highestBidder = bidderID;
	highestBid    = bidValue;

	return true;
}

/*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
/*! VALIDATOR:: Withdraw a bid that was overbid. !*/
/*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
bool SimpleAuction::withdraw(int bidderID)
{
	list<PendReturn>::iterator pr = pendingReturns.begin();
	for(; pr != pendingReturns.end(); pr++)
	{
		if( pr->ID == bidderID)
			break;
	}
	if(pr == pendingReturns.end() && pr->ID != bidderID)
	{
		cout<<"\nError:: Bidder "+to_string(bidderID)+" not found.\n";
	}

///	int amount = pendingReturns[bidderID];
	int amount = pr->value;
	if (amount > 0) 
	{
		// It is important to set this to zero because the recipient
		// can call this function again as part of the receiving call
		// before `send` returns.
///		pendingReturns[bidderID] = 0;
		pr->value = 0;
		if ( !send(bidderID, amount) )
		{
			// No need to call throw here, just reset the amount owing.
///			pendingReturns[bidderID] = amount;
			pr->value = amount;
			return false;
		}
	}
	return true;
}

/*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
/* VALIDATOR:: this fun can also be impelemted !*/
/* as method call to other smart contract. we  !*/
/* assume this fun always successful in send.  !*/
/*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
int SimpleAuction::send(int bidderID, int amount)
{
//	bidderAcount[bidderID] += amount;
	return 1;
}

/*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
/* VALIDATOR:: End the auction and send the highest bid to the beneficiary. !*/
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

	if(!ended)
	{
//		AuctionEnded(highestBidder, highestBid);
//		cout<<"\nAuctionEnd has already been called.";
		return true;
	}
	// 2. Effects
	ended = true;
//	AuctionEnded( );

	// 3. Interaction
	///beneficiary.transfer(highestBid);
	beneficiaryAmount = highestBid;
	return true;
}


void SimpleAuction::AuctionEnded( )
{
	cout<<"\n======================================";
	cout<<"\n| Auction Winer ID "<<highestBidder<<" |  Amount "<<highestBid;
	cout<<"\n======================================\n";	
}


void SimpleAuction::state(int* hBidder, int *hBid)
{
	*hBidder = highestBidder;
	*hBid    = highestBid;
}


/*~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/*!! FUNCTIONS FOR MINER !!!*/
/*~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
/*! MINER:: Bid on the auction with the value sent together with this      !*/
/*! transaction. The value will only be refunded if the auction is not won.!*/
/*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
int SimpleAuction::bid_m( int payable, int bidderID, int bidValue, 
							int *ts, list<int> &cList)
{
	voidVal* structVal = new voidVal( sizeof(int) );
	LinkedHashNode* tb = new LinkedHashNode(0, 0, structVal);

	auto end = high_resolution_clock::now();
	double_t now = duration_cast<milliseconds>( end - start ).count();

	if( now > auctionEnd)
	{
		return -1;//! invalid AUs: Auction already ended.
	}

	trans_log* txlog;
	STATUS ops;
	STATUS txs         = ABORT;
	int* highestBid    = new int;
	int* highestBidder = new int;
	*highestBid        = 0;
	*highestBidder     = 0;
	txlog              = lib->begin();
	*ts                = txlog->tid; //return time_stamp to caller.

	//! highestBid SObj id is maxBiderID+3. 
	ops = lib->t_lookup(txlog, 0, maxBiderID+3, highestBid, tb);
	if(ABORT == ops) return 0;//AU aborted.

	//! highestBidder SObj id is maxBiderID+2.
	ops = lib->t_lookup(txlog, 0, maxBiderID+2, highestBidder, tb);
	if(ABORT == ops) return 0;//AU aborted.

	if( bidValue <= *highestBid )
		return -1;//! invalid AUs: There already is a higher bid.

	// If the bid is no longer higher, send the money back to old bidder.
	if (*highestBid != 0) 
	{
		lib->t_insert(txlog, 0, *highestBidder, *highestBid, tb);//highestBidder

	}
	// increase the highest bid.
	lib->t_insert(txlog, 0, maxBiderID+2, bidderID, tb);//highestBidder
	lib->t_insert(txlog, 0, maxBiderID+3, bidValue, tb);//highestBid

	txs = lib->tryCommit(txlog, cList, tb);
	if(ABORT == txs) return 0;//AU aborted.
	else return 1;//bid successfully done; AU execution successful.
}

/*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
/*! MINER:: Withdraw a bid that was overbid. !*/
/*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
int SimpleAuction::withdraw_m(int bidderID, int *ts, list<int> &cList)
{
	voidVal* structVal = new voidVal( sizeof(int) );
	LinkedHashNode* tb = new LinkedHashNode(0, 0, structVal);

	trans_log* txlog;
	STATUS ops;
	STATUS txs     = ABORT;
	int* bidderVal = new int;
	*bidderVal     = 0;
	txlog          = lib->begin();
	*ts            = txlog->tid; //return time_stamp to caller.

	ops = lib->t_lookup(txlog, 0, bidderID, bidderVal, tb);
	if(ABORT == ops) return 0;//AU aborted.

	//int amount = pendingReturns[bidderID];
	int amount = *bidderVal;
	if(amount > 0) 
	{
		//pendingReturns[bidderID] = 0;
		bidderVal = 0;
		lib->t_insert(txlog, 0, bidderID, 0, tb);
		
		if(!send(bidderID, amount))
		{
			// No need to call throw here, just reset the amount owing.
			*bidderVal = amount;
			lib->t_insert(txlog, 0, bidderID, *bidderVal, tb);
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

	if(now < auctionEnd) return -1; //! Auction not yet ended.

	voidVal* structVal = new voidVal( sizeof(int) );
	LinkedHashNode* tb = new LinkedHashNode(0, 0, structVal);

	trans_log* txlog;
	STATUS ops;
	STATUS txs   = ABORT;
	int* endFlag = new int;
	*endFlag     = 0;
	txlog        = lib->begin();
	*ts          = txlog->tid; //return time_stamp to caller.

	ops = lib->t_lookup(txlog, 0, maxBiderID+1, endFlag, tb);
	if(ABORT == ops) return 0;//AU aborted.

	int ended = *endFlag;
	if( !ended ) return 1; //! AuctionEnd has already been called.

	lib->t_insert(txlog, 0, maxBiderID+1, 0, tb);

	int* hBidder = new int;
	int* hBid    = new int;

	ops = lib->t_lookup(txlog, 0, maxBiderID+2, hBidder, tb);
	if(ABORT == ops) return 0;//AU aborted.
	
	ops = lib->t_lookup(txlog, 0, maxBiderID+3, hBid, tb);
	if(ABORT == ops) return 0;//AU aborted.

	beneficiaryAmount = *hBid;

	txs = lib->tryCommit(txlog, cList, tb);
	if(ABORT == txs) return 0;//AU aborted.
	else return 1;//withdraw successfully done; AU execution successful.
}

bool SimpleAuction::AuctionEnded_m( )
{
	voidVal* structVal = new voidVal( sizeof(int) );
	LinkedHashNode* tb = new LinkedHashNode(0, 0, structVal);

	trans_log* txlog;
	STATUS ops;
	STATUS txs  = ABORT;
	int* hBid   = new int;
	int* hBider = new int;
	*hBid       = 0;
	*hBider     = 0;
	txlog       = lib->begin();

	ops = lib->t_lookup(txlog, 0, maxBiderID+3, hBid, tb);//! highestBid SObj
	if(ABORT == ops) return false;//AU aborted.
	
	ops = lib->t_lookup(txlog, 0, maxBiderID+2, hBider, tb);//! highestBidder SObj
	if(ABORT == ops) return false;//AU aborted.


	cout<<"\n======================================";
	cout<<"\n| Auction Winer ID "+to_string(*hBider)
			+" |  Amount "+to_string(*hBid);
	cout<<"\n======================================\n";	
	list<int> cList;
	txs = lib->tryCommit(txlog, cList, tb);
	if(ABORT == txs) return false;//AU aborted.
	else return true;//auction_end successfully done; AU execution successful.
}

void SimpleAuction::state_m(int *hBidder, int *hBid)
{
	voidVal* structVal = new voidVal( sizeof(int) );
	LinkedHashNode* tb = new LinkedHashNode(0, 0, structVal);

	trans_log* txlog;
	STATUS ops;
	STATUS txs  = ABORT;
	int* _hBid   = new int;
	int* _hBider = new int;
	*_hBid       = 0;
	*_hBider     = 0;
	txlog        = lib->begin();

	ops = lib->t_lookup(txlog, 0, maxBiderID+3, _hBid, tb);//! highestBid
	ops = lib->t_lookup(txlog, 0, maxBiderID+2, _hBider, tb);//! highestBidder
	
	*hBidder = *_hBider;
	*hBid    = *_hBid;
}



/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/*!!!functions for validator !!!*/
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
void Coin::reset()
{
	list<accNodeV>::iterator it = listAccount.begin();
	for(; it != listAccount.end(); it++){
		it->wCount = 0;
		it->rCount = 0;
	}
}


/*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
/*!!!            mint() called by minter thread.               !!!*/
/*!!!    initially credit some num of coins to each account    !!!*/
/*!!! (can be called by anyone but only minter can do changes) !!!*/
/*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
bool Coin::mint(int t_ID, int receiver_iD, int amount)
{
	if(t_ID != minter) 
	{
		cout<<"\nERROR:: Only ''MINTER'' (Contract Creator)"
			<<" can initialize the Accounts (Shared Objects)\n";
		return false; //false not a minter.
	}
	list<accNodeV>::iterator it = listAccount.begin();
	for(; it != listAccount.end(); it++)
	{
		if( (it)->ID == receiver_iD)
			(it)->bal = amount;
	}

//	account[receiver_iD] = account[receiver_iD] + amount;
	return true;      //AU execution successful.
}

/*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
/*!!!       send() called by account holder thread.            !!!*/
/*!!!   To send some coin from his account to another account  !!!*/
/*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
int Coin::send(int sender_iD, int receiver_iD, int amount)
{
	list<accNodeV>::iterator sender = listAccount.begin();
	for(; sender != listAccount.end(); sender++)
	{
		if( sender->ID == sender_iD)
		{
			pthread_mutex_lock(&(sender->accLock));
			if(sender->rCount == 0 && sender->wCount == 0)
			{
				sender->wCount = 1;
				pthread_mutex_unlock(&(sender->accLock));
				break;
			}
			else
			{
				pthread_mutex_unlock(&(sender->accLock));
				return -1;//miner is malicious not given proper dependencies.
			}
		}
	}


	if( sender != listAccount.end() )
	{
		if(amount > sender->bal)
		{
			pthread_mutex_lock(&(sender->accLock));
			sender->wCount = 0;
			pthread_mutex_unlock(&(sender->accLock));
			return -2; //not sufficent balance to send; AU is invalid.
		}
	}
	else
	{
		cout<<"Sender ID" << sender_iD<<" not found\n";
		return 0;//AU execution fail;
	}
	
	list<accNodeV>::iterator reciver = listAccount.begin();
	for(; reciver != listAccount.end(); reciver++)
	{
		if( reciver->ID == receiver_iD)
		{
			pthread_mutex_lock(&(reciver->accLock));
			if(reciver->rCount == 0 && reciver->wCount == 0)
			{
				reciver->wCount = 1;
				pthread_mutex_unlock(&(reciver->accLock));
				break;
			}
			else
			{
				sender->wCount = 0;
				pthread_mutex_unlock(&(reciver->accLock));
				return -1;//miner is malicious not given proper dependencies.
			}
		}
	}
	if( reciver != listAccount.end() )
	{
		sender->bal  -= amount;
		reciver->bal += amount;
		
		pthread_mutex_lock(&(sender->accLock));
		sender->wCount  = 0;
		pthread_mutex_unlock(&(sender->accLock));
		
		pthread_mutex_lock(&(reciver->accLock));
		reciver->wCount = 0;
		pthread_mutex_unlock(&(reciver->accLock));
		return 1; //AU execution successful.
	}
	else
	{
		pthread_mutex_lock(&(sender->accLock));
		sender->wCount = 0;
		pthread_mutex_unlock(&(sender->accLock));
		cout<<"Reciver ID" << receiver_iD<<" not found\n";
		return 0;// when id not found
	}
}

/*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
/*!!!    get_balance() called by account holder thread.        !!!*/
/*!!!       To view number of coins in his account             !!!*/
/*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
int Coin::get_bal(int account_iD, int *bal)
{	
	list<accNodeV>::iterator acc = listAccount.begin();
	for(; acc != listAccount.end(); acc++)
	{
		if( acc->ID == account_iD)
		{
			pthread_mutex_lock(&(acc->accLock));
			if(acc->wCount == 0)
			{
				acc->rCount = 1;
				pthread_mutex_unlock(&(acc->accLock));
				break;
			}
			else
			{
				pthread_mutex_unlock(&(acc->accLock));
				return -1;//miner is malicious not given proper dependencies.
			}
		}
	}
	if( acc != listAccount.end() )
	{
		*bal = acc->bal;
		pthread_mutex_lock(&(acc->accLock));
		acc->rCount = 0;
		pthread_mutex_unlock(&(acc->accLock));
		return 1; //AU execution successful.
	}
	else
	{
		cout<<"Account ID" << account_iD<<" not found\n";
		return 0;//AU execution fail.
	}
}



/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/*!!!  functions for miner   !!!*/
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
bool Coin::mint_m(int t_ID, int receiver_iD, int amount, int* time_stamp) 
{
	if(t_ID != minter)
	{
		cout<<"\nERROR:: Only ''MINTER'' (Contract Creator) can initialize the Accounts (Shared Objects)\n";
		return false;//false not a minter.
	}
	voidVal* structVal = new voidVal( sizeof(accNode) );
	LinkedHashNode* tb = new LinkedHashNode(0, 0, structVal);
	list<int> conf_list;
	trans_log* txlog;
	STATUS txs  = ABORT;
	STATUS ops  = ABORT;
	int* val    = new int;

	txlog       = lib->begin();
	*time_stamp = txlog->tid; //return time_stamp to user.

	ops = lib->t_lookup(txlog, 0, CIDStart+receiver_iD, val, tb);
	if(ABORT != ops)
	{
		*val += amount;
		(*(accNode*)tb->val).ID  = CIDStart+receiver_iD;
		(*(accNode*)tb->val).bal = amount;
		lib->t_insert(txlog, 0, CIDStart+receiver_iD, *val, tb);
		txs = lib->tryCommit(txlog, conf_list, tb);
	}
	if(ABORT == txs) return false;//AU aborted.
	else
	{
//		cout<<"Max t_id = "<<txlog->tid<<"\n";
		return true;//AU execution successful.
	}
}


int Coin::send_m(int t_ID, int sender_iD, int receiver_iD, int amount, int *time_stamp, list<int>&conf_list) 
{
	voidVal* structVal = new voidVal( sizeof(accNode) );
	LinkedHashNode* tb = new LinkedHashNode(0, 0, structVal);

	trans_log* txlog;
	STATUS txs  = ABORT;
	STATUS ops1 , ops2;
	int* Sval   = new int;
	int* Rval   = new int;
	*Sval = 0;
	*Rval = 0;
	txlog       = lib->begin();
	*time_stamp = txlog->tid; //return time_stamp to caller.

	(*(accNode*)tb->val).ID  = CIDStart+sender_iD;
	(*(accNode*)tb->val).bal = 0;

	ops1 = lib->t_lookup(txlog, 0, CIDStart+sender_iD, Sval, tb);
	if(ABORT == ops1) return 0;//AU aborted.
	*Sval = (*(accNode*)tb->val).bal;
	if(amount > *Sval) return -1;//not sufficent balance to send; AU is invalid, Trans aborted.

	(*(accNode*)tb->val).ID  = CIDStart+receiver_iD;
	(*(accNode*)tb->val).bal = 0;
	ops2 = lib->t_lookup(txlog, 0, CIDStart+receiver_iD, Rval, tb);
	*Rval = (*(accNode*)tb->val).bal;
	if(ABORT == ops2) return 0;//AU aborted.
  
	*Sval = *Sval - amount;
	*Rval = *Rval + amount;

	(*(accNode*)tb->val).ID  = CIDStart+sender_iD;
	(*(accNode*)tb->val).bal = *Sval;
	lib->t_insert(txlog, 0, CIDStart+sender_iD, *Sval, tb);

	(*(accNode*)tb->val).ID  = CIDStart+receiver_iD;
	(*(accNode*)tb->val).bal = *Rval;
	lib->t_insert(txlog, 0, CIDStart+receiver_iD, *Rval, tb);
	txs   = lib->tryCommit( txlog, conf_list, tb);
	if(ABORT == txs) return 0;//AU aborted.
	else return 1;//AU execution successful.
}


bool Coin::get_bal_m(int account_iD, int *bal, int t_ID, int *time_stamp, list<int>&conf_list) 
{
	voidVal* tb1       = new voidVal( sizeof(accNode) );
	LinkedHashNode* tb = new LinkedHashNode(0, 0, tb1);

	trans_log* txlog;
	STATUS txs  = ABORT;
	STATUS ops  = ABORT;
	int* val    = new int;
	txlog       = lib->begin();
	*time_stamp = txlog->tid; //return time_stamp to user.

	(*(accNode*)tb->val).ID  = CIDStart+account_iD;
	(*(accNode*)tb->val).bal = 0;

	ops = lib->t_lookup(txlog, 0, CIDStart+account_iD, val, tb);

	if(ABORT != ops) txs = lib->tryCommit( txlog, conf_list, tb);

	if(ABORT == txs) return false;//AU aborted.
	else
	{
		*bal = *val;
		*bal = (*(accNode*)tb->val).bal;
		return true;//AU execution successful.	
	}
}


//!~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//! fun called by the validator threads.
//!~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void Ballot::reset()
{
	list<VoterV>::iterator votr = voters.begin();
	for(; votr != voters.end(); votr++)
	{
		votr->voted    = false;
		votr->delegate = 0;
		votr->vote     = 0;
	}
	list<ProposalV>::iterator prop = proposals.begin();
	for(; prop != proposals.end(); prop++)
	prop->voteCount = 0;
}


/*----------------------------------------------------
! Validator::Give \`voter\` the right to vote on     !
! this ballot. May only be called by \`chairperson\`.!
*---------------------------------------------------*/
int Ballot::giveRightToVote(int senderID, int voter)
{
	/*! Ballot::Voter voter[Voter_Count+1];*/

	// If 'sender' is not a chairperson, return,
	// as only chairperson can give rights to vote.
	// \`throw\` terminates & reverts all changes to
	// the state and to Ether balances. It is often
	// a good idea to use this if functions are
	// called incorrectly. But watch out, this
	// will also consume all provided gas.
	list<VoterV>::iterator it = voters.begin();
	for(; it != voters.end(); it++)
	{
		if( it->ID == voter)
		{
			pthread_mutex_lock(&(it->accLock));
			if(it->rCount == 0 && it->wCount == 0)
			{
				it->wCount = 1;
				pthread_mutex_unlock(&(it->accLock));
				break;
			}
			else
			{
				pthread_mutex_unlock(&(it->accLock));
				return -1;//miner is malicious not given proper dependencies.
			}
		}
	}
	if(it == voters.end() && it->ID != voter)
	{
		cout<<"\nError:: voter "+to_string(voter)+" not found.\n";
	}

	if(senderID != chairperson || it->voted == true)
	{
		if(senderID != chairperson)
			cout<< "Error: Only chairperson can give right.\n";
		
		pthread_mutex_lock(&(it->accLock));
		it->wCount = 0;
		pthread_mutex_unlock(&(it->accLock));
		return 0;
	}
	pthread_mutex_lock(&(it->accLock));
	it->weight = 1;
	it->wCount = 0;
	pthread_mutex_unlock(&(it->accLock));
	return 1;
}

/*-----------------------------------------------------
! Validator:: Delegate your vote to the voter \`to\`. !
*----------------------------------------------------*/
int Ballot::delegate(int senderID, int to)
{
	// assigns reference.
	list<VoterV>::iterator sender = voters.begin();
	for(; sender != voters.end(); sender++)
	{
		if(sender->ID == senderID)
		{
			pthread_mutex_lock(&(sender->accLock));
			if(sender->rCount == 0 && sender->wCount == 0)
			{
				sender->wCount = 1;
				pthread_mutex_unlock(&(sender->accLock));
				break;
			}
			else
			{
				pthread_mutex_unlock(&(sender->accLock));
				return -1;//miner is malicious not given proper dependencies.
			}
		}
	}
	if(sender == voters.end() && sender->ID != senderID)
	{
		cout<<"\nError:: voter "+to_string(senderID)+" not found.\n";
	}
        
	if(sender->voted)
	{
		//cout<<"\nVoter "+to_string(senderID)+" already voted!\n";
		pthread_mutex_lock(&(sender->accLock));
		sender->wCount = 0;
		pthread_mutex_unlock(&(sender->accLock));
		return 0;//already voted.
	}
	// Forward the delegation as long as \`to\` also delegated.
	// In general, such loops are very dangerous, because if 
	// they run too long, they might need more gas than is 
	// available in a block. In this case, delegation will 
	// not be executed, but in other situations, such loops 
	// might cause a contract to get "stuck" completely.

	list<VoterV>::iterator Ito = voters.begin();
	for(; Ito != voters.end(); Ito++)
	{
		if( Ito->ID == to)
		{
			pthread_mutex_lock(&(Ito->accLock));
			if(Ito->wCount == 0)
			{
				Ito->rCount = 1;
				pthread_mutex_unlock(&(Ito->accLock));
				break;
			}
			else
			{
				pthread_mutex_unlock(&(Ito->accLock));
				return -1;//miner is malicious not given proper dependencies.
			}
		}
	}

	if(Ito == voters.end() && Ito->ID != to)
	{
		cout<<"\nError:: voter to "+to_string(to)+" not found.\n";
	}

	while( Ito->delegate != 0 && Ito->delegate != senderID ) 
	{
		pthread_mutex_lock(&(Ito->accLock));
		Ito->rCount = 0;
		pthread_mutex_unlock(&(Ito->accLock));

		to = Ito->delegate;
		for(Ito = voters.begin(); Ito != voters.end(); Ito++)
		{
			if( Ito->ID == to)
			{
				pthread_mutex_lock(&(Ito->accLock));
				if(Ito->wCount == 0)
				{
					Ito->rCount = 1;
					pthread_mutex_unlock(&(Ito->accLock));
					break;
				}
				else
				{
					pthread_mutex_unlock(&(Ito->accLock));
					return -1;//miner is malicious not given proper dependencies.
				}
			}
		}
		if(Ito == voters.end() && Ito->ID != to)
		{
			cout<<"\nError:: voter to "+to_string(to)+" not found.\n";
		}	
	}
	// We found a loop in the delegation, not allowed.
	if (to == senderID)
	{
		pthread_mutex_lock(&(sender->accLock));
		sender->wCount = 0;
		pthread_mutex_unlock(&(sender->accLock));
		
		pthread_mutex_lock(&(Ito->accLock));
		Ito->rCount    = 0;
		pthread_mutex_unlock(&(Ito->accLock));

		return -2;
	}

	// Since \`sender\` is a reference, this
	// modifies \`voters[msg.sender].voted\`
	sender->voted    = true;
	sender->delegate = to;
	
	list<VoterV>::iterator delegate = voters.begin();
	for(; delegate != voters.end(); delegate++)
	{
		if( delegate->ID == to)
		{
			pthread_mutex_lock(&(delegate->accLock));
			if(delegate->wCount == 0)
			{
				delegate->rCount = 1;
				pthread_mutex_unlock(&(delegate->accLock));
				break;
			}
			else
			{
				pthread_mutex_unlock(&(delegate->accLock));
				return -1;//miner is malicious not given proper dependencies.
			}
		}
	}
	if(delegate == voters.end() && delegate->ID != to)
	{
		cout<<"\nError:: voter to "+to_string(to)+" not found.\n";
	}
	if (delegate->voted) 
	{
		list<ProposalV>::iterator prop = proposals.begin();
		for(; prop != proposals.end(); prop++)
		{
			if( prop->ID == delegate->vote)
			{
				pthread_mutex_lock(&(prop->accLock));
				if(prop->rCount == 0 && prop->wCount == 0)
				{
					prop->wCount = 1;
					pthread_mutex_unlock(&(prop->accLock));
					break;
				}
				else
				{
					pthread_mutex_unlock(&(prop->accLock));
					return -1;//miner is malicious not given proper dependencies.
				}
			}
		}
		if(prop == proposals.end() && prop->ID != delegate->vote)
		{
			cout<<"\nError:: Proposal "+to_string(delegate->vote)+" not found.\n";
		}	
		// If the delegate already voted,
		// directly add to the number of votes
		prop->voteCount = prop->voteCount + sender->weight;
		sender->weight = 0;

		pthread_mutex_lock(&(sender->accLock));
		sender->wCount   = 0;
		pthread_mutex_unlock(&(sender->accLock));
		
		pthread_mutex_lock(&(Ito->accLock));
		Ito->rCount      = 0;
		pthread_mutex_unlock(&(Ito->accLock));

		pthread_mutex_lock(&(delegate->accLock));
		delegate->rCount = 0;
		pthread_mutex_unlock(&(delegate->accLock));

		pthread_mutex_lock(&(prop->accLock));
		prop->wCount     = 0;		
		pthread_mutex_unlock(&(prop->accLock));

		return 1;
	}
	else
	{
		// If the delegate did not voted yet,
		// add to her weight.
		delegate->weight = delegate->weight + sender->weight;

		pthread_mutex_lock(&(sender->accLock));
		sender->wCount   = 0;
		pthread_mutex_unlock(&(sender->accLock));
		
		pthread_mutex_lock(&(Ito->accLock));
		Ito->rCount      = 0;
		pthread_mutex_unlock(&(Ito->accLock));

		pthread_mutex_lock(&(delegate->accLock));
		delegate->rCount = 0;
		pthread_mutex_unlock(&(delegate->accLock));


		return 1;
	}
}

/*-------------------------------------------------------
! Validator:: Give your vote (including votes delegated !
! to you) to proposal \`proposals[proposal].name\`     .!
-------------------------------------------------------*/
int Ballot::vote(int senderID, int proposal)
{
	list<VoterV>::iterator sender = voters.begin();
	for(; sender != voters.end(); sender++)
	{
		if( sender->ID == senderID)
		{
			pthread_mutex_lock(&(sender->accLock));
			if(sender->rCount == 0 && sender->wCount == 0)
			{
				sender->wCount = 1;
				pthread_mutex_unlock(&(sender->accLock));
				break;
			}
			else
			{
				pthread_mutex_unlock(&(sender->accLock));
				return -1;//miner is malicious not given proper dependencies.
			}
		}
	}
	if(sender == voters.end() && sender->ID != senderID)
	{
		cout<<"\nError:: voter "+to_string(senderID)+" not found.\n";
	}

	if (sender->voted)
	{
		pthread_mutex_lock(&(sender->accLock));
		sender->wCount = 0;
		pthread_mutex_unlock(&(sender->accLock));
		return 0;//already voted
	}
	sender->voted = true;
	sender->vote  = proposal;

	// If \`proposal\` is out of the range of the array,
	// this will throw automatically and revert all changes.
	list<ProposalV>::iterator prop = proposals.begin();
	for(; prop != proposals.end(); prop++)
	{
		if( prop->ID == proposal)
		{
			pthread_mutex_lock(&(prop->accLock));
			if(prop->rCount == 0 && prop->wCount == 0)
			{
				prop->wCount = 1;
				pthread_mutex_unlock(&(prop->accLock));
				break;
			}
			else
			{
				pthread_mutex_unlock(&(prop->accLock));
				return -1;//miner is malicious not given proper dependencies.
			}
		}
	}
	if(prop == proposals.end() && prop->ID != proposal)
	{
		cout<<"\nError:: Proposal "+to_string(proposal)+" not found.\n";
	}
	prop->voteCount += sender->weight;
	
	pthread_mutex_lock(&(sender->accLock));
	sender->wCount = 0;
	pthread_mutex_unlock(&(sender->accLock));
	
	pthread_mutex_lock(&(prop->accLock));
	prop->wCount   = 0;
	pthread_mutex_unlock(&(prop->accLock));

	return 1;
}

/*-------------------------------------------------
! Validator:: @dev Computes the winning proposal  !
! taking all previous votes into account.        .!
-------------------------------------------------*/
int Ballot::winningProposal( )
{
	int winningProposal  = 0;
	int winningVoteCount = 0;
	for (int p = 1; p <= numPropsals; p++)//numProposal = proposals.length.
	{
		//Proposal *prop = &proposals[p];
		list<ProposalV>::iterator prop = proposals.begin();
		for(; prop != proposals.end(); prop++)
		{
			if( prop->ID == p) break;
		}
		if(prop == proposals.end() && prop->ID != p)
		{
			cout<<"\nError:: Proposal "+to_string(p)+" not found.\n";
		}
		if (prop->voteCount > winningVoteCount)
		{
			winningVoteCount = prop->voteCount;
			winningProposal  = p;
		}
	}
	cout<<"\n=======================================================\n";
	cout<<"Winning Proposal = "<<winningProposal
		<<" | Vote Count = "<<winningVoteCount;
	return winningProposal;
}

/*-----------------------------------------------------
! Validator:: Calls winningProposal() function to get !
! the index of the winner contained in the proposals  !
! array and then returns the name of the winner.      !
-----------------------------------------------------*/
void Ballot::winnerName(string *winnerName)
{
	int winnerID = winningProposal();
	list<ProposalV>::iterator prop = proposals.begin();
	for(; prop != proposals.end(); prop++)
	{
		if( prop->ID == winnerID) break;
	}
	if(prop == proposals.end() && prop->ID != winnerID)
	{
		cout<<"\nError:: Proposal "+to_string(winnerID)+" not found.\n";
	}

//	winnerName = &(prop)->name;
	cout<<" | Name = " <<*(prop->name) << " |";
	cout<<"\n=======================================================\n";
}


void Ballot::state(int ID, bool sFlag, int *ValidatorState)
{
//	cout<<"==========================\n";
	if(sFlag == false)//Proposal
	{
		list<ProposalV>::iterator prop = proposals.begin();
		for(; prop != proposals.end(); prop++)
		{
			if( prop->ID == ID)
				break;
		}
		if(prop == proposals.end() && prop->ID != ID)
		{
			cout<<"\nError:: Proposal "+to_string(ID)+" not found.\n";
		}
//		cout<<"Proposal ID \t= "  << prop->ID  <<endl;
//		cout<<"Proposal Name \t= "<< *(prop->name) <<endl;
//		cout<<"Vote Count \t= "  << prop->voteCount <<endl;
		ValidatorState[ID-1] = prop->voteCount;
//		cout<<"================================\n";
	}
	if(sFlag == true)
	{
		list<VoterV>::iterator it = voters.begin();
		for(; it != voters.end(); it++)
		{
			if( it->ID == ID)
				break;
		}
		if(it == voters.end() && it->ID != ID)
		{
			cout<<"\nError:: voter "+to_string(ID)+" not found.\n";
		}
		ValidatorState[ID-1] = it->voted;
//		cout<<"Voter ID \t= "  <<it->ID<<endl;
//		cout<<"weight \t\t= "  <<it->weight<<endl;
//		cout<<"Voted \t\t= "   <<it->voted<<endl;
//		cout<<"Delegate \t= "  <<it->delegate<<endl;
//		cout<<"Vote Index -\t= "<<it->vote<<endl;
//		cout<<"================================\n";
	}
}



//!~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//! fun called by the miner threads.
//!~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
/*----------------------------------------------------
! Miner::Give \`voter\` the right to vote on         !
! this ballot. May only be called by \`chairperson\`.!
*---------------------------------------------------*/
int Ballot::giveRightToVote_m(int senderID, int voter)
{
	voidVal* structVal   = new voidVal( sizeof(Voter) );
	LinkedHashNode* cObj = new LinkedHashNode(0, 0, structVal);
	int* val             = new int;

	(*(Voter*)cObj->val).ID = BIDStart+voter;

	list<int> conf_list;
	trans_log* txlog;
	STATUS ops, txs;
	txlog = lib->begin();

	ops = lib->t_lookup(txlog, 0, BIDStart+voter, val, cObj);
	if(ABORT == ops) return 0; // AU transaction aborted.

	if(senderID != chairperson || (*(Voter*)cObj->val).voted)
	{
//		cout<<"\nVoter "+to_string(senderID)+" already voted or your not chairperson!\n";
		return -1; //invalid AU.
	}

	(*(Voter*)cObj->val).weight = 1;
	lib->t_insert(txlog, 0, BIDStart+voter, *val, cObj);

	txs = lib->tryCommit(txlog, conf_list, cObj);

	if(ABORT == txs) return 0; //AU transaction aborted.

	else
	{
//		cout<<"Max t_id = "<<txlog->tid<<"\n";
		return 1;//valid AU: executed successfully.
	}
}


/*-------------------------------------------------
! Miner:: Delegate your vote to the voter \`to\`. !
*------------------------------------------------*/
int Ballot::delegate_m(int senderID, int to, int *ts, list<int>&cList)
{
	voidVal* structVal    = new voidVal( sizeof(Voter) );
	LinkedHashNode* sObj  = new LinkedHashNode(0, 0, structVal);
	LinkedHashNode* toObj = new LinkedHashNode(0, 0, structVal);
	int* val              = new int;

	trans_log* txlog;
	STATUS Sops, Tops, txs;

	txlog = lib->begin();
	*ts   = txlog->tid;

	Sops = lib->t_lookup(txlog, 0, BIDStart+senderID, val, sObj);
	if(ABORT == Sops) return 0; // AU transaction aborted.

	Tops = lib->t_lookup(txlog, 0, BIDStart+to, val, toObj);
	if(ABORT == Tops) return 0; // AU transaction aborted.

	if( (*(Voter*)sObj->val).voted )
	{
		return -1;//AU is invalid
	}

	(*(Voter*)toObj->val).ID = BIDStart+to;
	int delegateTO = (*(Voter*)toObj->val).delegate;
	while( delegateTO != 0 && delegateTO != BIDStart+senderID )
	{
		if(delegateTO == (*(Voter*)toObj->val).ID)
		{
			break;
		}

		(*(Voter*)toObj->val).ID = (*(Voter*)toObj->val).delegate;

		Tops = lib->t_lookup(txlog, 0, (*(Voter*)toObj->val).ID, val, toObj);
		if(ABORT == Tops) return 0; // AU transaction aborted.

		delegateTO = (*(Voter*)toObj->val).delegate;
	}

	//! We found a loop in the delegation, not allowed. 
	//! Prev while may cause loop.
	if( (*(Voter*)toObj->val).ID == (*(Voter*)sObj->val).ID) 
	{
//		lib->try_abort(T);
		return -1;//AU is invalid
	}

	(*(Voter*)sObj->val).voted    = true;
	(*(Voter*)sObj->val).delegate = (*(Voter*)toObj->val).ID;

	// If the delegate already voted, directly add to the number of votes.
	if( (*(Voter*)toObj->val).voted ) 
	{
		int votedPropsal = (*(Voter*)toObj->val).vote;

		voidVal* structVal   = new voidVal( sizeof(Proposal) );
		LinkedHashNode* pObj = new LinkedHashNode(0, 0, structVal);

		STATUS Pops = lib->t_lookup(txlog, 0, votedPropsal, val, pObj);
		if(ABORT == Pops) return 0; // AU transaction aborted.

		(*(Proposal*)pObj->val).voteCount += (*(Voter*)sObj->val).weight;

		(*(Voter*)sObj->val).weight = 0;

		lib->t_insert(txlog, 0, votedPropsal, *val, pObj);
	}
	else // If the delegate did not vote yet, add to weight.
	{
		(*(Voter*)toObj->val).weight += (*(Voter*)sObj->val).weight;
	}

	lib->t_insert(txlog, 0, (*(Voter*)sObj->val).ID, *val, sObj);
	lib->t_insert(txlog, 0, (*(Voter*)toObj->val).ID, *val, toObj);

	txs = lib->tryCommit(txlog, cList, sObj);

	if(ABORT == txs) return 0; //AU transaction aborted.

	else return 1;//valid AU: executed successfully.
}


/*---------------------------------------------------
! Miner:: Give your vote (including votes delegated !
! to you) to proposal \`proposals[proposal].name\  .!
---------------------------------------------------*/
int Ballot::vote_m( int senderID, int proposal, int *ts, list<int>&cList)
{
	voidVal* structValV    = new voidVal( sizeof(Voter) );
	LinkedHashNode* vObj   = new LinkedHashNode(0, 0, structValV);

	voidVal* structValP    = new voidVal( sizeof(Proposal) );
	LinkedHashNode* pObj   = new LinkedHashNode(0, 0, structValP);
	int* val               = new int;

	trans_log* txlog;
	STATUS Vops, Pops, txs;

	txlog = lib->begin();
	*ts   = txlog->tid;

	Vops = lib->t_lookup(txlog, 0, BIDStart+senderID, val, vObj);
	if(ABORT == Vops) return 0; // AU transaction aborted.

	if( (*(Voter*)vObj->val).voted )
	{
//		lib->try_abort(T);
		return -1;// AU is invalid.
	}

	Pops = lib->t_lookup(txlog, 0, BIDStart+numVoters+proposal, val, pObj);
	if(ABORT == Pops) return 0; // AU transaction aborted.

	(*(Voter*)vObj->val).voted = true;
	(*(Voter*)vObj->val).vote  = BIDStart+numVoters+proposal;
	(*(Proposal*)pObj->val).voteCount += (*(Voter*)vObj->val).weight;
	
//	cout<<" new vote count is "+to_string((*(Proposal*)pObj->value).voteCount);
	lib->t_insert(txlog, 0, BIDStart+senderID, *val, vObj);
	lib->t_insert(txlog, 0, BIDStart+numVoters+proposal, *val, pObj);

	txs = lib->tryCommit(txlog, cList, vObj);

	if(ABORT == txs) return 0; //AU transaction aborted.

	else return 1;//valid AU: executed successfully.
}


/*---------------------------------------------
! Miner:: @dev Computes the winning proposal  !
! taking all previous votes into account.    .!
---------------------------------------------*/
int Ballot::winningProposal_m()
{
	voidVal* structValP    = new voidVal( sizeof(Proposal) );
	LinkedHashNode* pObj   = new LinkedHashNode(0, 0, structValP);
	int* val               = new int;

	list<int> conf_list;
	trans_log* txlog;
	STATUS Pops, txs;
	txlog = lib->begin();


	int winningProposal  = 0;
	int winningVoteCount = 0;
	for(int p = (BIDStart+numVoters+1) ; p <= (BIDStart+numVoters+numPropsals); p++)//numProposal = proposals.length.
	{
		Pops = lib->t_lookup(txlog, 0, p, val, pObj);
		if(ABORT == Pops)
		{
			return 0; // AU transaction aborted.
			cout<<"\nError in reading Proposal "+to_string(p-numVoters-BIDStart)+"  State.\n";
		}
		if(winningVoteCount < (*(Proposal*)pObj->val).voteCount)
		{
			winningProposal  = p;
			winningVoteCount = (*(Proposal*)pObj->val).voteCount;
		}
//		cout<<"Proposal = " <<(p-(BIDStart+numVoters))<< " | Vote Count = "<<(*(Proposal*)pObj->val).voteCount;
	}
	txs = lib->tryCommit(txlog, conf_list, pObj);
	if(txs == ABORT)cout<<"\nError in reading Winner\n";
	cout<<"\n=======================================================\n";
	cout<<"Winning Proposal = " <<(winningProposal-numVoters-BIDStart)<< " | Vote Count = "<<winningVoteCount;
	return winningProposal;
}



/*-----------------------------------------------------
! Miner:: Calls winningProposal() function to get     !
! the index of the winner contained in the proposals  !
! array and then returns the name of the winner.      !
-----------------------------------------------------*/
void Ballot::winnerName_m(string *winnerName)
{
	voidVal* structValP    = new voidVal( sizeof(Proposal) );
	LinkedHashNode* pObj   = new LinkedHashNode(0, 0, structValP);
	int* val               = new int;

	int winningP = winningProposal_m();

	list<int> conf_list;
	trans_log* txlog;
	STATUS Pops, txs;

	txlog = lib->begin();
	Pops = lib->t_lookup(txlog, 0, winningP, val, pObj);
	if(ABORT == Pops)
		cout<<"\nError in reading Winning Proposal "+to_string(winningP-numVoters-BIDStart)+"  State.\n";

	winnerName = (*(Proposal*)pObj->val).name;

	cout<<" | Name = " <<*(*(Proposal*)pObj->val).name << " |";
	cout<<"\n=======================================================\n";
	txs = lib->tryCommit(txlog, conf_list, pObj);
}

void Ballot::state_m(int ID, bool sFlag, int *MinerState)
{
	list<int> conf_list;
	trans_log* txlog;
	STATUS Pops, txs;
	txlog = lib->begin();

//	cout<<"==========================\n";
	if(sFlag == false)//Proposal
	{
		voidVal* structValP = new voidVal( sizeof(Proposal) );
		LinkedHashNode* Obj = new LinkedHashNode(0, 0, structValP);
		int* val            = new int;

		Pops = lib->t_lookup(txlog, 0, BIDStart+numVoters+ID, val, Obj);
		if(ABORT == Pops)
		{
			cout<<"\nError in reading Winning Proposal "+to_string(ID)+"  State.\n";
			return;
		}
		MinerState[ID-1] =(*(Proposal*)Obj->val).voteCount;
//		cout<<"Proposal ID \t= "  << ID <<endl;
//		cout<<"Proposal Name \t= "<< *(*(Proposal*)Obj->val).name <<endl;
//		cout<<"Vote Count \t= "  << (*(Proposal*)Obj->val).voteCount <<endl;
//		cout<<"================================\n";
	}
	if(sFlag == true)
	{
		voidVal* structVal    = new voidVal( sizeof(Voter) );
		LinkedHashNode* Obj   = new LinkedHashNode(0, 0, structVal);
		int* val              = new int;

		Pops = lib->t_lookup(txlog, 0, BIDStart+ID, val, Obj);
		if(ABORT == Pops)
		{
			cout<<"\nError in reading Voter "+to_string(ID)+"  State.\n";
			return;
		}
		MinerState[ID-1] = (*(Voter*)Obj->val).voted;
//		cout<<"Voter ID : "  <<ID<<endl;
//		cout<<"weight : "    <<(*(Voter*)Obj->val).weight <<endl;
//		cout<<"Voted : "     <<(*(Voter*)Obj->val).voted <<endl;
//		cout<<"Delegate : "  <<(*(Voter*)Obj->val).delegate <<endl;
//		cout<<"Vote Index : "<<(*(Voter*)Obj->val).vote <<endl;
//		cout<<"================================\n";
	}
	voidVal* structVal    = new voidVal( sizeof(Voter) );
	LinkedHashNode* Obj   = new LinkedHashNode(0, 0, structVal);
	txs = lib->tryCommit(txlog, conf_list, Obj);
}
