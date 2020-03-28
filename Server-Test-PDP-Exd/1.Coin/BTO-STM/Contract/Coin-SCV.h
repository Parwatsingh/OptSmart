#pragma once
#include <string>
#include "stmBTO-lib/stm_BTO.cpp"

using namespace std;

class Coin
{
	private:
		struct accNode
		{
			int ID;
			int bal;
			pthread_mutex_t accLock;// lock with each account id used at validator.
			int rCount;//increase count by transaction that is reading dataitem.
			int wCount;//increase count by transaction that is writing to dataitem.
		};
		list<accNode>listAccount;

		std::atomic<bool> useCounter;

		std::atomic<int> minter; //! contract creator

	public:
		voidVal* structTB  = new voidVal( sizeof(int) );
		LinkedHashNode* tb = new LinkedHashNode(0, 0, structTB);
		BTO* lib           = new BTO(tb);


		//! constructor (m: num of sharedObj)
		Coin(int m, int minter_id)
		{
			useCounter = false;
			minter     = minter_id; //! minter is contract creator

			for(int i = 1; i <= m; i++)
			{
				accNode acc;
				acc.ID      = i;
				acc.bal     = 0;
				acc.rCount  = 0;
				acc.wCount  = 0;
				acc.accLock = PTHREAD_MUTEX_INITIALIZER;//initialize the lock with each account id.
				listAccount.push_back(acc);
				

				list<int> conf_list;
				trans_log* txlog;
				STATUS ops, txs;
				txlog = lib->begin();

				(*(int*)tb->val) = 0;

				ops = lib->t_write(txlog, 0, i, 0, tb);
				txs = lib->bto_TryComit(txlog, conf_list, tb);
				if(ABORT == txs)
					cout<<"\nError!!Failed to create Shared Object\n";
			}
		};

		//used for malicious miner
		void reset();
		void setCounterFlag(bool cFlag);


		/*!!! STANDERED COIN CONTRACT FUNCTION FROM SOLIDITY CONVERTED IN C++ USED BY validator !!!*/
		int mint(int t_ID, int receiver_iD, int amount);      //! serial function1 for validator.
		int send(int sender_iD, int receiver_iD, int amount);//! concurrent function1 for validator.
		int get_bal(int account_iD, int *bal);               //! concurrent function2 for validator.


		/*!!! CONTRACT with 3 functions for miner return TRUE/1 if Try_Commit SUCCESS !!!*/
		bool mint_m(int t_ID, int receiver_iD, int amount, int *time_stamp);                                    //! serial function1 for miner.
		int send_m(int t_ID, int sender_iD, int receiver_iD, int amount, int *time_stamp, list<int>&conf_list); //! concurrent function1 for miner.
		bool get_bal_m(int account_iD, int *bal, int t_ID, int *time_stamp, list<int>&conf_list);               //! concurrent function2 for miner.
		
		~Coin()
		{
			delete lib;
		};//destructor
};
