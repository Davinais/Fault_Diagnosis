/**********************************************************************/
/*  Parallel-Fault Event-Driven Transition Delay Fault Simulator      */
/*                                                                    */
/*           Author: Tsai-Chieh Chen                                  */
/*           last update : 10/22/2018                                 */
/**********************************************************************/

#include "atpg.h"

/* pack 16 faults into one packet.  simulate 16 faults together. 
 * the following variable name is somewhat misleading */
#define num_of_pattern 16

/* The faulty_wire contains a list of wires that 
 * change their values in the fault simulation for a particular packet.
 * (that is wire_value1 != wire_value2) 
 * Note that the wire themselves are not necessarily a fault site.
 * The list is linked by the pnext pointers */

/* fault simulate a set of test vectors */
void ATPG::transition_delay_fault_simulation(int &total_detect_num) {

}

void ATPG::generate_tdfault_list() {

}