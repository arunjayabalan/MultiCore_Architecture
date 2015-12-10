/*
// File: Assignment_2.cpp
//              
// Framework to implement Task 1 of the Advances in Computer Architecture lab 
// session. This uses the ACA 2009 library to interface with tracefiles which
// will drive the read/write requests
// Assignment 2
//
// Extending the code from Task 1, you are to simulate a multiprocessing system with a shared memory
// architecture. Your simulation should be able to support multiple processors, all working in parallel.
// Each of the processors is associated with a local cache, and all cache modules are connected to a
// single bus. A main memory is also connected to the bus, where all the correct data is supposed to
// be stored, although, again, the memory and its contents does not have to be simulated. Assume a
// pipelined memory which can handle a request from each processor in parallel with the same latency.
// In order to make cache data coherent within the system, implement a bus snooping protocol. This
// means that each cache controller connected to the bus receives every memory request on the bus made
// by other caches, and makes the proper modification on its local cache line state. In this task, you
// should implement the simplest VALID-INVALID protocol. The cache line state transition diagram for
// this protocol is shown in Figure 1.
//
//
*/

#include "aca2009.h"
#include <systemc.h>
#include <iostream>

#define TAG_CHECK 0xFFFFF000
#define SET_CHECK 0x00000FE0
#define OFF_CHECK 0x0000001F

#define READ 0
#define WRITE 1

using namespace std;

long BusRead;
long BusWrites;

static const int MAX_CACHE_ENTRIES 	= 128;
static const int MAX_CACHE_SIZE 	= 32768; 	//32KB
static const int CACHE_LINE_SIZE 	= 32; 		//32 bytes
static const int MAX_SET 			= 8;


typedef struct 
    { 
	//cache fields - Data, Valid, tag
        unsigned int c_data[8];
        unsigned int c_valid:1;
        unsigned int c_tag:20;
    } cache_data;
   
typedef struct 
    {
        unsigned int lru:7;                     	// 7 bit representation
        cache_data way[MAX_SET];        		// 8

    } cache_set;

//unsigned int Memory_addr[12];
int max_cpus = 0;
/* Bus interface, modified version from assignment. */
class Bus_if : public virtual sc_interface 
{
    public:
        virtual bool read(int writer, int addr) = 0;
        virtual bool write(int writer, int addr, int data) = 0;
		virtual bool writeX(int writer, int addr, int data) = 0;
		virtual bool busLock() = 0;
		virtual bool busUnlock() = 0;
};


	
SC_MODULE(Cache) 
{

    public:
    	enum Function 
    	{
	    	//FUNC_INVALID,
            FUNC_READ,
            FUNC_WRITE,
        };

    	enum RetCode 
    	{
            RET_READ_DONE,
            RET_WRITE_DONE,
    	};
	
    	enum Bus_Req 
        {
	    	BUS_RD,
	    	BUS_WR,
	    	BUS_RdX,
	    	BUS_INVALID,
        };
	
		/* Possible line states depending on the cache coherence protocol. */
    	enum Line_State 
    	{
            INVALID,
            VALID,
    	};

    	sc_in<bool>     Port_CLK;
    	sc_in<Function> Port_Func;
    	sc_in<int>      Port_Addr;
    	sc_out<RetCode> Port_Done;
    	sc_inout_rv<32> Port_Data;
	
    	//Bus Address declaration
    	sc_inout_rv<32>	Port_BusAddr;
    	sc_in<int>      Port_BusWriter;
    	//sc_in<Function> Port_BusValid;
    	sc_in<Bus_Req> Port_BusValid;

    	/* Bus requests ports. */
    	sc_port<Bus_if> Port_Bus;

    	/* Variables. */
    	int cache_id;
		int snooping;

    	void update_lru(int line, int way);
    	int lru_call(int line);
    	int cache_operation(int cache_id, int addr, int mode);


    	SC_CTOR(Cache) 
    	{
		/* Create threads for handling data and snooping the bus. */
		// this is very important, and is the heart of assignment 2: to have two independent threads, one to snoop the bus, and the other to handle data.
		// your code should be mainly to implement these two threads.
            SC_THREAD(bus);
            SC_THREAD(execute);
		
            sensitive << Port_CLK.pos();
            dont_initialize();
		
            m_data = new cache_set[MAX_CACHE_SIZE /( MAX_SET * CACHE_LINE_SIZE)];
	    
	    	//for (int k = 0; k< 160; k++)
	    	//	Memory_Data[k] = 0; 	

            for (int i = 0; i < MAX_CACHE_ENTRIES ; i++)
            {
                for (int j = 0; j < MAX_SET ; j++)
            	{
                    m_data[i].way[j].c_tag = 0xFFFFF;
		    		m_data[i].way[j].c_valid = 0; // set cache line to invalid
            	}
            }	
        }		

        ~Cache() 
        {
            delete[] m_data;
        }

    private:

    	cache_set* m_data;
    	int data;
    	/* Thread that handles the bus. */
    	int BusReads;
    	int BusWrites;
    	void bus() 
    	{
            //int f = 0;

            /* Continue while snooping is activated. */
            while(true)
            {
            	/* Wait for work. */
				cout << "cache_id:"<< cache_id << " waiting for Port_BusValid to change" << endl;
            	wait(Port_BusValid.value_changed_event());
	    		int snooper = Port_BusWriter.read();
		
				cout << "cache_id:" << cache_id << " - Port_BusValid changed" << endl;
				if (snooper != cache_id)
				{ 
	    			int addr = Port_BusAddr.read().to_int();
	    			unsigned int tag = (addr & TAG_CHECK) >> 12;
	    			unsigned int set = (addr & SET_CHECK) >> 5;
	    			//unsigned int word = (addr & OFF_CHECK) >> 2;
	
	    			//Function req = Port_BusValid.read(); 
	    			Bus_Req req = Port_BusValid.read(); 
					//cout << "Port reading is done" << endl;
	    			cout << "Cache id    :" << cache_id << endl;
	    			cout << "Bus Request :" << req << endl;
					cout << "Bus snooper :" << snooper <<endl;
            		/* Possibilities. */
            		switch(Port_BusValid.read())
            		{
					// your code of what to do while snooping the bus
					// keep in mind that a certain cache should distinguish between bus requests made by itself and requests made by other caches.
					// count and report the total number of read/write operations on the bus, in which the desired address (by other caches) is found in the snooping cache (probe read hits and probe write hits).
					//Function req = Port_BusValid.read();
		 				case BUS_RD:
						{
			    			//Does normal Read Hit or Miss
			    			cout << "*** Executing BUS_RD command ***" << endl;
			    			BusReads++;
			    			break;
						}
						case BUS_RdX:
						case BUS_WR:
						{
			     			//	update memory
			     			//	invalidate other caches if tag match
			     			//	update local cache
			     			//  cache[i]->cache_id = i;
			     			//	cache[i]->snooping = snooping;
			     			cout << "*** Executing BUS_WR/BUS_RdX command ***" << endl;
			     			if (snooper != cache_id)
			     			{
								cout << "Snooper is not same as cache id" <<endl;
  			         			for (int i=0; i < MAX_SET; i++)
				 				{
				     				if ( m_data[set].way[i].c_valid == 1 )
				     				{
				         				if ( m_data[set].way[i].c_tag == tag)
					 					{
					     					m_data[set].way[i].c_valid = 0;
					 					}
				      				}
				  				}
							}
							BusWrites++;
							break;
						}
						case BUS_INVALID:
						{
			    			cout << "----------------------------------------------------" << endl;
			    			cout << "----   *** Invalid BUS command is observed ***  ----" << endl;
			    			cout << "----------------------------------------------------" << endl;
			    			break;
						}
				
						default:
						{
			    			cout << "--------------------------------------------------------" << endl;
			    			cout << "----   *** Could not recognize the BUS Command ***  ----" << endl;
			    			cout << "--------------------------------------------------------" << endl;
			    			break;
						}
				} //end of snooper case

				wait();

			}//end of switch
        }//end of while
    } //end of bus
	
    void execute() 
    {
        while (true)
        {
	   		// cout << " Waiting in cache::execute for WR function cache id_"<<cache_id<<endl;
            wait(Port_Func.value_changed_event());
			
	    	//cout << " cache::execute Port_function value is changed for cache id_"<<cache_id<<endl;
            //cout << "Cache ID is " << cache_id << endl; 
            Function f = Port_Func.read();
            int addr   = Port_Addr.read();
            int data   = 0;
	    	int cache_data;
			
            if (f == FUNC_WRITE) 
            {
                cout << sc_time_stamp() << " Cache_id: " << cache_id <<" received write" << endl;
                data = Port_Data.read().to_int();
            }

            // This simulates cache memory read/write delay
            //wait(99);
			
	    	if (f == FUNC_READ) 
            {
                cache_data = cache_operation(cache_id, addr, READ);
                Port_Data.write( cache_data );
                Port_Done.write( RET_READ_DONE );
                wait();
                Port_Data.write("ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ");
            }
            else
            {
                cache_data = cache_operation(cache_id, addr, WRITE);
                cout << "Data " << data <<" is written at [ " << addr << " ]" << endl;
                Port_Done.write( RET_WRITE_DONE );
            }
        }//end of while
    }//end of execute
};//end of cache
		
    int Cache::cache_operation(int cache_id, int addr, int mode)
	{
	    //Check if the address is available in the cache by comparing the TAG
	    // if tag match == HIT otherwise MISS
	    int i = 0; 
	    int ret_data = 0 ;
	    int found = 0;
	    unsigned int tag = (addr & TAG_CHECK) >> 12;
	    unsigned int set = (addr & SET_CHECK) >> 5;
	    unsigned int word = (addr & OFF_CHECK) >> 2;
	    //int pid = 0;
	    data = 0;
	    if ( mode == 0 ) //READ
	    {
		//	READ MODE CHECK FOR TAG MATCH, IF TAG MATCHES THEN CHECK VALID.
		//  IF VALID BLOCK IS FOUND, THEN RETURN THE VALUE OTHERWISE IT IS A READ MISS
			for (i=0; i < MAX_SET; i++)
			{
		    	if ((m_data[set].way[i].c_tag == tag)  && (found == 0))
		    	{
					if ( m_data[set].way[i].c_valid == 1)
					{
			    		cout << "*** Read Hit - Valid Data is found ***" << endl;
			    		cout << "*** No Bus command is required, since it is Read Hit PRd/-" << endl;
		            	cout << " Read Hit on cache_id: " << cache_id << " at [ " << addr << " ]" <<endl;
			    		stats_readhit(cache_id);
			    		found = 1;
			    		ret_data = m_data[set].way[i].c_data[word];
			    		update_lru(set, i);
			    		wait();
			    		break;	
					}
					else
					{
			    		// READ MISS, COPY THE DATA FROM MAIN MEMORY
			    		cout << " *** Read Miss - Data is not valid, copy data from Memory ***" << endl;
			    		cout << " *** Read Miss - Issuing a BusRd, to copy the data from memory PRd/BusRd ***" << endl;
		            	cout << " Read Miss on cache_id: " << cache_id << " at [ " << addr << " ]" << endl;
			    		Port_Bus->read(cache_id, addr);
		            	//cout << "I am back, cache_id : "<< cache_id << "Port_Bus->read is executed" << endl;
			    		stats_readmiss(cache_id);
			    		wait(100);
		        		// wait(100); //write back the data to memory before updating the cache line
		        		int selected_way = lru_call(set) ;	
		    			m_data[set].way[selected_way].c_data[word] = rand();
			    		m_data[set].way[selected_way].c_valid = 1;
			    		m_data[set].way[selected_way].c_tag = tag;
			    		ret_data = m_data[set].way[i].c_data[word];
			    		found = 1;
			    // update LRU
			    update_lru(set, i);
			    break;
			}
		     }
		}
		if (found == 0)
		{
		    //READ MISS, GET THE DATA FROM MEMORY AND REPLACE ONE LINE FROM CACHE USING LRU
		    //GET THE WAY TO BE UPDATED THROUGH LRU
		    cout << " *** Read Miss - Data is stale, Copy from memory ***" << endl;
		    cout << " *** Read Miss - Issuing a BusRd, to copy the data from memory PRd/BusRd ***" << endl;
		    cout << " Read Miss on cache_id: " << cache_id << " at [ " << addr << " ]" << endl;
		    Port_Bus->read(cache_id, addr);
		    //cout << "I am back, cache_id : "<< cache_id << "Port_Bus->read is executed" << endl;
		    stats_readmiss(cache_id);
		    //wait(100);
		    wait(100); //write back the data to memory before updating the cache line
		    int selected_way = lru_call(set) ;	
		    m_data[set].way[selected_way].c_data[word] = rand();
		    m_data[set].way[selected_way].c_valid = 1;
		    m_data[set].way[selected_way].c_tag = tag;
		    //update LRU table
		    update_lru(set, selected_way);
		    ret_data = m_data[set].way[selected_way].c_data[word];
		    found = 1;		
		}
	    return ret_data;	
	    }		
	    else
	    // NEED TO PERFORM WRITE OPERATION
	    {
		
		for (i=0; i < MAX_SET; i++)
		{
		    if ((m_data[set].way[i].c_tag == tag)  && (found == 0) && (m_data[set].way[i].c_valid == 1))
		    {
				cout << " *** Cache write Hit ***" << endl;
				cout << " *** WriteHit - Issue bus command PrWR/BusWr *** " << endl;
		        cout << " Write Hit on cache_id: " << cache_id << " at [ " << addr << " ]" << endl;
				Port_Bus->write(cache_id, addr, data);
		        //cout << "I am back, cache_id : "<< cache_id << "Port_Bus->Write is executed" << endl;
				Port_Bus->busLock();
				m_data[set].way[i].c_data[word] = data;
				stats_writehit(cache_id);
				m_data[set].way[i].c_valid = 1;
				//update LRU
				update_lru(set, i);
				found = 1;
				ret_data = m_data[set].way[i].c_data[word];
				wait();
				break ;
		    }
		}
		if (found == 0)
		{
		    //WRITE MISS, HENCE GET A BLOCK ADDRESS THROUGH LRU LOOK UP TABLE
		    cout << " *** Cache Write Miss " << endl;
		    cout << " *** Write Miss - Issue Bus Command PrWr/BusRdX ***" << endl;
		    cout << " Write Miss on cache_id: " << cache_id << " at [ " << addr << " ]" << endl;
		    //Implement BusRdX prototcol - CHECK
		    Port_Bus->writeX(cache_id, addr, data); 
		    //cout << "I am back, cache_id : "<< cache_id << "Port_Bus->WriteX is executed" << endl;
		    stats_writemiss(cache_id);
		    Port_Bus->busLock();
		    //Valid Invalid Protocol Assumes Write Through Protocol
		    wait(100);

		    int selected_way = lru_call(set);
		    cout << " Way Selected by LRU Look Up Table is " << selected_way <<endl;
		    m_data[set].way[selected_way].c_data[word] = rand();
		    m_data[set].way[selected_way].c_valid = 1;
		    m_data[set].way[selected_way].c_tag = tag;
		    //update LRU LUT
		    update_lru(set, selected_way);
		    //found = 1;	
		}
		cout << "Write Through is Assumed, hence writing the data back to memory" << endl;
		wait(100);	
		Port_Bus->busUnlock();
		return 0;
	    }	

    }  // END OF cache_operation();
	
    int Cache::lru_call(int line) 
    {
    	int present_state = m_data[line].lru;
    	cout << "Present State for line "<< line << "is : " << present_state <<endl; 
    	if( (present_state & 0xB ) == 0)
    	{
            return 0;
    	} 
		else if( (present_state & 0xB ) == 0x8 ) 
    	{
            return 1;
    	} 
		else if ( (present_state & 0x13 ) == 0x2 )
    	{
            return 2;
		} 
		else if ( (present_state & 0x13 ) == 0x12 )
    	{
            return 3;
    	} 
		else if ( (present_state & 0x25 ) == 0x1 )
    	{
            return 4;
    	} 
		else if ( (present_state & 0x25 ) == 0x21 ) 
    	{
            return 5;
    	}
		else if ( (present_state & 0x45 ) == 0x5 )
    	{
            return 6;
		} 
		else if ( (present_state & 0x45 ) == 0x45 )
    	{
            return 7;
    	}
    	else 
		{
            cout << "Something wrong in finding LRU Way :( "<< endl;
            return 0;  
    	}

		
    } //end of Cache::lru

    void Cache::update_lru(int line, int way)
    {
		cout << " LRU way is: " << way << " and present LRU is " << m_data[line].lru << endl; 
        if ( way == 0 ) //NS  xxx1x11
		{
            m_data[line].lru = m_data[line].lru | 0xB;
        }
        else if ( way == 1 )
		{ // L1 is replaced. Next state xxx0x11
            m_data[line].lru = m_data[line].lru | 0x3;
            m_data[line].lru = m_data[line].lru & 0x77; 
        }
        else if ( way == 2 )
		{ // NS xx1xx01
            m_data[line].lru = m_data[line].lru | 0x11; 
            m_data[line].lru = m_data[line].lru & 0x7D; 
        }
		else if ( way == 3 )
        { // NS  xx0xx01
            m_data[line].lru = m_data[line].lru | 0x1;
            m_data[line].lru = m_data[line].lru & 0x6D;
        }
        else if( way ==4) 
		{// NS x1xx1x0
            m_data[line].lru = m_data[line].lru | 0x24; 
            m_data[line].lru = m_data[line].lru & 0x7E; 
        }
        else if ( way == 5 ) // NS x0xx1x0
        {      
	    	m_data[line].lru = m_data[line].lru | 0x4; 
            m_data[line].lru = m_data[line].lru & 0x5E; 
        }
        else if ( way == 6 )
		{ // NS 1xxx0x0
            m_data[line].lru = m_data[line].lru | 0x40; 
            m_data[line].lru = m_data[line].lru & 0x7A; 
        }
        else if ( way ==7 ) 
		{ // NS 0xxx0x0    
            m_data[line].lru = m_data[line].lru & 0x3A;    
        } 
        else
            cout << "Something wrong with LRU update"<< endl;

		cout << " LRU way is: " << way << " and LRU is updated to " << m_data[line].lru << endl; 

    } //end of Cache::update_lru

	
/* Bus class, provides a way to share one memory in multiple CPU + Caches. */
class Bus : public Bus_if, public sc_module
{

public:

    /* Ports and  vb Signals. */
    sc_in<bool> Port_CLK;
    //sc_out<Cache::Function> Port_BusValid;
    sc_out<Cache::Bus_Req> Port_BusValid;
    sc_out<int> Port_BusWriter;

    sc_signal_rv<32> Port_BusAddr;

    /* Bus mutex. */
    sc_mutex bus;

    /* Variables. */

    int waits;
    int reads;
    int writes;

public:
    /* Constructor. */
    SC_CTOR(Bus) 
    {
        /* Handle Port_CLK to simulate delay */
        sensitive << Port_CLK.pos();

        // Initialize some bus properties
        Port_BusAddr.write("ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ");

        /* Update variables. */
        waits = 0;
        reads = 0;
        writes = 0;

    }//end of SC_CTOR

    /* Perform a read access to memory addr for CPU #writer. */
    virtual bool read(int writer, int addr)
    {
        /* Try to get exclusive lock on bus. */
        while(bus.trylock() == -1)
		{
            /* Wait when bus is in contention. */
            waits++;
            wait();
        }

        /* Update number of bus accesses. */
        reads++;

        /* Set lines. */
        Port_BusAddr.write(addr);
        Port_BusWriter.write(writer);
        Port_BusValid.write(Cache::BUS_RD);

        /* Wait for everyone to recieve. */
        wait();

        /* Reset. */
        Port_BusValid.write(Cache::BUS_INVALID);
        Port_BusAddr.write("ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ");
        bus.unlock();

        return true;
    }

    virtual bool busLock()
    {
		while(bus.trylock() == -1)
		{
	    	waits++;
	    	wait();
		}
		return true;
    }

    virtual bool busUnlock()
    {
		bus.unlock();
		return true;
    }


    /* Write action to memory, need to know the writer, address and data. */
    virtual bool write(int writer, int addr, int data)
    {
        /* Try to get exclusive lock on the bus. */
        while(bus.trylock() == -1)
		{
            waits++;
            wait();
        }

        /* Update number of accesses. */
        writes++;

        /* Set. */
        Port_BusAddr.write(addr);
        Port_BusWriter.write(writer);
        Port_BusValid.write(Cache::BUS_WR);

        /* Wait for everyone to recieve. */
        wait();

        /* Reset. */
        Port_BusValid.write(Cache::BUS_INVALID);
        Port_BusAddr.write("ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ");
        bus.unlock();

        return true;
    }
	
    virtual bool writeX(int writer, int addr, int data) 
    {
    /* Try to get exclusive lock on the bus. */
    	while(bus.trylock() == -1)
		{
	    	waits++;
	    	wait();
		}
		
		/* Update number of accesses. */
		writes++;

		Port_BusAddr.write(addr);
		Port_BusValid.write(Cache::BUS_RdX);
		Port_BusWriter.write(writer);

		/* Wait for everyone to recieve. */
		wait();
		
		/* Reset. */
        Port_BusValid.write(Cache::BUS_INVALID);
        Port_BusAddr.write("ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ");
        bus.unlock();

	return true;
    }


    /* Bus output. */
    void output()
    {
        /* Write output as specified in the assignment. */
        double avg = (double)waits / double(reads + writes);
        printf("\n 2. Main memory access rates\n");
        printf("    Bus had %d reads and %d writes.\n", reads, writes);
        printf("    A total of %d accesses.\n", reads + writes);
        printf("\n 3. Average time for bus acquisition\n");
        printf("    There were %d waits for the bus.\n", waits);
        printf("    Average waiting time per access: %f cycles.\n", avg);
        //printf("\n 4. Total execution time is %d", sc_time_stamp().to_int());
        //printf("\n");
    }
};

		

	
SC_MODULE(CPU) 
{

public:
    sc_in<bool>                	Port_CLK;
    sc_in<Cache::RetCode>   	Port_MemDone;
    sc_out<Cache::Function> 	Port_MemFunc;
    sc_out<int>                	Port_MemAddr;
    sc_inout_rv<32>            	Port_MemData;
	
	int cpu_id;
	
	//Bus Address declaration
	//sc_inout_rv<32>				Port_MemBusAddr;

    SC_CTOR(CPU) 
    {
        SC_THREAD(execute);
        sensitive << Port_CLK.pos();
        dont_initialize();
    }

private:
    void execute() 
    {
        TraceFile::Entry    tr_data;
        Cache::Function  f;
		
        int addr, data;
		
        // Loop until end of tracefile
        while(!tracefile_ptr->eof())
        {
            // Get the next action for the processor in the trace
            if(!tracefile_ptr->next(cpu_id, tr_data))
            {
                cerr << "Error reading trace for CPU" << endl;
                break;
            }

            // To demonstrate the statistic functions, we generate a 50%
            // probability of a 'hit' or 'miss', and call the statistic
            // functions below
            //int j = rand()%2;
	    	addr = tr_data.addr;

            switch(tr_data.type)
            {
                case TraceFile::ENTRY_TYPE_READ:
                    f = Cache::FUNC_READ;
                    break;

                case TraceFile::ENTRY_TYPE_WRITE:
                    f = Cache::FUNC_WRITE;
                    break;

                case TraceFile::ENTRY_TYPE_NOP:
                    break;

                default:
                    cerr << "Error, got invalid data from Trace" << endl;
                    exit(0);
            }

            if(tr_data.type != TraceFile::ENTRY_TYPE_NOP)
            {
                Port_MemAddr.write(addr);
                Port_MemFunc.write(f);

                if (f == Cache::FUNC_WRITE) 
                {
                    cout << sc_time_stamp() << ": CPU_" << cpu_id << " sends write" << endl;

                    data = rand();
                    Port_MemData.write(data);
                    wait();
                    Port_MemData.write("ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ");
                }
                else
                {
                    cout << sc_time_stamp() << ": CPU_" << cpu_id << " sends read" << endl;
                }

                wait(Port_MemDone.value_changed_event());

               if (f == Cache::FUNC_READ)
                {
                    cout << sc_time_stamp() << ": CPU reads: " << Port_MemData.read() << endl;
                }
            }
       /*     else
            {
                cout << sc_time_stamp() << ": CPU_" << cpu_id << " executes NOP" << endl;
            }
         */   // Advance one cycle in simulated time            
            wait();
        }
        
        // Finished the Tracefile, now stop the simulation
        sc_stop();
    }
};


int sc_main(int argc, char* argv[])
{
    try
    {
        // Get the tracefile argument and create Tracefile object
        // This function sets tracefile_ptr and num_cpus
        init_tracefile(&argc, &argv);

        //max_cpus = num_cpus;
		
		cout << "Number of CPU is "<< num_cpus << endl;
		unsigned int i;
		int snooping = 1;
		
        // Initialize statistics counters
        stats_init();
		
		// The clock that will drive the CPU and Memory
        sc_clock clk("clk", sc_time(1, SC_NS));

		// Instantiate Module for Bus
		Bus bus("bus");
		
		//Connect to Bus
		bus.Port_CLK(clk);

        /* Cpu and Cache pointers. */
        Cache* cache[num_cpus];
        CPU* cpu[num_cpus];
        // Signals
        sc_buffer<Cache::Function> sigMemFunc[num_cpus];
        sc_buffer<Cache::RetCode>  sigMemDone[num_cpus];
        sc_signal<int>              sigMemAddr[num_cpus];
        sc_signal_rv<32>            sigMemData[num_cpus];
		        
		/* Signals Chache/Bus. */
        sc_signal<int>              sigBusWriter;
        //sc_signal<Cache::Function>  sigBusValid;
        sc_signal<Cache::Bus_Req>  sigBusValid;
		
		/* General Bus signals. */
        bus.Port_BusWriter(sigBusWriter);
        bus.Port_BusValid(sigBusValid);
		
		
		//int snooping = 1;
	
		for ( i =0; i < num_cpus; i++)
		{
		
	    	char name_cache[12];
            char name_cpu[12];
			
	    	sprintf(name_cache, "cache_%d", i);
            sprintf(name_cpu, "cpu_%d", i);

            /* Create CPU and Cache. */
            cache[i] = new Cache(name_cache);
            cpu[i] = new CPU(name_cpu);

            /* Set ID's. */
            cpu[i]->cpu_id = i;
            cache[i]->cache_id = i;
            cache[i]->snooping = snooping;
			
	    	/* Cache to Bus. */
            cache[i]->Port_BusAddr(bus.Port_BusAddr);
            cache[i]->Port_BusWriter(sigBusWriter);
            cache[i]->Port_BusValid(sigBusValid);
            cache[i]->Port_Bus(bus);
		
	    	// Connecting module ports with signals
	    	cache[i]->Port_Func(sigMemFunc[i]);
	    	cache[i]->Port_Addr(sigMemAddr[i]);
	    	cache[i]->Port_Data(sigMemData[i]);
	    	cache[i]->Port_Done(sigMemDone[i]);

	    	cpu[i]->Port_MemFunc(sigMemFunc[i]);
	    	cpu[i]->Port_MemAddr(sigMemAddr[i]);
	    	cpu[i]->Port_MemData(sigMemData[i]);
	    	cpu[i]->Port_MemDone(sigMemDone[i]);
			
	    	cache[i]->Port_CLK(clk);
	    	cpu[i]->Port_CLK(clk);
			
		}

        sc_trace_file *wf = sc_create_vcd_trace_file("Valid_Invalid_Protocol");
        // Dump the desired signals
        sc_trace(wf, clk, "clock");
		for (unsigned int k = 0; k < num_cpus; k++)
		{
	    	char cpu_addr[12];
	    	char cpu_data[12];
	    	char cpu_Done[12];
	    	char cpu_Func[12];
	
	    	sprintf(cpu_addr, "cpu%d_addr", k);
	    	sprintf(cpu_data, "cpu%d_data", k);
	    	sprintf(cpu_Func, "cpu%d_func", k);
	    	sprintf(cpu_Done, "cpu%d_done", k);
	    

            sc_trace(wf, sigMemData[k], cpu_data);
            sc_trace(wf, sigMemAddr[k], cpu_addr);
	   		sc_trace(wf, sigMemFunc[k], cpu_Func);
	    	sc_trace(wf, sigMemDone[k], cpu_Done);

		}

        sc_trace(wf, sigBusWriter, "Bus_Writer");
		sc_trace(wf, sigBusValid, "Bus_Request");

        cout << "Running (press CTRL+C to interrupt)... " << endl;

		sc_report_handler::set_actions(SC_ID_MORE_THAN_ONE_SIGNAL_DRIVER_, SC_DO_NOTHING);
        // Start Simulation
        sc_start();

		sc_close_vcd_trace_file(wf);
        
        // Print statistics after simulation finished
        stats_print();
		bus.output();
		cout << " \nTotal execution time is " << sc_time_stamp() << endl;
    } // end of try

    catch (exception& e)
    {
        cerr << e.what() << endl;
    }
    
    return 0;
}
