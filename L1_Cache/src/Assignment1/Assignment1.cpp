/*
// File: Assignment_1.cpp
//              
// Framework to implement Task 1 of the Advances in Computer Architecture lab 
// session. This uses the ACA 2009 library to interface with tracefiles which
// will drive the read/write requests
//Assignment 1
//
//In this task you must build a single 32kB 8-way set-associative L1 D-cache with a 32-Byte line size.
//The simulator must be able to be driven with the single processor trace files. Assume a Memory latency
//of 100 cycles, and single cycle cache latency. Use a least-recently-used, write-back replacement
//strategy.
//
//single 32kB 8-way set-associative L1 D-cache with a 32-Byte line size
//
//offset = 2^5 = 32bytes == 5  offset
//
//cache block = 32KB/32 bits = 1KB cache blocks
//
//sets = 1024/8 = 128 sets = 2^7 == 7 indices
//
//tag field = 20 bits
//
//32KB  ==> 32*1024 = 32768
//
//index = 32K/(32*8) = 1024/8 = 128==> 2^7
//
//LRU replacement bits = 7 (8 way -1)
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

static const int MAX_CACHE_ENTRIES 	= 128;
static const int MAX_CACHE_SIZE 	= 32768; 	//32KB
static const int CACHE_LINE_SIZE 	= 32; 		//32 bytes
static const int MAX_SET 		= 8;


typedef struct {
	//cache fields - Data, Valid, tag
        unsigned int c_data[8];
        unsigned int c_valid:1;
        unsigned int c_tag:20;
} cache_data;
   
typedef struct 
    {
        unsigned int lru:7;                     // 7 bit representation
        cache_data way[MAX_SET];        	// 8

    } cache_set;


SC_MODULE(Memory) 
{

public:
    enum Function 
    {
        FUNC_READ,
        FUNC_WRITE,
    };

    enum RetCode 
    {
        RET_READ_DONE,
        RET_WRITE_DONE,
    };
	

    sc_in<bool>     Port_CLK;
    sc_in<Function> Port_Func;
    sc_in<int>      Port_Addr;
    sc_out<RetCode> Port_Done;
    sc_inout_rv<32> Port_Data;

    void update_lru(int line, int way);
    int lru_call(int line);
    int cache_operation(int addr, int mode);

    SC_CTOR(Memory) 
    {
        SC_THREAD(execute);
        sensitive << Port_CLK.pos();
        dont_initialize();
        m_data = new cache_set[MAX_CACHE_SIZE /( MAX_SET * CACHE_LINE_SIZE)];
        for (int i = 0; i < MAX_CACHE_ENTRIES ; i++)
        {
            for (int j = 0; j < MAX_SET ; j++)
            {
                m_data[i].way[j].c_tag = 0xFFFFF;
            }
        }	
   }

    ~Memory() 
    {
        delete m_data;
    }

private:

    cache_set* m_data;
    int data;
    void execute() 
    {
        while (true)
        {
            wait(Port_Func.value_changed_event());
            
            Function f = Port_Func.read();
            int addr   = Port_Addr.read();
            int data   = 0;
	    int cache_data;
			
            if (f == FUNC_WRITE) 
            {
                cout << sc_time_stamp() << ": CACHE received write" << endl;
                data = Port_Data.read().to_int();
            }

            // This simulates cache memory read/write delay
            //wait(99);
			
	    if (f == FUNC_READ) 
            {
                cache_data = cache_operation(addr, READ);
                Port_Data.write( cache_data );
                Port_Done.write( RET_READ_DONE );
                wait();
                Port_Data.write("ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ");
            }
            else
            {
                cache_data = cache_operation(addr, WRITE);
                cout << "Data " << data <<" is written at " << addr  << endl;
                Port_Done.write( RET_WRITE_DONE );
            }
        }
    }
};
		
    int Memory::cache_operation(int addr, int mode)
	{
		//Check if the address is available in the cache by comparing the TAG
		// if tag match == HIT otherwise MISS
		int i = 0; 
		int ret_data = 0 ;
		int found = 0;
		unsigned int tag = (addr & TAG_CHECK) >> 12;
		unsigned int set = (addr & SET_CHECK) >> 5;
		unsigned int word = (addr & OFF_CHECK) >> 2;
		int pid = 0;
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
						found = 1;
						ret_data = m_data[set].way[i].c_data[word];
						update_lru(set, i);
						stats_readhit(pid);
						wait();
						break;	
					}
					else
					{
						// READ MISS, COPY THE DATA FROM MAIN MEMORY
						cout << " *** Read Miss - Data is not valid, copy data from Memory ***" << endl;
						stats_readmiss(pid);
						wait(100);
						m_data[set].way[i].c_data[word] = rand();
						m_data[set].way[i].c_valid = 1;
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
				stats_readmiss(pid);
				wait(100);
				int selected_way = lru_call(set) ;	//lru???
				m_data[set].way[selected_way].c_data[word] = rand();
				m_data[set].way[selected_way].c_valid = 1;
				m_data[set].way[selected_way].c_tag = tag;
				//update LRU table
				update_lru(set, selected_way);
				ret_data = m_data[set].way[selected_way].c_data[word];
				found = 1;		
			}
			return ret_data	;	
		}		
		else
		// NEED TO PERFORM WRITE OPERATION
		{	
			for (i=0; i < MAX_SET; i++)
			{
				if ((m_data[set].way[i].c_tag == tag)  && (found == 0))
				{
					cout << " Cache write Hit" << endl;
					m_data[set].way[i].c_data[word] = data;
					stats_writehit(0);
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
				cout << "Cache Write Miss " << endl;
            			stats_writemiss(0);
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
			return 0;
		}	

	}  // END OF cache_operation();
	
int Memory::lru_call(int line) 
	{
    		int present_state = m_data[line].lru;
    		cout << "Present State for line "<< line << "is : " << present_state <<endl; 
    		if( (present_state & 0xB ) == 0)
    		{
        		return 0;
    		} else if( (present_state & 0xB ) == 0x8 ) 
    		{
        		return 1;
    		} else if ( (present_state & 0x13 ) == 0x2 )
    		{
        		return 2;
		} else if ( (present_state & 0x13 ) == 0x12 )
    		{
        		return 3;
    		} else if ( (present_state & 0x25 ) == 0x1 )
    		{
        		return 4;
    		} else if ( (present_state & 0x25 ) == 0x21 ) 
    		{
        		return 5;
    		} else if ( (present_state & 0x45 ) == 0x5 )
    		{
        		return 6;
	    	} else if ( (present_state & 0x45 ) == 0x45 )
    		{
        		return 7;
    		}
    		else 
		{
        		cout << "Something wrong in finding LRU Way :( "<< endl;
        		return 0;  
    		}

		
	}

void Memory::update_lru(int line, int way)
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

	}

		

	
SC_MODULE(CPU) 
{

public:
    sc_in<bool>                Port_CLK;
    sc_in<Memory::RetCode>   Port_MemDone;
    sc_out<Memory::Function> Port_MemFunc;
    sc_out<int>                Port_MemAddr;
    sc_inout_rv<32>            Port_MemData;

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
        Memory::Function  f;
	//int addr, data, pid=0;
        int addr, data;
        // Loop until end of tracefile
        while(!tracefile_ptr->eof())
        {
            // Get the next action for the processor in the trace
            if(!tracefile_ptr->next(0, tr_data))
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
                    f = Memory::FUNC_READ;
                    break;

                case TraceFile::ENTRY_TYPE_WRITE:
                    f = Memory::FUNC_WRITE;
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

                if (f == Memory::FUNC_WRITE) 
                {
                    cout << sc_time_stamp() << ": CPU sends write" << endl;

                    data = rand();
                    Port_MemData.write(data);
                    wait();
                    Port_MemData.write("ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ");
                }
                else
                {
                    cout << sc_time_stamp() << ": CPU sends read" << endl;
                }

                wait(Port_MemDone.value_changed_event());

               if (f == Memory::FUNC_READ)
                {
                    cout << sc_time_stamp() << ": CPU reads: " << Port_MemData.read() << endl;
                }
            }
            else
            {
                cout << sc_time_stamp() << ": CPU executes NOP" << endl;
            }
            // Advance one cycle in simulated time            
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

        // Initialize statistics counters
        stats_init();

        // Instantiate Modules
        Memory mem("main_memory");
        CPU    cpu("cpu");

        // Signals
        sc_buffer<Memory::Function> sigMemFunc;
        sc_buffer<Memory::RetCode>  sigMemDone;
        sc_signal<int>              sigMemAddr;
        sc_signal_rv<32>            sigMemData;

        // The clock that will drive the CPU and Memory
        sc_clock clk("clk", sc_time(1, SC_NS));

        // Connecting module ports with signals
        mem.Port_Func(sigMemFunc);
        mem.Port_Addr(sigMemAddr);
        mem.Port_Data(sigMemData);
        mem.Port_Done(sigMemDone);

        cpu.Port_MemFunc(sigMemFunc);
        cpu.Port_MemAddr(sigMemAddr);
        cpu.Port_MemData(sigMemData);
        cpu.Port_MemDone(sigMemDone);

        sc_trace_file *wf = sc_create_vcd_trace_file("L1_Cache_Waveform");
        // Dump the desired signals
        sc_trace(wf, clk, "clock");
        sc_trace(wf, sigMemDone, "Memory_Done");
        sc_trace(wf, sigMemAddr, "Memory_address");


        mem.Port_CLK(clk);
        cpu.Port_CLK(clk);

        cout << "Running (press CTRL+C to interrupt)... " << endl;


        // Start Simulation
        sc_start();

	sc_close_vcd_trace_file(wf);
        
        // Print statistics after simulation finished
        stats_print();
    }

    catch (exception& e)
    {
        cerr << e.what() << endl;
    }
    
    return 0;
}
