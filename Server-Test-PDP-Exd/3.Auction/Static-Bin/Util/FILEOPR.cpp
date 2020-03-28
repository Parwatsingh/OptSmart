#include "FILEOPR.h"

/*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
/*!!! RANDOM NUMBER GENERATER FOR ACCOUNT BALANCE !!!*/
/*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
float FILEOPR::getRBal( ) 
{
	//Random seed.
	random_device rd;
	
	//Initialize Mersenne Twister 
	//pseudo-random number generator.
	mt19937 gen(rd());
	
	//Uniformly distributed in range (1, 1000).
	uniform_int_distribution<> dis( 1, 1000 );
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
	std::random_device rd;          
	std::mt19937 gen(rd());        
	std::uniform_int_distribution<> dis(1, nCFun);
	int num = dis(gen);
	return num;
}

	
/*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
!!! genAUs() generate and store the Atomic Unites    ! 
!!! (transactions to be executed by miner/validator) !
!!! in a list & file, nFunC: parallel fun's (AUs)    !
!!! in smart contract, numAUs: number of AUs         !
!!! to be requested by client to execute.            !
!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
void FILEOPR::genAUs(int numAUs, int nBidder, int nFunC, vector<string>& ListAUs)
{
	std::ifstream input( "inp-output/listAUs.txt" );
	int count = 0;
	while(count < numAUs) {
		std::string trns; getline( input, trns );
//		cout<<trns<<"\n";
		ListAUs.push_back(trns);
		count++;
	}
	input.close();
/*
	int auCount = 1;
	ofstream out_file;
	out_file.open("inp-output/listAUs.txt");
	ListAUs.clear();
	
	while(auCount <= numAUs)
	{
		// Gives Smart contract func: 
		// 1 = "bid()"
		// 2 = "withdraw()"
		// 3 = "auction_end()"
		int funName = getRFunC( nFunC );
		if(funName == 1)
		{
			int payable  = getRBal( );
			int bidderID = getRId(nBidder);
			int bidValue = getRBal( );
			string t = to_string(auCount)+" bid "
						+to_string(payable)+" "
						+to_string(bidderID)+" "
						+to_string(bidValue)+"\n";
			out_file << t;
			ListAUs.push_back(t);
			auCount++;
		}
		else if (funName == 2)
		{
			int bidderID = getRId(nBidder);
			string t = to_string(auCount)+" withdraw "
						+to_string(bidderID)+"\n";

			out_file << t;
			ListAUs.push_back(t);
			auCount++;
		}
		else if (funName == 3)
		{
			string t = to_string(auCount)+" auction_end\n";
			
			out_file << t;
			ListAUs.push_back(t);
			auCount++;
		}
	}
	out_file.close ( );
	return;
*/
}

/*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
!!! getInp() reads #Shared Objects,  #Threads,        !
!!! #AUs, & random delay seed "Lemda" from input file !
!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
void FILEOPR::getInp(int* nBidder, int *bidEndTime, 
					 int *nThreads, int* nAUs,
					 double* lemda) 
{
	//stores input from file
	string ipBuffer[5];
	ifstream inputFile;
	inputFile.open ( "inp-output/inp-params.txt" );
	while(!inputFile)
	{
		cerr<<"Error!! Unable to open <inp-params.txt>\n";
		exit(1); //call system to stop
	}
	int i = 0;
	while( !inputFile.eof( ) )
	{
		inputFile >> ipBuffer[i];
		i++;
	}
	
	// nBidder: # of Bidder;
	*nBidder = stoi(ipBuffer[0]);
	
	// bidEndTime: Time in seconds to stop the bid;
	*bidEndTime = stoi(ipBuffer[1]);
	
	// nThreads: # of threads;
	*nThreads = stoi(ipBuffer[2]);
	
	// nAUs: Total # of AUs or Transactions;
	*nAUs = stoi(ipBuffer[3]);
	
	// λ: random delay
	*lemda = stof(ipBuffer[4]);

	inputFile.close( );
	return;
}


/*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
!!! writeOpt() stores the Time taken        !
!!! by algorithm in output file "Time.txt"  !
!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
void FILEOPR::writeOpt( int nBidder,
                        int nThreads, 
                        int nAUs,
                        double TTime[], 
                        float_t mTTime[], 
                        float_t vTTime[],
                        int aCount[], 
                        int vAUs,
                        list<float>&mIT,
                        list<float>&vIT,
                        bool vtype)
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

	out <<"\n# Bidders   \t=\t"<< nBidder
		<<"\n# Threads   \t=\t"<< nThreads
		<<"\n# Total AUs \t=\t" << nAUs
		<<"\n";


	float_t t_Time[3];
	t_Time[0] = 0;//total time miner thread
	t_Time[1] = 0;//total time validator thred

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
	out.close( );
//	return;
}
