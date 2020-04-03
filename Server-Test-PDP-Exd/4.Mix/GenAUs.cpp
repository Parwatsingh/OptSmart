/*
* Compile:: g++ -std=c++17 GenAUs.cpp -o genAUs -O3
*/

#include <iostream>
#include <fstream>
#include <string>
#include <cstring>
#include <random>
#include <vector>
#include <sstream>
#include <list>

#define CFCount 3//# methods in coin contract.
#define BFCount 5//# methods in ballot contract.
#define AFConut 6//# methods in simple auction contract.

//Out of coin AUs, what % will be get_Balance()?
#define LOOKUPPer 95

//Out of Ballot AUs, what % will be delegate()?
#define delegatePer 1

//Out of Auction AUs, what % will be bid() and withdraw()?
#define bidPer 10
#define withdrawPer 10


#define CoinIdStart 5000
#define VoterIdStart 10000 //Proposal Id is -Ve in I/P file for Bin approach


using namespace std;


int  text[200]; //! for input
bool cmdLineArg = true;
int  lemda       = 0;

/*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
/*!!! RANDOM NUMBER GENERATER FOR ACCOUNT BALANCE !!!*/
/*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
float getRBal( ) 
{
	//Random seed
	random_device rd;
	//Initialize Mersenne Twister pseudo-random number generator
	mt19937 gen(rd());
	//Uniformly distributed in range (1, 1000)
	uniform_int_distribution<> dis( 1, 1000 );
	int num = dis(gen);
	return num;
}
	
/*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
/*!!! RANDOM NUMBER GENERATER FOR ID !!!*/
/*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
int getRId( int numSObj) 
{
	random_device rd; 
	mt19937 gen(rd());
	uniform_int_distribution<> dis(1, numSObj); 
	int num = dis(gen);
	return num;
}
	
/*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
/*!!! RANDOM NUMBER GENERATER FOR FUNCTION CALL !!!*/
/*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
int getRFunC( int nCFun ) 
{
	random_device rd;          
	mt19937 gen(rd());        
	uniform_int_distribution<> dis(1, nCFun);
	int num = dis(gen);
	return num;
}

/*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
!!! genAUs() generate and store the Atomic Unites     !!!
!!! (transactions to be executed by miner/validator)  !!!
!!! in a list & file, nFunC: parallel fun's (AUs) in  !!!
!!! smart contract, numAUs: number of AUs to be       !!!
!!! requested by client to execute                    !!!
!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
void genAUs(int input[], int nCF, int nBF, int nAF)
{
	fstream f_smpBto, f_smpMvto, f_smpOstm, f_smpMvos, f_serial;
	fstream f_smpSta, f_smpStm,  f_smpTry,  file_log;

	
	f_smpBto.open ("BTO-STM/inp-output/listAUs.txt",    ios::out);
	f_smpMvto.open("MVTO/inp-output/listAUs.txt",       ios::out);
	f_smpOstm.open("OSTM/inp-output/listAUs.txt",       ios::out);
	f_smpMvos.open("MVOSTM/inp-output/listAUs.txt",     ios::out);
	f_serial.open ("Serial/inp-output/listAUs.txt",     ios::out);
	f_smpSta.open ("Static-Bin/inp-output/listAUs.txt", ios::out);
	f_smpStm.open ("STM-Bin/inp-output/listAUs.txt",    ios::out);
	f_smpTry.open ("Spec-Bin/inp-output/listAUs.txt",   ios::out);


	int Sobj = text[2]+text[3]+text[4]+text[5];


	int numAUs     = input[1]; // # of AUs or Transactions;
	int CoinSObj   = input[2]; // # Coin Account Shared Object.
	int BalPrObj   = input[3]; // # Ballot Proposal Shared Object.
	int BalVotObj  = input[4]; // # Ballot Voter Shared Object.
	int AuctBidObj = input[5]; // # Auction Bidder Shared Object.

	if(lemda != 0 && CoinSObj > 4 && numAUs > 2) {
		CoinSObj -= 3;
	}

	//Respective Contract AUs %.
	int coinAU = ceil(numAUs*.34);
	int ballAU = ceil(numAUs*.33);
	int auctAU = ceil(numAUs*.33);


	int getBal = ceil((coinAU * (LOOKUPPer)) /100);
	int sendAB = coinAU - getBal;
	int cgetBalCount = 1, SendABCount = 1;

	int delgC = ceil((ballAU * (delegatePer)) /100);
	int voterC = ballAU - delgC;
	int local_delgC = 1, local_voterC = 1;

	int bidC = ceil((auctAU * (bidPer)) /100);
	int withdrawC = ceil((auctAU * (withdrawPer)) /100);
	int endC = auctAU - (bidC + withdrawC);
	int local_bidC = 1, local_withdrawC = 1, local_endC = 1;


	int auCount = 1;
	int cCount = 1, bCount = 1, aCount = 1;
	while(auCount <= numAUs)
	{	
		if(cCount <= coinAU)//Coin contract
		{
			int funName;
			if(lemda != 0 && auCount < 3) funName = 1;
			else funName = getRFunC( nCF );
			if(funName == 1 )
			{
				if(SendABCount <= sendAB)
				{
					int from = getRId(CoinSObj);
					int to   = getRId(CoinSObj);
				
					while (from == to) to = getRId(CoinSObj);

					int ammount = getRBal( );

					string trns;
					if(lemda != 0 && auCount < 3) {
						trns = to_string(1)+" send "
						      +to_string(CoinSObj+1)+" "
						      +to_string(CoinSObj+2)+" "
						      +to_string(0)+"\n";

						trns = trns + to_string(2)+" send "
						      +to_string(CoinSObj+1)+" "
						      +to_string(CoinSObj+3)+" "
						      +to_string(0)+"\n";
					}
					else {
						trns = to_string(auCount)+" send "
						      +to_string(from)+" "
						      +to_string(to)+" "
						      +to_string(ammount)+"\n";
					}

					f_smpBto  << trns; f_smpMvto << trns; f_smpOstm << trns;
					f_smpMvos << trns;
					f_serial  << trns;
					f_smpStm  << trns;
					file_log  << trns;



					if(lemda != 0 && auCount < 3) {
						trns = to_string(1)+" send "
						      +to_string(CoinSObj+1+CoinIdStart)+" "
						      +to_string(CoinSObj+2+CoinIdStart)+" "
						      +to_string(0)+"\n";

						trns = trns + to_string(2)+" send "
						            + to_string(CoinSObj+1+CoinIdStart)+" "
						            + to_string(CoinSObj+3+CoinIdStart)+" "
						            + to_string(0)+"\n";
						auCount++;
						cCount++;
						SendABCount++;
					}
					else {
						trns = to_string(auCount)+" send "
							  +to_string(from+CoinIdStart)+" "
							  +to_string(to+CoinIdStart)
							  +" "+to_string(ammount)+"\n";
					}

					f_smpSta  << trns;
					f_smpTry  << trns;

					auCount++;
					cCount++;
					SendABCount++;
				}
			}
			else if (funName == 2)
			{
				if(cgetBalCount <= getBal)
				{
					int id      = getRId(CoinSObj);
					string trns = to_string(auCount)+" get_bal "
					             +to_string(id)+"\n";
				
					f_smpBto  << trns; f_smpMvto << trns; f_smpOstm << trns;
					f_smpMvos << trns;
					f_serial  << trns;
					f_smpStm  << trns;
					file_log  << trns;


					trns = to_string(auCount) +" get_bal "
					      +to_string(id+CoinIdStart)+"\n";

					f_smpSta  << trns;
					f_smpTry  << trns;

					auCount++;
					cCount++;
					cgetBalCount++;
				}
			}
		}
		if(bCount <= ballAU)//Ballot Contract
		{
			//gives contract func: 1 = "delegate()" and 2 = "vote()"
			int funName = getRFunC( nBF );
			if(funName == 1)
			{
				if(local_delgC <= delgC)
				{
					int from = getRId(BalVotObj);
					int to   = getRId(BalVotObj);
			
					while (from == to) to = getRId(BalVotObj);

					string trns = to_string(auCount)+" delegate "+
								to_string(from)+" "+to_string(to)+"\n";
				
					f_smpBto  << trns; f_smpMvto << trns; f_smpOstm << trns;
					f_smpMvos << trns;
					f_serial  << trns;
					f_smpStm  << trns;
					file_log  << trns;


					trns = to_string(auCount)+" delegate "+
								to_string(from+VoterIdStart)+" "
								+to_string(to+VoterIdStart)+"\n";

					f_smpSta  << trns;
					f_smpTry  << trns;

					auCount++;
					bCount++;
					local_delgC++;
				}
			}
			else if (funName == 2)
			{
				if(local_voterC <= voterC)
				{
					int vID  = getRId(BalVotObj);//voter ID
					int pID  = getRId(BalPrObj);//proposalID
					string trns = to_string(auCount)+" vote "+
									to_string(vID)+" "+to_string(pID)+"\n";

					f_smpBto  << trns; f_smpMvto << trns; f_smpOstm << trns;
					f_smpMvos << trns;
					f_serial  << trns;
					file_log  << trns;


					trns = to_string(auCount)+" vote "+
					       to_string(vID)+" "+
					       to_string(-pID)+"\n";

					f_smpStm  << trns;


					trns = to_string(auCount)+" vote "+
					       to_string(vID+VoterIdStart)+" "+
					       to_string(-pID)+"\n";

					f_smpSta  << trns;
					f_smpTry  << trns;

					auCount++;
					bCount++;
					local_voterC++;
				}
			}
		}
		if(aCount <= auctAU)//Auction Contract
		{
			// Gives Smart contract func: 
			// 1 = "bid()"
			// 2 = "withdraw()"
			// 3 = "auction_end()"
			int funName = getRFunC( nAF );
			if(funName == 1)
			{
				if(local_bidC <= b
