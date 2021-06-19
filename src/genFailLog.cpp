/**********************************************************************/
/*                  Generate Faillog                                  */
/*                                                                    */
/*           Author: I-Wei Chiu                                       */
/*           last update : 05/08/2021                                 */
/**********************************************************************/

#include "atpg.h"
#include <unordered_map>

#define num_of_pattern 16

void ATPG::generate_genFailLog_list() {
  int fault_num;
  wptr w;
  nptr n;
  fptr_s f;
  unordered_map<wptr, int> num_of_eqv_sa0, num_of_eqv_sa1;
  /* walk through every wire in the circuit*/
  for (auto pos : sort_wlist) {
    w = pos;
    bool check = false;
    int index = 0;
    for (string faultwire : fault_Wire_Pos) {
        // cout << w->name.substr(0,faultwire.size()) << endl;
        // cout << faultwire << endl;
        if (w->name.substr(0,faultwire.size()) == faultwire){
            check = true;
            break;
        }
        index++;
    }
    if (!check) continue;

    check = false;
    if (fault_IO[index] == "GO"){
        for (auto faultgate : w->inode){
            if (faultgate->name == fault_Gate_Pos[index]){
                n = faultgate;
                check = true;
                break;
            }
        }
    }
    else {
        for (auto faultgate : w->onode){
            if (faultgate->name == fault_Gate_Pos[index]){
                n = faultgate;
                check = true;
                break;
            }
        }
    }
    if (!check) continue;


    // n = w->inode.front();

    /* for each gate, create a gate output stuck-at zero (SA0) fault */
    f = move(fptr_s(new(nothrow) FAULT));
    if (f == nullptr) error("No more room!");
    f->node = n;
    if (fault_IO[index] == "GO") f->io = GO;     // gate output SA fault
    else f->io = GI;     // gate input SA fault

    if (fault_Type[index] == "SA0") f->fault_type = STUCK0;
    else f->fault_type = STUCK1;

    f->to_swlist = w->wlist_index;

    // for AND NOR NOT BUF, their GI fault is equivalent to GO SA0 fault
    // For each fault f, we count how many faults are in the same equivalent class as f.
    //   So that, we can calculate uncollapsed fault coverage.

    f->eqv_fault_num = 1;    // GO SA0 fault itself
    num_of_gate_fault += f->eqv_fault_num;
    flist_undetect.push_front(f.get());
    flist.push_front(move(f));
    
  }
  flist.reverse();
  flist_undetect.reverse();

  /* If the fault list is still empty, it means that we cannot find a valid position for given input */
  if (flist_undetect.empty()) {
    cerr << "Error: Invalid fault position! Abort..." << endl;
    exit(1);
  }
  
  /*walk through all faults, assign fault_no one by one  */
  fault_num = 0;
  for (fptr f: flist_undetect) {
    f->fault_no = fault_num;
    fault_num++;
    //cout << f->fault_no << f->node->name << ":" << (f->io?"O":"I") << endl;

    // cout  << f->node->name << ":" << (f->io?"O":"I")<<" " << "SA" << f->fault_type << endl;
  }

//   fprintf(stdout, "#number of equivalent faults = %d\n", fault_num);
}/* end of generate_fault_list */



void ATPG::genFailLog_fault_sim() {
    int i;
    
    /* for every vector */
    for (i = vectors.size() - 1; i >= 0; i--) {
        genFailLog_sim_a_vector(vectors[i],i);
    }
}


void ATPG::genFailLog_sim_a_vector(const string &vec, int &number){
    wptr w, faulty_wire;
    /* array of 16 fptrs, which points to the 16 faults in a simulation packet  */
    fptr simulated_fault_list[num_of_pattern];
    fptr f;
    int fault_type;
    int i, start_wire_index, nckt;
    int num_of_fault;
    num_of_fault = 0; // counts the number of faults in a packet

    /* num_of_current_detect is used to keep track of the number of undetected faults
    * detected by this vector.  Initialize it to zero */
    

    /* Keep track of the minimum wire index of 16 faults in a packet.
    * the start_wire_index is used to keep track of the
    * first gate that needs to be evaluated.
    * This reduces unnecessary check of scheduled events.*/
    start_wire_index = 10000;

    /* for every input, set its value to the current vector value */
    for (i = 0; i < cktin.size(); i++) {
        cktin[i]->value = ctoi(vec[i]);
    }

    /* initialize the circuit - mark all inputs as changed and all other
    * nodes as unknown (2) */
    nckt = sort_wlist.size();
    for (i = 0; i < nckt; i++) {
        if (i < cktin.size()) {
        sort_wlist[i]->set_changed();
        } else {
        sort_wlist[i]->value = U;
        }
        sort_wlist[i]->remove_faulty();
    }

    sim(); /* do a fault-free simulation, see sim.cpp */


    for (i = 0; i < nckt; i++) {
    switch (sort_wlist[i]->value) {
      case 1:
        sort_wlist[i]->wire_value1 = ALL_ONE;  // 11 represents logic one
        sort_wlist[i]->wire_value2 = ALL_ONE;
        break;
      case 2:
        sort_wlist[i]->wire_value1 = 0x55555555; // 01 represents unknown
        sort_wlist[i]->wire_value2 = 0x55555555;
        break;
      case 0:
        sort_wlist[i]->wire_value1 = ALL_ZERO; // 00 represents logic zero
        sort_wlist[i]->wire_value2 = ALL_ZERO;
        break;
    }
  } // for in

    for (auto pos = flist_undetect.cbegin(); pos != flist_undetect.cend(); ++pos){
       
        f = *pos;
        //cout << f->fault_no << f->node->name << ":" << (f->io?"O":"I")<<" " << "SA" << f->fault_type << endl;
        //if (f->fault_type != sort_wlist[f->to_swlist]->value) cout<<"Not activate!!!\n";

        if ((f->fault_type == sort_wlist[f->to_swlist]->value) && (total_target_fault == 1)) continue;

        fault_type = f->fault_type;
        // Since the fault is from user input(external), the 3rd arg of get_faulty_wire should be set as true.
        auto faulty_wire = (f->io == GO) ? sort_wlist[f->to_swlist] : get_faulty_wire(f, fault_type, true);
        if (faulty_wire != nullptr) {
            if (!(faulty_wire->is_faulty())) {
                faulty_wire->set_faulty();
                // doesn't be used?!
                wlist_faulty.push_front(faulty_wire);
            }
            inject_fault_value(faulty_wire, 0, fault_type);
            faulty_wire->set_fault_injected();

            if ((f->node->type == OUTPUT) || (faulty_wire->is_output())) {;}
            else {
                for (auto pos_n : faulty_wire->onode) {
                    pos_n->owire.front()->set_scheduled();
                }
            }
        }
    }

    for (i = 0; i < nckt; i++) {
        if (sort_wlist[i]->is_scheduled()) {
          sort_wlist[i]->remove_scheduled();
          if(!sort_wlist[i]->is_faulty()) fault_sim_evaluate(sort_wlist[i]);
        //   cout << "hi"<<endl;
        }
        // cout << "hi2"<<endl;
    } /* event evaluations end here */

    for (i = 0; i < cktout.size(); i++) {
        wptr w = cktout[i];
        //cout << (w->wire_value1 & Mask[0]) <<", " << (w->wire_value2 & Mask[0]) << endl;
        if((w->wire_value1 & Mask[0]) != (w->wire_value2 & Mask[0])){
            cout << "vector[" << number << "]  " ;
            //print_name(w->name);
            cout << w->name << "   ";
            cout << "expect " ;
            if ((w->wire_value1 & Mask[0]) == 3) cout << "H, observe ";
            else cout << "L, observe ";
            if ((w->wire_value2 & Mask[0]) == 3) cout << "H      #   T'" << vec << "'" << endl;
            else cout << "L      #   T'" << vec << "'" << endl;
            //cout << (w->wire_value1 & Mask[0]) <<", " << (w->wire_value2 & Mask[0]) << endl;
        }
    } /* event evaluations end here */
 
}



void ATPG::print_name(string s){

    for (auto it=s.cbegin(); it!=s.cend(); ++it){
        if (*it == '('){
            cout << "   ";
            return;
        } 
        cout << *it;
    }
    
}


void ATPG::parse_diag_log(fstream& in){

    string gateName, observed, pattern, _, vec_no;
    string pure_pattern;
    int temp, c_pos, last = -1;     // c_pos record the position of "[" in vec_no
    //bool ob;

    while(1){
        // in >> _ >> vec_no >> gateName >> _ >> _ >> _ >> observed >> _ >> pattern;
        in >> vec_no >> gateName >> _ >> _ >> _ >> observed >> _ >> pattern;
        if (in.eof() == true) {
            break;
        }
        // temp = stoi(vec_no.substr (1, pattern.length()-1));
        // for (int i=vec_no.length();i>=0;i--){
        //     ctemp = i;
        //     if (vec_no[i] == '[') break;
        // }
        c_pos = vec_no.find("[");
        temp = stoi(vec_no.substr(c_pos+1, vec_no.length() - c_pos - 2));
        pure_pattern = pattern.substr (2, pattern.length()-3);
        if (temp != last) {
            last = temp;
            fail_vec_no.push_back(temp);
        }

        //if(observed == "H") ob = 1;
        //else ob = 0;
        //pair <string, bool> p;
        //p.first = gateName;
        //p.second = ob;

        //pattern_to_data[pure_pattern].push_back(p);
        //cout<<pure_pattern + gateName<<endl;
        pattern_to_data.insert({pure_pattern + gateName,false});
        all_fail_opGate.insert(gateName);

        auto it = fail_vec_to_FO.find(temp);
        if (it == fail_vec_to_FO.end()) {
            unordered_set<string> temp_FO;
            temp_FO.insert(gateName);
            fail_vec_to_FO.insert({temp,temp_FO});
        } else {
            it->second.insert(gateName);
        }
        total_TF++;
        // cout << "@@" << endl;
    }

    // unordered_set<string>::iterator h;
    // for( h=all_fail_opGate.begin();h!=all_fail_opGate.end();h++){
    //     cout << *h << endl;
    // }

    return;
}