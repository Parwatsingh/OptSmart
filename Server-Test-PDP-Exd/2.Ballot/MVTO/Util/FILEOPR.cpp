#include "FILEOPR.h"

/*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
/*!!! RANDOM NUMBER GENERATER FOR ACCOUNT BALANCE !!!*/
/*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
float FILEOPR::getRBal( ) 
{
	random_device rd;  //Random seed
	mt19937 gen(rd()); //Initialize Mersenne Twister pseudo-random generator
	uniform_int_distribution<> dis( 1, 1000 ); //Uniformly dist in range(1,1000)
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


/*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
!!! getInp() reads #Shared Objects, #Threads, #AUs, !!!
!!! & random delay seed "Lemda" from input file     !!!
!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
void FILEOPR::getInp(int* nProposal, int* nVoter, 
                     int *nThreads, int* nAUs, double* lemda) 
{
	string ipBuffer[5]; //stores input from file
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
	*nProposal = stoi(ipBuffer[0]); // nProposal: # of Proposal;
	*nVoter    = stoi(ipBuffer[1]); // nVoter: # of Voters;
	*nThreads  = stoi(ipBuffer[2]); // nThreads: # of threads;
	*nAUs      = stoi(ipBuffer[3]); // nAUs: Total # of AUs or Transactions;
	*lemda     = stof(ipBuffer[4]); // λ: random delay

	inputFile.close( );
	return;
}


/*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
!!! writeOpt() stores the Time taken by algorithm in output file "Time.txt"  !
!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
void FILEOPR::writeOpt(int nProposal, int nVoter, int nThreads,
                       int nAUs, float TTime[],
                       float_t mTTime[], float_t vTTime[],
                       float_t fvTTime[],
                       int aCount[], int vAUs,
                       list<float>&mIT, list<float>&vIT,
                       list<float>&fvIT, bool vtype)
{
	ofstream out;
	int n = nThreads;
	out.open("inp-output/Time.csv",ios::app);

	if(vtype == false) {
		out<<"\n\n_______________________________________________"
		<<"\n||||||||||   Default Validators      ||||||||||\n"
		<<"-----------------------------------------------";
	}
	else {
		out<<"\n\n_______________________________________________"
		<<"\n||||||||||    Smart  Validators      ||||||||||\n"
		<<"-----------------------------------------------";
	}

	out <<"\n# Proposals \t=\t"<< nProposal
	    <<"\n# Voters    \t=\t"<< nVoter
		<<"\n# Threads   \t=\t"<< nThreads
		<<"\n# Total AUs \t=\t" << nAUs
		<<"\n";


	float_t t_Time[3];
	t_Time[0] = 0;//total time miner thread
	t_Time[1] = 0;//total time validator thred
	t_Time[2] = 0;//total time fork validator thred
	
	out<<"================================";
	//cout<<"\nTime Taken By Miner Threads:\n";
	out <<"\nTime Taken By Miner Threads:";
	out<<"\n================================\n";
	int mcount = 0;
	for(int i = 0; i < n; i++) 
	{
	//	cout<<"THREAD "<< i << " = "<< mTTime[i] <<" \t microseconds\n";
		if(mTTime[i] != 0) 
			out <<"THREAD "<< i << " \t = \t "
			    << mTTime[i] <<" \t microseconds\n";
		if(mTTime[i] != 0)
			mcount++;
		t_Time[0] = t_Time[0] + mTTime[i];
	}

	out<<"\n================================\n";
	//cout<<"\nTime Taken By Validator Threads:\n";
	out <<"Time Taken By Validator Threads:";
	out<<"\n================================\n";
	int dvcount = 0, fvcount = 0;
	for(int i = 0; i < n; i++) {
		if(vTTime[i] != 0)
			out <<"Dec  THREAD "<< i << " \t = \t "
			    << vTTime[i] <<" \t microseconds\n";

		if(vTTime[i] != 0)
			dvcount++;

		t_Time[1] = t_Time[1] + vTTime[i];
	}
	out<<"\n-----------------------------\n";
	for(int i = 0; i <= n; i++) {
		if(fvTTime[i] != 0)
			out <<"Fork THREAD "<< i << " \t = \t "
			    << fvTTime[i] <<" \t microseconds\n";

		if(fvTTime[i] != 0)
			fvcount++;
		t_Time[2] = t_Time[2] + fvTTime[i];
	}
	t_Time[2] = t_Time[2] + fvTTime[n];


	out<<"\n================================\n";
	int total_Abort = 0;	
	for(int i = 0; i < n; i++) {
		total_Abort = total_Abort + aCount[i];
		if(aCount[i] != 0)
			out <<"Abort by Thread "<< i <<" \t = \t "
			    <<aCount[i]<<"\n";
	}
	
	out<<"================================\n";
//	cout<<" Total Aborts = "<<total_Abort;
	out <<"# Total Aborts  \t = \t "<<total_Abort;
	out<<"\n================================\n";
	//Average Time Taken by one Miner Thread = Total Time/# Threads
	out <<"\n\nAverage Time Miner     Thread        \t = \t "
	    <<t_Time[0]/mcount << " \t microseconds\n";
//	cout<<"\n    Avg Miner = "<<t_Time[0]/n<<" \t microseconds\n";
	mIT.push_back(t_Time[0]/mcount);

	//Average Time Taken by one Validator Thread = Total Time/# Threads
	out <<"Average Time Dec  Validator Thread   \t = \t "
	    <<t_Time[1]/dvcount << " \t microseconds\n";
//	cout<<"Avg Validator = "<<t_Time[1]/n<<" \t microseconds\n";
	vIT.push_back(t_Time[1]/dvcount);

	//Average Time Taken by one fork Validator Thread = Total Time/# Threads
	out <<"Average Time Fork Validator Thread   \t = \t "
	    <<t_Time[2]/fvcount << " \t microseconds\n";
//	cout<<"Avg Validator = "<<t_Time[1]/n<<" \t microseconds\n";
	fvIT.push_back(t_Time[2]/(fvcount+1));

	out<<"********************************\n";
	out.close( );
	return;
}
	
	
	
/*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
!!! genAUs() generate and store the Atomic Unites                     !!!
!!! (transactions to be executed by miner/validator) in a list & file !!!
!!! nFunC: parallel fun's (AUs) in smart contract,                    !!!
!!! numAUs: number of AUs to be requested by client to execute        !!!
!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
void FILEOPR::genAUs(int numAUs, int numVoter, int numProposal, 
                     int nFunC, vector<string>& ListAUs)
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
	while(auCount <= numAUs)
	{
		//gives contract func: 1 = "delegate()" and 2 = "vote()"
		int funName = getRFunC( nFunC );
		if(funName == 1)
		{
				int from = getRId(numVoter);
				int to   = getRId(numVoter);
				
				while (from == to)
				{
					to = getRId(numVoter);
				}
				//int ammount = getRBal( );
				
				string t = to_string(auCount)+" delegate "
				          +to_string(from)+" "+to_string(to)+"\n";
				//cout<<" "+t;
				out_file << t;
				ListAUs.push_back(t);
				auCount++;
		}
		else if (funName == 2)
		{
				int vID  = getRId(numVoter);//voter ID
				int pID  = getRId(numProposal);//proposalID
				string t = to_string(auCount)+" vote "
				          +to_string(vID)+" "+to_string(pID)+"\n";
				
				out_file << t;
				//cout<<" "+t;
				ListAUs.push_back(t);
				auCount++;
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


/*----------------------------------------------------------------------------
Print Table used for Mapping AUs with Committed Trans (that has executed AU) !
----------------------------------------------------------------------------*/
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
