#pragma once
#include <iostream>
#include <fstream>
#include <sstream>
#include <atomic>
#include <vector>

class Graph
{
	public:
		//Structure for conflicting edge list of current AUs.
		struct EdgeNode {
			int au_id; // au_id = conflictig atomic unit ID
			std::atomic<EdgeNode*> next;

			// used by validator to decement in_degree
			// of Grpah_Node pointed by this ref 
			void *ref;
		};

		//Structure for graph of Atomic Units.
		struct Graph_Node {
			int  AU_ID; // AU_ID    = Atomic unit ID
			int  ts;    // ts       = Begining timestamp of AU

			// marked = logical field used for validator in parallel execultion
			bool marked     = false;

			// in_count = denotes # of incident edges on node
			std::atomic<int> in_count;

			// edgeHead = head of list of AUs on which
			// edge  is incident from current node
			EdgeNode *edgeHead  = new EdgeNode;
			EdgeNode *edgeTail  = new EdgeNode;
			std::atomic<Graph_Node*> next;

		}*g_start, *verHead, *verTail;
		
		Graph() {
			verHead        = new Graph_Node;
			verTail        = new Graph_Node;
			verHead->AU_ID = -1;         // vertex initial sentianl node
			verTail->AU_ID = 2147483647; // vertex end sentinal node
			verHead->next  = verTail;
			
			verHead->edgeHead->au_id = -1;
			verHead->edgeTail->au_id = 2147483647;
			verHead->edgeHead->next  = verHead->edgeTail;
			
			verTail->edgeHead->au_id = -1;
			verTail->edgeTail->au_id = 2147483647;
			verTail->edgeHead->next  = verTail->edgeTail;
		}; 


		void add_node(int A_ID, int ts_AU, Graph_Node **ref);

		//To add edges between atomic unit in graph
		void add_edge(int from, int to, int from_ts, int to_ts );

		//g_temp is current vertex node
		void findEWind(EdgeNode* edgeTail, EdgeNode **edg_pre, EdgeNode **edg_curr, int key);
		void findVWind(Graph_Node **ver_pre, Graph_Node **ver_curr, int key);

		int  print_grpah();
		void copy_BG(Graph *tGraph);
		void print_BG(Graph *copiedG);
		void remove_Edge(Graph *BG);
		void remove_AU_Edge(Graph *BG, int AU_ID);
		int  inDegAUs(Graph *BG);
		int concBinGen(Graph *BG, vector<int>& ListAUs);

		//destructor
		~Graph() 
		{
			delete verHead;
			verHead = NULL;
			delete verTail;
			verTail = NULL;
		 };
};
