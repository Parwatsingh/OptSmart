#ifndef common_h
#define common_h

#include <iostream>                 // for cout in cpp
#include <thread>                   // for threads
#include <mutex>                    // std::mutex, std::unique_lock
#include <condition_variable>       // std::condition_variable
#include <sys/time.h>               // System time
#include <math.h>
#include <sstream>
#include <assert.h>
#include <cstring>

#define BAD_INDEX INT_MAX
#define BAD_VALUE INT_MIN
#define DEFAULT_KEY INT_MIN
#define DEFAULT_VALUE INT_MIN
#define DEFAULT_TS 0
#define DEFAULT_MARKED false
#define DEBUG_LOGS 0

typedef uint64_t uint_t;
const int TABLE_SIZE = 50;

enum OPN_NAME
{
	WRITE = 5,
	DELETE = 6,
	READ = 7,
	//error
	WRONG_OPN        = 8, //program shall not proceed
	DEFAULT_OPN_NAME = 111
};
enum OPN_STATUS
{
	ABORT  = 10,
	OK     = 11,
	FAIL   = 12,
	COMMIT = 13,
	RETRY  = 14,
	//error
	BUCKET_EMPTY      = 100,
	VARIABLE_NULL     = 101,
	WRONG_STATUS      = 102,  //program shall not proceed,
	DEFAULT_OP_STATUS = 222
};
enum VALIDATION_TYPE{
	RV,
	TRYCOMMIT
};
enum LIST_TYPE
{
	RL_BL,
	RL,
	BL
};

#define status(x) ((x == 10)? ("**ABORT**"):( (x == 11)?("OK"): ( (x ==12)?("FAIL"): ( (x == 13)?("COMMIT"): ( (x ==14)?("RETRY"):(  (x == 102)?("WRONG_STATUS"):( (x == 222)?("DEFAULT_OP_STATUS!!!"):("***SCREW") )) ) ) )))
#define opname(x) ((x == 5)?("WRITE"):( (x==6)?("DELETE"):( (x==7)?("READ"):(( x==8)?("WRONG_OPN**"):("DEFAULT_OPN_NAME")))))

#define elog(_message_)  do {fprintf(stderr,                    \
"%s():%s:%u: %s\n",        \
__FUNCTION__, __FILE__, __LINE__,    \
_message_); fflush(stderr);}while(0);


//mutex copyBytes_lock;
void copyBytes( void *a, void *b, int howMany )
{
	int i;
	char* x = (char*) a;
	char* y = (char*) b;
	for( i  = 0; i<howMany; i++)
	{
		*(x+i) = *(y+i);
	}
}

class voidVal
{
  public:
	int size;
	void *val;
	voidVal(int size)
	{
		this->size = size;
		this->val  = operator new(size);
		memset((char*)(this->val), 0, size);
	}
	~voidVal(){ }
};
#endif /* common_h */
