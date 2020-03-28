/*
* Compile:: g++ -std=c++17 GenAUs.cpp -o genAUs -O3 -g
*/

#include <iostream>
#include <fstream>
#include <string>
#include <cstring>
#include <random>
#include <vector>
#include <sstream>
#include <list>

#define LOOKUPPer 90
using namespace std;

int text[200]; //! for input
bool cmdLineArg = true;
int lemda       = 0;



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


void genAUs(int SObj, int numAUs, int nFunC, int nThreads)
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
	f_smpTry.open ("Spec-Bin/inp-output/listAUs.txt",ios::out);

	if(text[1] != 50) {
		if(text[1] == 10)
			file_log.open("Log/W2/Th10/inp-output/listAUs.txt", ios::out);
		else if(text[1] == 20)
			file_log.open("Log/W2/Th20/inp-output/listAUs.txt", ios::out);
		else if(text[1] == 30)
			file_log.open("Log/W2/Th30/inp-output/listAUs.txt", ios::out);
		else if(text[1] == 40)
			file_log.open("Log/W2/Th40/inp-output/listAUs.txt", ios::out);
		else if(text[1] == 60)
			file_log.open("Log/W2/Th60/inp-output/listAUs.txt", ios::out);
		}
	else if(text[0] != 500) {
		if(text[0] == 100)
			file_log.open("Log/W3/Sobj100/inp-output/listAUs.txt", ios::out);
		else if(text[0] == 200)
			file_log.open("Log/W3/Sobj200/inp-output/listAUs.txt", ios::out);
		else if(text[0] == 300)
			file_log.open("Log/W3/Sobj300/inp-output/listAUs.txt", ios::out);
		else if(text[0] == 400)
			file_log.open("Log/W3/Sobj400/inp-output/listAUs.txt", ios::out);
		else if(text[0] == 600)
			file_log.open("Log/W3/Sobj600/inp-output/listAUs.txt", ios::out);
	}
	else {
		if(text[2] == 50)
			file_log.open("Log/W1/AUs50/inp-output/listAUs.txt", ios::out);
		else if(text[2] == 100)
			file_log.open("Log/W1/AUs100/inp-output/listAUs.txt", ios::out);
		else if(text[2] == 150)
			file_log.open("Log/W1/AUs150/inp-output/listAUs.txt", ios::out);
		else if(text[2] == 200)
			file_log.open("Log/W1/AUs200/inp-output/listAUs.txt", ios::out);
		else if(text[2] == 250)
			file_log.open("Log/W1/AUs250/inp-output/listAUs.txt", ios::out);
		else if(text[2] == 300)
			file_log.open("Log/W1/AUs300/inp-output/listAUs.txt", ios::out);
	}
	
	int auCount = 1;
	ofstream out_file;

	//cout<<"\n---------------------";
	//cout<<"\nAU | Operations\n";
	//cout<<"---------------------\n";
	int getBal = ceil((numAUs * (LOOKUPPer)) /100);
	int sendAB = numAUs - getBal;

	int cgetBalCount = 1, SendABCount = 1;

	while(auCount <= numAUs)
	{
		//gives contract func: 1 = "send()" and 2 = "get_bal()"
		int funName;
		if(lemda != 0 && auCount < 3) funName = 1;		
		else funName = getRFunC( nFunC );

		if(funName == 1)
		{
			if(SendABCount <= sendAB)
			{
				int from = getRId(SObj);
				int to   = getRId(SObj);
				
				while (from == to) {
					to = getRId(SObj);
				}
				int ammount = getRBal( );

				string trns;
				if(lemda != 0 && auCount < 3) {
					trns = to_string(1)+" send "
					      +to_string(SObj+1)+" "
					      +to_string(SObj+2)+" "
					      +to_string(0)+"\n";

					trns = trns + to_string(2)+" send "
					            + to_string(SObj+1)+" "
					            + to_string(SObj+3)+" "
					            + to_string(0)+"\n";
					auCount++;
					SendABCount++;
				}
				else
					trns = to_string(auCount)+" send "+to_string(from)
							+" "+to_string(to)+" "+to_string(ammount)+"\n";
				//cout<<" "+trns;
				f_smpBto  << trns; f_smpMvto << trns; f_smpOstm << trns;
				f_smpMvos << trns;
				f_serial  << trns;
				f_smpSta  << trns; f_smpStm << trns;  f_smpTry  << trns;
				file_log  << trns;
				auCount++;
				SendABCount++;
			}
		}
		else if (funName == 2)
		{	
			if(cgetBalCount <= getBal)
			{
				int id      = getRId(SObj);
				string trns = to_string(auCount)+" get_bal "+to_string(id)+"\n";

				f_smpBto  << trns; f_smpMvto << trns; f_smpOstm << trns;
				f_smpMvos << trns;
				f_serial  << trns;
				f_smpSta  << trns; f_smpStm << trns;  f_smpTry  << trns;
				file_log  << trns;
				//cout<<" "+trns;
				auCount++;
				cgetBalCount++;
			}
		}
	}
	//cout<<"---------------------\n";
	//closing the file
	f_smpBto.close(); f_smpMvto.close(); f_smpOstm.close(); f_smpMvos.close();
	f_serial.close();
	f_smpSta.close(); f_smpStm.close();  f_smpTry.close();
	file_log.close();
}

void printAUl()
{
	std::ifstream input( "BTO-STM/inp-output/listAUs.txt" );
	for( std::string line; getline( input, line ); )
		cout<<line<<"\n";
	input.close();
}

int main(int argc, char *argv[])
{
	fstream f_smpBto, f_smpMvto, f_smpOstm, f_smpMvos, f_serial;
	fstream f_smpSta, f_smpStm,  f_smpTry,  file_log;


	f_smpBto.open ("BTO-STM/inp-output/inp-params.txt",    ios::out);
	f_smpMvto.open("MVTO/inp-output/inp-params.txt",       ios::out);
	f_smpOstm.open("OSTM/inp-output/inp-params.txt",       ios::out);
	f_smpMvos.open("MVOSTM/inp-output/inp-params.txt",     ios::out);
	f_serial.open ("Serial/inp-output/inp-params.txt",     ios::out);
	f_smpSta.open ("Static-Bin/inp-output/inp-params.txt", ios::out);
	f_smpStm.open ("STM-Bin/inp-output/inp-params.txt",    ios::out);
	f_smpTry.open ("Spec-Bin/inp-output/inp-params.txt",ios::out);
	

	cout << "\n===============================================================";
	cout << "\nEnter the values of the input for Coin Contract";
	cout << "\nPLEASE FOLLOW THE INSTRUCTIONS::";
	cout << "\n===============================================================";
	cout << "\nInput will be updated in <Algo Name>/inp-output/inp-params.txt";
	cout << "\nInput should be a line space seprated 4 values as given below::";
	cout << "\n---------------------------------------------------------------";
	cout << "\n5 10 1000 1\n";
	cout << "\tWhere:\n";
	cout << "\t1. m: # of SObj <Range: 2 to 5000>\n"; 
	cout << "\t2. n: # of Threads <Range: 1 to maximum available H/W threads>\n"; 
	cout << "\t3. K: # of Total AUs or Transactions "
		 <<"<Range: integer >= # Threads>\n";
	cout << "\t4. λ: if Miner is Maliciois then 1 else 0";
	cout << "\n-------------------------------------------------------------\n";

	if(cmdLineArg) {
		text[0] = atoi(argv[1]);
		text[1] = atoi(argv[2]);
		text[2] = atoi(argv[3]);
		text[3] = atoi(argv[4]);

		string input = to_string(text[0])+" "+to_string(text[1])+" "
		              +to_string(text[2])+" "+to_string(text[3]);

		cout << input << endl;
		cout << "\n======================================"
			 << "============================================\n";

		if(text[0] < 2 || text[0] > 5000)
			cout<<"\n\tError!!! # of Accounts in Coin should"
			    <<" be from Range [2, 5000]\n";

		if(text[1] <= 0)
			cout<<"\n\tError!!! # of Threads should be"
			    <<" from Range [1, max available]\n";

		if(text[2] < 1)
			cout<<"\n\t# of AUs should be an integer >= # Threads\n";

	}
	else {
		cout<<"\tPlease Enter # of Accounts in Coin"
		    <<" from <Range: 2 to 1000> :: ";
		cin>>text[0];
		while(text[0] < 2 || text[0] > 5000)
		{
			cout<<"\n\tError!!! Please Enter # of Accounts in Coin"
			    <<" from <Range: 2 to 1000> :: ";
			cin>>text[0];
			if(text[0] > 2  && text[0] < 5000 ) break;
		}


		cout << "\n\tPlease Enter # of Threads <Range: 1 to max available> :: ";
		cin>>text[1];
		while(text[1] <= 0)
		{
			cout<<"\n\tError!!! Please Enter # of Threads"
			    <<" <Range: from 1 to max available> :: ";
			cin>>text[1];
			if(text[1] > 0) break;
		}


		cout << "\n\tPlease Enter # of AUs <Range: integer >= # Threads> :: ";
		cin>>text[2];
		while(text[2] < text[1])
		{
			cout<<"\n\tError!!! Please Enter # of AUs "
			    <<"<Range: integer >= # Threads> :: ";
			cin>>text[2];
			if(text[2] >= text[1]) break;
		}


		cout << "\n\tPlease Enter value of λ <0/1> :: ";
		cin>>text[3];
		while(text[3] < 0 || text[3] > 100)
		{
			cout<<"\n\tError!!! Please Enter value of λ <0/1> :: ";
			cin>>text[3];
			if(text[3] >= 0  && text[3] <= 100 ) break;
		}
	}

	string input = to_string(text[0])+" "+to_string(text[1])+" "
	              +to_string(text[2])+" "+to_string(text[3]);

	if(text[1] != 50) {
		if(text[1] == 10)
			file_log.open("Log/W2/Th10/inp-output/inp-params.txt", ios::out);
		else if(text[1] == 20)
			file_log.open("Log/W2/Th20/inp-output/inp-params.txt", ios::out);
		else if(text[1] == 30)
			file_log.open("Log/W2/Th30/inp-output/inp-params.txt", ios::out);
		else if(text[1] == 40)
			file_log.open("Log/W2/Th40/inp-output/inp-params.txt", ios::out);
		else if(text[1] == 60)
			file_log.open("Log/W2/Th60/inp-output/inp-params.txt", ios::out);
		}
	else if(text[0] != 500) {
		if(text[0] == 100)
			file_log.open("Log/W3/Sobj100/inp-output/inp-params.txt", ios::out);
		else if(text[0] == 200)
			file_log.open("Log/W3/Sobj200/inp-output/inp-params.txt", ios::out);
		else if(text[0] == 300)
			file_log.open("Log/W3/Sobj300/inp-output/inp-params.txt", ios::out);
		else if(text[0] == 400)
			file_log.open("Log/W3/Sobj400/inp-output/inp-params.txt", ios::out);
		else if(text[0] == 600)
			file_log.open("Log/W3/Sobj600/inp-output/inp-params.txt", ios::out);
	}
	else {
		if(text[2] == 50)
			file_log.open("Log/W1/AUs50/inp-output/inp-params.txt", ios::out);
		else if(text[2] == 100)
			file_log.open("Log/W1/AUs100/inp-output/inp-params.txt", ios::out);
		else if(text[2] == 150)
			file_log.open("Log/W1/AUs150/inp-output/inp-params.txt", ios::out);
		else if(text[2] == 200)
			file_log.open("Log/W1/AUs200/inp-output/inp-params.txt", ios::out);
		else if(text[2] == 250)
			file_log.open("Log/W1/AUs250/inp-output/inp-params.txt", ios::out);
		else if(text[2] == 300)
			file_log.open("Log/W1/AUs300/inp-output/inp-params.txt", ios::out);
	}


	
	f_smpBto  << input << endl;
	f_smpMvto << input << endl;
	f_smpOstm << input << endl;
	f_smpMvos << input << endl;
	f_serial  << input << endl;
	f_smpSta  << input << endl;
	f_smpStm  << input << endl;
	f_smpTry  << input << endl;
	file_log  << input << endl;

	// Reding from file
	cout<<"\nCONGRATULATIONS!!!!\nEntered input is updated."
		<<"\nin all the directoryies for Coin Contract\n\n";
	f_smpBto >> input;
	cout << input << endl;
	cout <<"\n=============================================================\n";


	//closing the file
	f_smpBto.close();
	f_smpMvto.close();
	f_smpOstm.close();
	f_smpMvos.close();
	f_serial.close();
	f_smpSta.close();
	f_smpStm.close();
	f_smpTry.close();
	file_log.close();

	lemda = text[3]; //! if lemda is 0 miner is non-malicious else malicious.
	int Sobj = text[0];

	if(lemda != 0 && text[0] > 3 && text[2] > 2) {
		Sobj -= 3;
	}


	genAUs(Sobj, text[2], 3, text[1]);

//	printAUl();
	return 0;
}
