#include "FILEOPR.h"

/*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
/*!!! RANDOM NUMBER GENERATER FOR ACCOUNT BALANCE !!!*/
/*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
float FILEOPR::getRBal( ) 
{
	random_device rd;                          //Random seed
	mt19937 gen(rd());                         //Initialize Mersenne Twister pseudo-random number generator
	uniform_int_distribution<> dis( 1, 1000 ); //Uniformly distributed in range (1, 1000)
	int num = dis(gen);
	return num;
}
	
/*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
/*!!! RANDOM NUMBER GENERATER FOR ID !!!*/
/*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
int FILEOPR::getRId( int numSObj) 
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
int FILEOPR::getRFunC( int nCFun ) 
{
	random_device rd;          
	mt19937 gen(rd());        
	uniform_int_distribution<> dis(1, nCFun);
	int num = dis(gen);
	return num;
}


/*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
!!! getInp() reads #Shared Objects, #Threads, #AUs, & !!! 
!!! random delay seed "Lemda" from input file         !!!
!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
void FILEOPR::getInp(int* m, int* n, int* K, double* lemda ) 
{
	string ipBuffer[4]; //stores input from file
	ifstream inputFile;
	inputFile.open ( "inp-output/inp-params.txt" );
	while(!inputFile)
	{
		cerr << "Error!! Unable to open inputfile <inp-params.txt>\n\n";
		exit(1); //call system to stop
	}
	int i = 0;
	while( !inputFile.eof( ) )
	{
		inputFile >> ipBuffer[i];
		i++;
	}
	*m     = stoi(ipBuffer[0]);     // m: # of SObj;
	*n     = stoi(ipBuffer[1]);     // n: # of threads;
	*K     = stoi(ipBuffer[2]);     // K: Total # of AUs or Transactions;
	*lemda = stof(ipBuffer[3]);     // λ: random delay

	inputFile.close( );
	return;
}


/*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
!!! writeOpt() stores the Time taken by algorithm in output file "Time.txt"  !
!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
void FILEOPR::writeOpt(int m,int n,int K,double total_time[],float_t mTTime[],float_t vTTime[],int aCount[],int validAUs,list<float>&mIT,list<float>&vIT)
{
	ofstream out;
	out.open("inp-output/Time.csv");
	
	float_t t_Time[2];
	t_Time[0] = 0;//total time miner thread
	t_Time[1] = 0;//total time validator thred
	
	//cout<<"=============================";
	//cout<<"\nTime Taken By Miner Threads:\n";
	out <<"\nTime Taken By Miner Threads:\n";
	//cout<<"=============================\n";
	for(int i = 0; i < n; i++) 
	{
	//	cout<<"THREAD "<< i << "\t =\t "<< mTTime[i] <<" \t microseconds\n";
		out <<"THREAD "<< i << "\t =\t "<< mTTime[i] <<" \t microseconds\n";
		t_Time[0] = t_Time[0] + mTTime[i];
	}

	//cout<<"\n\n================================";
	//cout<<"\nTime Taken By Validator Threads:\n";
	out <<"\nTime Taken By Validator Threads:\n";
	//cout<<"================================\n";
	for(int i = 0; i < n; i++) 
	{
	//	cout<<"THREAD "<< i << "\t =\t "<< vTTime[i] <<" \t microseconds\n";
		out <<"THREAD \t"<< i << "\t =\t "<< vTTime[i] <<" \t microseconds\n";
		t_Time[1] = t_Time[1] + vTTime[i];
	}
	

	out <<"\n# Shared Objects = "<< m <<" \n# Threads = "
	    << n << "\n# Total AUs = " << K << "\n";


	int total_Abort = 0;	
	for(int i = 0; i < n; i++)
	{
		total_Abort = total_Abort + aCount[i];
	}	
//	cout<<" Total Aborts = "<<total_Abort;
	out <<" # Total Aborts \t = \t"<<total_Abort<<"\n\n";
	
	//Average Time Taken by one Miner Thread = Total Time/# Threads
	out <<"\n\nAverage Time Taken by a Miner     Thread      \t  = \t "
	    <<t_Time[0]/n << " \t microseconds\n";
//	cout<<"\n    Avg Miner = "<<t_Time[0]/n<<" \t microseconds\n";
	mIT.push_back(t_Time[0]/n);

	//Average Time Taken by one Validator Thread = Total Time/# Threads
	out <<"Average Time Taken by a Validator Thread      \t  = \t "
	    <<t_Time[1]/n << " \t microseconds\n";
//	cout<<"Avg Validator = "<<t_Time[1]/n<<" \t microseconds\n";
	vIT.push_back(t_Time[1]/n);

	out.close( );
	return;
}
	
	
	
/*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
!!! genAUs() generate and store the Atomic Unites (transactions !!! 
!!! to be executed by miner/validator) in a list & file         !!!
!!! nFunC: parallel fun's (AUs) in smart contract, numAUs:      !!!
!!! number of AUs to be requested by client to execute          !!!
!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
void FILEOPR::genAUs(int numAUs, int SObj, int nFunC, vector<string>& ListAUs)
{

	std::ifstream input( "inp-output/listAUs.txt" );
	for( std::string trns; getline( input, trns ); )
	{
//		cout<<trns<<"\n";
		ListAUs.push_back(trns);
	}
	input.close();
/*
	//Random_gen r_val;
	int auCount = 1;
	ofstream out_file;
	out_file.open("inp-output/listAUs.txt");
	ListAUs.clear();
	//cout<<"\n---------------------";
	//cout<<"\nAU | Operations\n";
	//cout<<"---------------------\n";
	int getBal = ceil((numAUs * (LOOKUPPer)) /100);
	int sendAB = numAUs - getBal;
	
	int cgetBalCount = 1, SendABCount = 1;
	while(auCount <= numAUs)
	{
		int funName = getRFunC( nFunC );//gives contract func: 1 = "send()" and 2 = "get_bal()"
		if(funName == 1)
		{
			if(SendABCount <= sendAB)
			{
				int from = getRId(SObj);
				int to   = getRId(SObj);
				
				while (from == to)
				{
					to = getRId(SObj);
				}
				int ammount = getRBal( );
				
				string trns = to_string(auCount)+" send "+to_string(from)+" "+to_string(to)+" "+to_string(ammount)+"\n";
				//cout<<" "+trns;
				out_file << trns;
				ListAUs.push_back(trns);
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
				
				out_file << trns;
				//cout<<" "+trns;
				ListAUs.push_back(trns);
				auCount++;
				cgetBalCount++;
			}
		}
	}
	//cout<<"---------------------\n";
	out_file.close ( );
	return;
*/
}

/*---------------------------------------
Print Conflict List of given Atomic Unit.
---------------------------------------*/
void FILEOPR::printCList(int AU_ID, list<int>&CList)
{
	string str;
	
	for(auto it = CList.begin(); it != CList.end(); it++)
	{
		str  = str + to_string(*it)+" ";
	}	
	cout<< " AU_ID- "+to_string(AU_ID)+" Conf_list (time-stamp) [ "+str+"]\n";
	return;
}


/*--------------------------------------------------------------------------
Print Table used for Mapping AUs with Committed Trans (that has executed AU)
---------------------------------------------------------------------------*/
void FILEOPR::pAUTrns(std::atomic<int> *mAUTrns, int numAUs)
{
	cout<<"\n========================\n";
	cout << "  AU_ID |  Timestamp";
	cout<<"\n========================\n";
	
	for (int i = 0; i < numAUs; i++)
	{
		cout  <<  "   " << i+1 <<  "\t|    " << mAUTrns[i] << "\n";
	}
	cout<<"========================\n";	
	return;
}
