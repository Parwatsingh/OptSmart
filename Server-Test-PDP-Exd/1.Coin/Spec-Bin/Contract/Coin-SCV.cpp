#include "Coin-SCV.h"

//==============================================================================
//===============================Validator SC Fun===============================
//==============================================================================
void Coin::setCounterFlag(bool cFlag) {
	useCounter = cFlag;
}

void Coin::reset() {
	auto it = listAccount.begin();
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
int Coin::mint(int t_ID, int receiver_iD, int amount)
{
	if(t_ID != minter) {
		cout<<"\nERROR:: Only ''MINTER'' (Contract Creator) can initialize the Accounts (Shared Objects)\n";
		return 0; //false not a minter.
	}
	auto it = listAccount.begin();
	for(; it != listAccount.end(); it++)
		if(it->ID == receiver_iD) break;

	it->bal = amount;
	return 1;      //AU execution successful.
}

/*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
/*!!!       send() called by account holder thread.            !!!*/
/*!!!   To send some coin from his account to another account  !!!*/
/*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
int Coin::send(int sender_iD, int receiver_iD, int amount)
{
	auto sender = listAccount.begin();
	for(; sender != listAccount.end(); sender++) {
		if( sender->ID == sender_iD) {
			if(useCounter == true) {
				pthread_mutex_lock(&(sender->accLock));
				if(sender->rCount == 0 && sender->wCount == 0) {
					sender->wCount = 1;
					pthread_mutex_unlock(&(sender->accLock));
					break;
				}
				else {
					pthread_mutex_unlock(&(sender->accLock));
					return -1;//miner is malicious not given proper dependencies.
				}
			}
			else break;
		}
	}


	if( sender != listAccount.end() ) {
		if(amount > sender->bal) {
			pthread_mutex_lock(&(sender->accLock));
			sender->wCount = 0;
			pthread_mutex_unlock(&(sender->accLock));
			return -2; //not sufficent balance to send; AU is invalid.
		}
	}
	else {
		cout<<"Sender ID" << sender_iD<<" not found\n";
		return 0;//AU execution fail;
	}
	
	auto reciver = listAccount.begin();
	for(; reciver != listAccount.end(); reciver++) {
		if( reciver->ID == receiver_iD) {
			if(useCounter == true) {
				pthread_mutex_lock(&(reciver->accLock));
				if(reciver->rCount == 0 && reciver->wCount == 0) {
					reciver->wCount = 1;
					pthread_mutex_unlock(&(reciver->accLock));
					break;
				}
				else {
					sender->wCount = 0;
					pthread_mutex_unlock(&(reciver->accLock));
					return -1;//miner is malicious not given proper dependencies.
				}
			}
			else break;
		}
	}
	if( reciver != listAccount.end() ) {
		sender->bal  -= amount;
		reciver->bal += amount;

		pthread_mutex_lock(&(sender->accLock));
		pthread_mutex_lock(&(reciver->accLock));
		sender->wCount  = 0;
		reciver->wCount = 0;
		pthread_mutex_unlock(&(reciver->accLock));
		pthread_mutex_unlock(&(sender->accLock));
		return 1; //AU execution successful.
	}
	else {
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
	auto acc = listAccount.begin();
	for(; acc != listAccount.end(); acc++) {
		if( acc->ID == account_iD) {
			if(useCounter == true) {
				pthread_mutex_lock(&(acc->accLock));
				if(acc->wCount == 0) {
					acc->rCount = 1;
					pthread_mutex_unlock(&(acc->accLock));
					break;
				}
				else {
					pthread_mutex_unlock(&(acc->accLock));
					return -1;//miner is malicious not given proper dependencies.
				}
			}
			else break;
		}
	}
	if( acc != listAccount.end() ) {
		*bal = acc->bal;
		pthread_mutex_lock(&(acc->accLock));
		acc->rCount = 0;
		pthread_mutex_unlock(&(acc->accLock));
		return 1; //AU execution successful.
	}
	else {
		cout<<"Account ID" << account_iD<<" not found\n";
		return 0;//AU execution fail.
	}
}




//==============================================================================
//===============================Miner SC Fun===================================
//==============================================================================
bool Coin::mint_m(int t_ID, int receiver_iD, int amount) 
{
	if(t_ID != minter) 
	{
		cout<<"\nERROR:: Only ''MINTER'' (Contract Creator) "
			<<"can initialize the Accounts (Shared Objects)\n";
		return false; //false not a minter.
	}
	auto it = listAccountMiner.begin();
	for(; it != listAccountMiner.end(); it++)
	{
		if( (it)->ID == receiver_iD) {
			if(! (tLock[receiver_iD].try_lock()) )  return -1;			
			(it)->bal = amount;
			tLock[receiver_iD].unlock();
		}
	}
	return true; //AU execution successful.
}


int Coin::send_m(int t_ID,int sender_iD,int receiver_iD,int amount,bool binFlag)
{
	auto sender = listAccountMiner.begin();
	for(; sender != listAccountMiner.end(); sender++) {
		if( sender->ID == sender_iD) break;
	}

	if( sender != listAccountMiner.end() ) {

		if(binFlag == true) {
			bool x = tLock[sender_iD].try_lock();
			if(x == false) {
				//not sufficent balance to send; AU is invalid.
				if(amount > sender->bal) return false; // AU is inavlid.
				else return -1; // Some other Tx has done update already.
			}
			//not sufficent balance to send; AU is invalid.
			else 
				if(amount > sender->bal) return false;
		}
		else {
			//not sufficent balance to send; AU is invalid.
			if(amount > sender->bal) return false;
		}
	}
	else {
		cout<<"Sender ID" << sender_iD<<" not found\n";
		return false;//AU execution fail;
	}

	auto reciver = listAccountMiner.begin();
	for(; reciver != listAccountMiner.end(); reciver++)
		if( reciver->ID == receiver_iD) break;

	if( reciver != listAccountMiner.end() ) {
		if(binFlag == true) {
			bool x = tLock[receiver_iD].try_lock();
			if(x == false) return -1;
		}
		sender->bal  -= amount;
		reciver->bal += amount;
		return true; //AU execution successful.
	}
	else {
		cout<<"Reciver ID" << receiver_iD<<" not found\n";
		return false;// when id not found
	}
}


int Coin::get_bal_m(int account_iD, int *bal, int t_ID, bool binFlag) {
	auto acc = listAccountMiner.begin();
	for(; acc != listAccountMiner.end(); acc++)
		if( acc->ID == account_iD) break;

	if( acc != listAccountMiner.end() ) {
		if(binFlag == true) {
			bool x = tLock[account_iD].try_lock_shared();
			if(x == false) return -1;
		}
		*bal = acc->bal;
		return true; //AU execution successful.
	}
	else {
		cout<<"Account ID" << account_iD<<" not found\n";
		return false;//AU execution fail.
	}
}

void Coin::allUnlock() {
	for(int i = 0; i < 5000; i++)
		tLock[i].unlock();
}
