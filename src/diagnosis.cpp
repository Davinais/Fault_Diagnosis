/**********************************************************************/
/*                  Diagnosis                                         */
/*                                                                    */
/*           Author: Cheng-Sian Kuo                                   */
/*           last update : 05/21/2021                                 */
/**********************************************************************/

#include "atpg.h"
#include <stack>
#include <unordered_map>
#include <unordered_set>

#define num_of_faults_in_parallel 16
#define SA1 1
#define SA0 0
#define CONFLICT 2

bool ATPG::compareScore(fptr f1, fptr f2) {
    if (f1->score > f2->score) return true;
    else return false;
}

void ATPG::diagnosis() {
    fptr f;
    //cktout_value = new int(cktout.size());
    if (SSF_diagnosis()) {      // successful SSF diagnosis
      cout<<"Successful SSF!!!"<<endl;
      write_diagnosis_report();
    } else {                    // SSF diagnosis fails, need MSF diagnosis
      cout<<"Perform MSF!!!"<<endl;
      reset_flist();
      MSF_diagnosis();
    }
  //delete cktout_value;
}

void ATPG::MSF_diagnosis() {

  path_tracing();

  // auto final_set = fail_vec_to_fault.find(fail_vec_no[0]);
  // for (int i = 1; i < fail_vec_no.size(); i++) {
  //   auto curfailvec = fail_vec_to_fault.find(fail_vec_no[i]);
  //   for (auto pf : curfailvec->second) {
  //     final_set->second.insert(pf);
  //   }
  // }
  unordered_set<fptr> &final_set = fail_vec_to_fault[fail_vec_no[0]];
  for (auto& pf : final_set) {
    pf->TFSF++;
  }
  for (int i = 1; i < fail_vec_no.size(); i++) {
    auto& curfailvec = fail_vec_to_fault[fail_vec_no[i]];
    //cout<<fail_vec_no[i]<<" : "<<curfailvec.size()<<endl;
    for (auto& pf : curfailvec) {
      pf->TFSF++;
      final_set.insert(pf);
    }
  }
  result.clear();
  for (auto& pf : final_set) {
    pf->score = 100;
    result.push_back(pf);
  }
  write_diagnosis_report();
  // cout<<"Final suspects: "<<final_set.size()<<endl;
  // for (auto& pf : final_set) {
  //   cout<<pf->fault_no<<" - ";
  //   cout<<sort_wlist[pf->to_swlist]->name<<" ";
  //   cout<<pf->node->name<<" ";
  //   if (pf->io) cout<<"GO ";
  //   else cout<<"GI ";
  //   if (pf->fault_type) cout<<"SA1,  ";
  //   else cout<<"SA0,  :";
  //   cout<<pf->TFSF<<endl;
  // }
}

bool ATPG::SSF_diagnosis() {
    //filename = "c17";
    fptr f;
    generate_fault_list();
    build_mapping(false);
    //cout<<"finish build\n";
    find_suspects();
    //cout<<"After diagnosis:\n";
    // for (auto pos = flist_undetect.cbegin(); pos != flist_undetect.cend(); ++pos) {
    //     f = *pos;
    //     cout<<f->fault_no<<" TFSF: "<<f->TFSF<<" TPSF:"<<f->TPSF<< " TFSP:"<<f->TFSP;
    //     cout<<endl;
    // }

    // no matching suspect!!! SSF diagnosis fail
    if (flist_undetect.empty()) return false;
    findEQV();
    return evaluateResult();
    //write_diagnosis_report();
}

bool ATPG::evaluateResult() {
  fptr f;
  for (auto pos = flist_undetect.cbegin(); pos != flist_undetect.cend(); ++pos) {
    f = *pos;
    f->TFSP = total_TF - f->TFSF;
    // cout<<f->fault_no<<" TFSF: "<<f->TFSF<<" TPSF:"<<f->TPSF<< " TFSP:"<<f->TFSP;
    // cout<<" EQVF: "<<f->eqv_fault_num;
    // cout<<endl;
    f->score = static_cast<double>(f->TFSF) / static_cast<double>(f->TFSF + f->TPSF + f->TFSP) * 100.0;
    result.push_back(f);
  }
  sort(result.begin(), result.end(), compareScore);
  // check whether there is a perfect suspect
  if (result[0]->score == 100) return true;
  else return false;
}

void ATPG::find_suspects() {
    fptr f;
    int cur_id = 0, num = 0;
    structural_backtrace();
    // cout<<"After backtrace: ";
    // for (auto pos = flist_undetect.cbegin(); pos != flist_undetect.cend(); ++pos) {
    //     f = *pos;
    //     cout<<f->fault_no<<" ";
    // }
    //cout<<endl;
    // check the TFSF & TPSF in the faling pattern
    for (auto fail_no : fail_vec_no) {
        //cout<<vectors[fail_no]<<endl;
        fault_sim_a_failvector(vectors[fail_no]);
    }
    // reset f->detect
    for (auto pos = flist_undetect.cbegin(); pos != flist_undetect.cend(); ++pos) {
        f = *pos;
        f->detect = FALSE;
    }
    // check TPSF in the other pattern, improve resolution by removing suspects

    for (int i = vectors.size() - 1; i >= 0; i--) {
        if (cur_id == fail_vec_no.size()) {
            break;
        } else if (i == fail_vec_no[cur_id]) {
            cur_id++;
            continue;
        } else {
            fault_sim_a_vector(vectors[i], num);
        }
    }


}

// build the mapping between gate & fault, wire & fault, name to po
void ATPG::build_mapping(bool show) {
    fptr f;
    wptr w;
    for (auto pos = flist_undetect.cbegin(); pos != flist_undetect.cend(); ++pos) {
        f = *pos;
        string curgate = f->node->name;
        auto it = gate_to_fault.find(curgate);
        if (it == gate_to_fault.end()) {
            forward_list<fptr> temp;
            temp.push_front(f);
            gate_to_fault.insert({curgate,temp});
        } else {
            it->second.push_front(f);
        }
        // if (f->node->type == INPUT) cout<<"PI: ";
        // if (f->node->type == OUTPUT) cout<<"PO: ";
        // cout<<f->node->name<<endl; 
    }

    // for (auto pos = flist_undetect.cbegin(); pos != flist_undetect.cend(); ++pos) {
    //     f = *pos;
    //     auto it = wire_to_fault.find(f->to_swlist);
    //     if (it == wire_to_fault.end()) {
    //         forward_list<fptr> temp;
    //         temp.push_front(f);
    //         wire_to_fault.insert({f->to_swlist,temp});
    //     } else {
    //         it->second.push_front(f);
    //     }
    // }
    if (show) {
        // for (auto it = wire_to_fault.begin(); it != wire_to_fault.end(); it++) {
        //     cout<<it->first<<": ";
        //     for (auto f : it->second) {
        //         cout<<f->to_swlist<<","<<f->fault_no<<";";
        //     }
        //     cout<<endl;
        // }

        for (auto it = gate_to_fault.begin(); it != gate_to_fault.end(); it++) {
            cout<<it->first<<": ";
            for (auto f : it->second) {
                cout<<f->fault_no<<" ";
            }
            cout<<endl;
        }
    }
    for (int i = 0; i < cktout.size(); i++) {
        //size_t pos = cktout[i]->name.find("(");
        //string key = cktout[i]->name.substr(0,pos);
        string key = cktout[i]->name;
        name_to_po.insert({key,cktout[i]});
    }

}

/* fault simulate a single fail vector and record TFSF, TPSF, TFSP for each fault*/
void ATPG::fault_sim_a_failvector(const string &vec) {
  wptr w, faulty_wire;
  /* array of 16 fptrs, which points to the 16 faults in a simulation packet  */
  fptr simulated_fault_list[num_of_faults_in_parallel];
  fptr f;
  int fault_type;
  int i, start_wire_index, nckt;
  int num_of_fault;

  num_of_fault = 0; // counts the number of faults in a packet

  /* num_of_current_detect is used to keep track of the number of undetected faults
   * detected by this vector.  Initialize it to zero */
  //num_of_current_detect = 0;

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
  }

  sim(); /* do a fault-free simulation, see sim.cpp */
  if (debug) { display_io(); }

  /* expand the fault-free value into 32 bits (00 = logic zero, 11 = logic one, 01 = unknown)
   * and store it in wire_value1 (good value) and wire_value2 (faulty value)*/
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

  /* walk through every undetected fault
   * the undetected fault list is linked by pnext_undetect */
  for (auto pos = flist_undetect.cbegin(); pos != flist_undetect.cend(); ++pos) {
    //int fault_detected[num_of_faults_in_parallel] = {0}; //for n-det
    f = *pos;
    f->detect = FALSE;
    if (f->detect == REDUNDANT) { continue; } /* ignore redundant faults */
    /* consider only active (aka. excited) fault
     * (sa1 with correct output of 0 or sa0 with correct output of 1) */
    if (f->fault_type != sort_wlist[f->to_swlist]->value) {

      /* if f is a primary output or is directly connected to an primary output
       * the fault is detected */
      if ((f->node->type == OUTPUT) ||
          (f->io == GO && sort_wlist[f->to_swlist]->is_output())) {
        f->detect = TRUE;
        //string cur_wire;
        w = sort_wlist[f->to_swlist];
        //cur_wire = w->name.substr(0,w->name.find("("));
        auto it = pattern_to_data.find(vec+w->name);
        if (it != pattern_to_data.end()) {
            //cout<<cur_wire;
            f->TFSF++;
        }  else {
            f->TPSF++;
        }
      } else {
        /* if f is an gate output fault */
        if (f->io == GO) {

          /* if this wire is not yet marked as faulty, mark the wire as faulty
           * and insert the corresponding wire to the list of faulty wires. */
          if (!(sort_wlist[f->to_swlist]->is_faulty())) {
            sort_wlist[f->to_swlist]->set_faulty();
            wlist_faulty.push_front(sort_wlist[f->to_swlist]);
          }

          /* add the fault to the simulated fault list and inject the fault */
          simulated_fault_list[num_of_fault] = f;
          inject_fault_value(sort_wlist[f->to_swlist], num_of_fault, f->fault_type);

          /* mark the wire as having a fault injected
           * and schedule the outputs of this gate */
          sort_wlist[f->to_swlist]->set_fault_injected();
          for (auto pos_n : sort_wlist[f->to_swlist]->onode) {
            pos_n->owire.front()->set_scheduled();
          }

          /* increment the number of simulated faults in this packet */
          num_of_fault++;
          /* start_wire_index keeps track of the smallest level of fault in this packet.
           * this saves simulation time.  */
          start_wire_index = min(start_wire_index, f->to_swlist);
        }  // if gate output fault

          /* the fault is a gate input fault */
        else {

          /* if the fault is propagated, set faulty_wire equal to the faulty wire.
           * faulty_wire is the gate output of f.  */
          faulty_wire = get_faulty_wire(f, fault_type);
          if (faulty_wire != nullptr) {

            /* if the faulty_wire is a primary output, it is detected */
            if (faulty_wire->is_output()) {

              //f->detected_time++;
              //if (f->detected_time == detected_num) {
                f->detect = TRUE;
              //}
                //string cur_wire;
                w = f->node->owire.front();
                //cur_wire = w->name.substr(0,w->name.find("("));
                auto it = pattern_to_data.find(vec+w->name);
                if (it != pattern_to_data.end()) {
                    f->TFSF++;
                }  else {
                    f->TPSF++;
                }

            } else {
              /* if faulty_wire is not already marked as faulty, mark it as faulty
               * and add the wire to the list of faulty wires. */
              if (!(faulty_wire->is_faulty())) {
                faulty_wire->set_faulty();
                wlist_faulty.push_front(faulty_wire);
              }
              /* add the fault to the simulated list and inject it */
          
              simulated_fault_list[num_of_fault] = f;
              inject_fault_value(faulty_wire, num_of_fault, fault_type);

              /* mark the faulty_wire as having a fault injected
               *  and schedule the outputs of this gate */
              faulty_wire->set_fault_injected();
              for (auto pos_n : faulty_wire->onode) {
                pos_n->owire.front()->set_scheduled();
              }

              num_of_fault++;
              start_wire_index = min(start_wire_index, f->to_swlist);
            }
          }
        }
      
      } // if  gate input fault
    } // if fault is active

    /*
     * fault simulation of a packet
     */
    /* if this packet is full (16 faults)
     * or there is no more undetected faults remaining (pos points to the final element of flist_undetect),
     * do the fault simulation */
    if ((num_of_fault == num_of_faults_in_parallel) || (next(pos, 1) == flist_undetect.cend())) {

      /* starting with start_wire_index, evaulate all scheduled wires
       * start_wire_index helps to save time. */
      for (i = start_wire_index; i < nckt; i++) {
        if (sort_wlist[i]->is_scheduled()) {
          sort_wlist[i]->remove_scheduled();
          fault_sim_evaluate(sort_wlist[i]);
        }
      } /* event evaluations end here */

      /* pop out all faulty wires from the wlist_faulty
        * if PO's value is different from good PO's value, and it is not unknown
        * then the fault is detected.
        *
        * IMPORTANT! remember to reset the wires' faulty values back to fault-free values.
      */

      while (!wlist_faulty.empty()) {
        w = wlist_faulty.front();
        wlist_faulty.pop_front();
        w->remove_faulty();
        w->remove_fault_injected();
        w->set_fault_free();       
        /*
         * After simulation is done,if wire is_output(), we should compare good value(wire_value1) and faulty value(wire_value2). 
         * If these two values are different and they are not unknown, then the fault is detected.  We should update the simulated_fault_list.  Set detect to true if they are different.
         * Since we use two-bit logic to simulate circuit, you can use Mask[] to perform bit-wise operation to get value of a specific bit.
         * After that, don't forget to reset faulty values (wire_value2) to their fault-free values (wire_value1).
         */
        if (w->is_output()) { // if primary output
            bool SF = false, TF = false; 
            //string cur_wire;
            //cur_wire = w->name.substr(0,w->name.find("("));
            auto it = pattern_to_data.find(vec+w->name);
            //cout<<vec+cur_wire<<endl;
            if (it != pattern_to_data.end()) {
                //cout<<cur_wire;
                TF = true;
            } 

          for (i = 0; i < num_of_fault; i++) { // check every undetected fault
            SF = false;
            // check the value of good value and faulty value
            // if (simulated_fault_list[i]->fault_no == 7) {
            //     cout<<"Good: "<<(w->wire_value1& Mask[i])<<" Faulty: "<<(w->wire_value2& Mask[i])<<endl;
            // }
            //if (!(simulated_fault_list[i]->detect)) {
              if ((w->wire_value2 & Mask[i]) ^    // if value1 != value2
                  (w->wire_value1 & Mask[i])) {
                if (((w->wire_value2 & Mask[i]) ^ Unknown[i]) &&  // and not unknowns
                    ((w->wire_value1 & Mask[i]) ^ Unknown[i])) {
                    simulated_fault_list[i]->detect = TRUE;
                    SF = true;
                    // if (simulated_fault_list[i]->fault_no == 7) {
                    //     cout<<SF<<" "<<TF<<endl;
                    // }
                  //fault_detected[i] = 1;// then the fault is detected
                }
              }
            //}
            if (TF && SF) simulated_fault_list[i]->TFSF++;
            //else if (TF && !SF) simulated_fault_list[i]->TFSP++;
            else if (!TF && SF) simulated_fault_list[i]->TPSF++;

          }
        }
        w->wire_value2 = w->wire_value1;  // reset to fault-free values
      } // pop out all faulty wires
      num_of_fault = 0;  // reset the counter of faults in a packet
      start_wire_index = 10000;  //reset this index to a very large value.
    } // end fault sim of a packet
  } // end loop. for f = flist

  /* fault dropping  */
  // drop the fault that can't induce a faling output
  flist_undetect.remove_if(
      [&](const fptr fptr_ele) {
        if (fptr_ele->detect == false) {
          //num_of_current_detect += fptr_ele->eqv_fault_num;
          return true;
        }  
        else if (fptr_ele->TFSF == 0) {
            return true;
        } 
        else {
          return false;
        }
      });
}/* end of fault_sim_a_vector */

void ATPG::findEQV() {
    wptr w;
    fptr f;
    fptr head;
    for (auto pos = flist_undetect.cbegin(); pos != flist_undetect.cend(); ++pos) {
        f = *pos;
        find_EQVF_head(f, sort_wlist[f->to_swlist], f->fault_type);
        head = f->EQVF_head;
        w = sort_wlist[head->to_swlist];

        f->eqv_fault_list.push_front(
            new EQV_FAULT(w->inode.front(), GO, head->fault_type, head->to_swlist));
        
        if (head->eqv_fault_num > 1) backward_collect_EQVF(f, w, head->fault_type);
    }
}

void ATPG::find_EQVF_head(fptr target_fault, wptr w, bool cur_ftype) {
    nptr n;
    n = w->onode.front();
    if (target_fault->io == GI) {   // deal with GI fault (In fanout branch)
        target_fault->EQVF_head = target_fault;
        return;
    }
    if (cur_ftype == SA0) {
        /* Meet EQVF head, stop finding */
        if (w->onode.size() > 1 || n->type == EQV ||n->type == OR
            || n->type == NOR || n->type == XOR || n->type == OUTPUT) {
            for(auto f :gate_to_fault[w->inode.front()->name]) {
                if (f->io == GO && f->fault_type == SA0) {
                    target_fault->EQVF_head = f;
                    break;
                }
            }
            return;
        }
        switch (n->type) {
            case AND:
            case BUF:
                find_EQVF_head(target_fault, n->owire.front(), SA0);
                break;
            case NAND:
            case NOT:
                find_EQVF_head(target_fault, n->owire.front(), SA1);
                break;
            case INPUT:
                cout<<"[Error] wire output can't be PI!!!\n";
                break;
        }
    } else {
        /* Meet EQVF head, stop finding */
        if (w->onode.size() > 1 || n->type == EQV ||n->type == AND
            || n->type == NAND || n->type == XOR || n->type == OUTPUT) {
            for(auto f : gate_to_fault[w->inode.front()->name]) {
                if (f->io == GO && f->fault_type == SA1) {
                    target_fault->EQVF_head = f;
                    break;
                }
                
            }
            return;
        }
        switch (n->type) {
            case OR:
            case BUF:
                find_EQVF_head(target_fault, n->owire.front(), SA1);
                break;
            case NOR:
            case NOT:
                find_EQVF_head(target_fault, n->owire.front(), SA0);
                break;
            case INPUT:
                cout<<"[Error] wire output can't be PI!!!\n";
                break;
        }
    }
    return;
}

void ATPG::backward_collect_EQVF(fptr target_fault, wptr w, bool cur_ftype) {
    nptr n;
    n = w->inode.front();

    if (cur_ftype == SA0) {
        switch (n->type) {
            case AND:
            case BUF:
                for (wptr wptr_ele: w->inode.front()->iwire) {
                    if (wptr_ele->onode.size() > 1) {
                        target_fault->eqv_fault_list.push_front(
                            new EQV_FAULT(n, GI, SA0, wptr_ele->wlist_index));
                        continue;
                    } else {
                        target_fault->eqv_fault_list.push_front(
                            new EQV_FAULT(wptr_ele->inode.front(), GO, SA0, wptr_ele->wlist_index));
                    }
                    // only need to backwardtrace the following gate
                    nptr n1 = wptr_ele->inode.front();  
                    if (n1->type == AND || n1->type == BUF || n1->type == NOT || n1->type == NOR) {
                        backward_collect_EQVF(target_fault, wptr_ele, SA0);
                    }
    
                }
                break;
            case NOR:
            case NOT:
                for (wptr wptr_ele: w->inode.front()->iwire) {
                    if (wptr_ele->onode.size() > 1) {
                        target_fault->eqv_fault_list.push_front(
                            new EQV_FAULT(n, GI, SA1, wptr_ele->wlist_index));
                        continue;
                    } else {
                        target_fault->eqv_fault_list.push_front(
                            new EQV_FAULT(wptr_ele->inode.front(), GO, SA1, wptr_ele->wlist_index));
                    }
                    // only need to backwardtrace the following gate
                    nptr n1 = wptr_ele->inode.front();  
                    if (n1->type == OR || n1->type == BUF || n1->type == NOT || n1->type == NAND) {
                        backward_collect_EQVF(target_fault, wptr_ele, SA1);
                    }

                }
                break;
        }
    } else {
        switch (n->type) {
            case OR:
            case BUF:
                for (wptr wptr_ele: w->inode.front()->iwire) {
                    if (wptr_ele->onode.size() > 1) {
                        target_fault->eqv_fault_list.push_front(
                            new EQV_FAULT(n, GI, SA1, wptr_ele->wlist_index));
                        continue;
                    } else {
                        target_fault->eqv_fault_list.push_front(
                            new EQV_FAULT(wptr_ele->inode.front(), GO, SA1, wptr_ele->wlist_index));
                    }
                    // only need to backwardtrace the following gate
                    nptr n1 = wptr_ele->inode.front();  
                    if (n1->type == OR || n1->type == BUF || n1->type == NOT || n1->type == NAND) {
                        backward_collect_EQVF(target_fault, wptr_ele, SA1);
                    }


                }
                break;
            case NAND:
            case NOT:
                for (wptr wptr_ele: w->inode.front()->iwire) {
                    if (wptr_ele->onode.size() > 1) {
                        target_fault->eqv_fault_list.push_front(
                            new EQV_FAULT(n, GI, SA0, wptr_ele->wlist_index));
                        continue;
                    } else {
                        target_fault->eqv_fault_list.push_front(
                            new EQV_FAULT(wptr_ele->inode.front(), GO, SA0, wptr_ele->wlist_index));
                    }
                    // only need to backwardtrace the following gate
                    nptr n1 = wptr_ele->inode.front();  
                    if (n1->type == AND || n1->type == BUF || n1->type == NOT || n1->type == NOR) {
                        backward_collect_EQVF(target_fault, wptr_ele, SA0);
                    }

                }
                break;
        }
    }
}

void ATPG::structural_backtrace() {
    wptr w;
    fptr f;
    int num_fpo = 0;
    for (auto fpo : all_fail_opGate) {
        num_fpo++;
        w = name_to_po[fpo];
        _curfo = w->wlist_index;
        trace_cone(w);
    }
    flist_undetect.remove_if(
        [&](const fptr fptr_ele) { if (fptr_ele->fpo_num != num_fpo) return true;
        else return false;
        });
}

void ATPG::trace_cone(wptr w) {
    stack<nptr> n_stack;
    n_stack.push(w->inode.front());
    unordered_set<nptr> visited;
    visited.reserve(ncktnode);

    while(!n_stack.empty()) {
      nptr curnode = n_stack.top();
      n_stack.pop();
      if (visited.find(curnode) == visited.end()) {
        visited.insert(curnode);
        if (curnode->curfo != _curfo) {
            for (auto& f : gate_to_fault[curnode->name]) {
                f->fpo_num++;
            }
            curnode->curfo = _curfo;
        } 

        // Trace until reaching PI
        if (curnode->type != INPUT) {
          for (auto& wi : curnode->iwire) {
              n_stack.push(wi->inode.front());
          }
        }
      }
    }
}

void ATPG::reset_flist() {
  fptr f;
  flist_undetect.clear();

  for (auto& f : flist) {
    f->TFSF = 0;
    f->TPSF = 0;
    f->TFSP = 0;
    f->score = 0.0;
    f->detect = false;
    flist_undetect.push_front(f.get());
  }
  flist_undetect.reverse();
}

void ATPG::path_tracing() {
  int i, nckt;
  int last_total_pf = 0;
  int cur_total_pf = 0;
  fptr pf;      // potential fault site
  wptr w;
  int cktout_temp[cktout.size()];
  //fptr f;
  //int num_fpo = 0;

  // first for loop, find the potential faults for each pattern
  for (auto fail_no : fail_vec_no) {
    //int fail_no = 52;
    // cout<<"vec["<<fail_no<<"]: ";

    //bool backward_eliminate = false;
    /* for every input, set its value to the current vector value */
    for (i = 0; i < cktin.size(); i++) {
      cktin[i]->value = ctoi(vectors[fail_no][i]);
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
    }
    sim(); /* do a fault-free simulation, see sim.cpp */
    // for (int i = 0; i < sort_wlist.size(); i++) {
    //   cout<<sort_wlist[i]->name<<" : "<<sort_wlist[i]->value<<endl;
    // }
    // for (int i = 0; i < cktout.size(); i++) {
    //   cktout_temp[i] = cktout[i]->value;
    // }
    for (auto& fpo : fail_vec_to_FO[fail_no]) {
        w = name_to_po[fpo];
        w->fail_vec_no = fail_no;
        _curfo = w->wlist_index;
        path_trace_cone(w, fail_no);
    }
    // for (auto& pf : fail_vec_to_fault[fail_no]) {
    //   cout<<pf->fault_no<<" ";
    // }
    // cout<<endl;
    //cur_total_pf += fail_vec_to_fault[fail_no].size();
  }

  for (auto& fail_no : fail_vec_no) {
    int iter = 0;
    bool init_output = true;
    cur_total_pf = fail_vec_to_fault[fail_no].size();
    last_total_pf = cur_total_pf + 1;
    //cout<<"vec["<<fail_no<<"]: ";
    while (cur_total_pf < last_total_pf) {
      last_total_pf = cur_total_pf;
      auto &potential_fault_set = fail_vec_to_fault[fail_no];
      bool backward_eliminate = false;

      /* for every input, set its value to the current vector value */
      for (i = 0; i < cktin.size(); i++) {
        cktin[i]->value = ctoi(vectors[fail_no][i]);
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
      }

      sim(); /* do a fault-free simulation, see sim.cpp */

      if (init_output) {
        for (int i = 0; i < cktout.size(); i++) {
          cktout_temp[i] = cktout[i]->value;
        }
        init_output = false;
      }
      for (auto& pf : potential_fault_set) {
        X_propagate(sort_wlist[pf->to_swlist]);
      }
      //cout<<"After x_propagate: ";
      // for (int i = 0; i < cktout.size(); i++) {
      //   cout<<i<<" : "<<cktout[i]->value<<endl;
      // }

      for (int i = 0; i < cktout.size(); i++) {
        if (cktout[i]->fail_vec_no != fail_no && cktout[i]->value == U) {
          backward_eliminate = true;
          cktout[i]->value = cktout_temp[i];
          if (C_backward_imply(cktout[i], cktout[i]->value) == CONFLICT) cout<<"[Error] Backward implication fail!!!\n";
        }
        // cout<<i<<" : "<<cktout[i]->value<<endl;
      }
      if (backward_eliminate) {
        //cout<<"  Perform "<<iter<<"th backward elimination!!!\n";
        for (auto pos = potential_fault_set.begin(), last = potential_fault_set.end(); pos != last; ) {
          pf = *pos;
          if ((pf->fault_type == sort_wlist[pf->to_swlist]->value) 
                  && sort_wlist[pf->to_swlist]->value != U) {
            pos = potential_fault_set.erase(pos);
          } else {
            ++pos;
          }
        }
      // cout<<"new: "<<potential_fault_set.size();
      // cout<<" ori: "<<last_total_pf<<endl;
      }
      iter++;

      cur_total_pf = fail_vec_to_fault[fail_no].size();
    }
    //cout<<endl;
  } 
}

void ATPG::path_trace_cone(wptr w, int fail_no) {
    unordered_set<fptr> temp;
    stack<nptr> n_stack;
    n_stack.push(w->inode.front());

    unordered_set<nptr> visited;
    visited.reserve(ncktnode);
    // for (auto& f : gate_to_fault[w->inode.front()->name]) {
    //   if (f->io == GO)
        // temp.push_front(f);
    // }
    fail_vec_to_fault.insert({fail_no, temp});
    auto curfailvec = fail_vec_to_fault.find(fail_no);

    while(!n_stack.empty()) {
      nptr curnode = n_stack.top();
      wptr curwire = curnode->owire.front();
      n_stack.pop();
      if (visited.find(curnode) == visited.end()) {
        visited.insert(curnode);
        switch (curnode->type) {
          case AND :
            if (curwire->value == 1) {
              for (auto& wi : curnode->iwire) 
                  n_stack.push(wi->inode.front());
              for (auto& f : gate_to_fault[curnode->name]) {
                  if (f->fault_type == SA0)     // only consider the fault that can be activated
                    curfailvec->second.insert(f);
              }
            } else {
              for (auto& wi : curnode->iwire) {
                if (wi->value == 0)
                  n_stack.push(wi->inode.front());
              }
              for (auto& f : gate_to_fault[curnode->name]) {
                if ((f->io == GI && sort_wlist[f->to_swlist]->value == 0) || f->io == GO) {
                  if (f->fault_type == SA1)
                    curfailvec->second.insert(f);
                }
              }   
            }
            break;
          case NOR :
            if (curwire->value == 1) {
              for (auto& wi : curnode->iwire) 
                  n_stack.push(wi->inode.front());
              for (auto& f : gate_to_fault[curnode->name]) 
                  if ((f->io == GI && f->fault_type == SA1) || (f->io == GO && f->fault_type == SA0))
                    curfailvec->second.insert(f);
            } else {
              for (auto& wi : curnode->iwire) {
                if (wi->value == 1)
                  n_stack.push(wi->inode.front());
              }
              for (auto& f : gate_to_fault[curnode->name]) {
                if ((f->io == GI && sort_wlist[f->to_swlist]->value == 1 && f->fault_type == SA0) 
                    || (f->io == GO && f->fault_type == SA1))
                  curfailvec->second.insert(f);
              }   
            }
            break;
          case NAND :
            if (curwire->value == 0) {

              for (auto& wi : curnode->iwire) 
                  n_stack.push(wi->inode.front());
              for (auto& f : gate_to_fault[curnode->name]) {
                  // if (sort_wlist[f->to_swlist]->name == "3GAT(2)") {
                  //   cout<<curnode->name<<" ";
                  //   cout<<"3GAT: "<<sort_wlist[f->to_swlist]->value<<" "<< f->fault_type<<" "<<f->io<<endl;
                  // }
                  if ((f->io == GI && f->fault_type == SA0) || (f->io == GO && f->fault_type == SA1))
                    curfailvec->second.insert(f);
              }
            } else {
              for (auto& wi : curnode->iwire) {
                if (wi->value == 0) 
                  n_stack.push(wi->inode.front());
              }
              for (auto& f : gate_to_fault[curnode->name]) {
                // if (sort_wlist[f->to_swlist]->name == "3GAT(2)") {
                //   cout<<curnode->name<<" ";
                //   cout<<"3GAT: "<<sort_wlist[f->to_swlist]->value<<" "<< f->fault_type<<" "<<f->io<<endl;
                // }
                if ((f->io == GI && sort_wlist[f->to_swlist]->value == 0 && f->fault_type == SA1) 
                    || (f->io == GO && f->fault_type == SA0))
                  curfailvec->second.insert(f);
              }   
            }   
            break;   
          case OR :
            if (curwire->value == 0) {
              for (auto& wi : curnode->iwire) 
                  n_stack.push(wi->inode.front());
              for (auto& f : gate_to_fault[curnode->name]) 
                if (f->fault_type == SA1)
                  curfailvec->second.insert(f);
            } else {
              for (auto& wi : curnode->iwire) {
                if (wi->value == 1)
                  n_stack.push(wi->inode.front());
              }
              for (auto& f : gate_to_fault[curnode->name]) {
                if ((f->io == GI && sort_wlist[f->to_swlist]->value == 1) || f->io == GO)
                  if (f->fault_type == SA0)
                    curfailvec->second.insert(f);
              }   
            }  
            break;
          case BUF :
          case NOT :
          case EQV :
          case XOR :
            for (auto& wi : curnode->iwire) 
              n_stack.push(wi->inode.front());
            for (auto& f : gate_to_fault[curnode->name]) 
                curfailvec->second.insert(f);
            break;
          case INPUT :
            for (auto& f : gate_to_fault[curnode->name]) 
                curfailvec->second.insert(f);
            break;
        }
      }
    }
}

// propagate X from potentail fault site to PO
void ATPG::X_propagate(wptr w) {
    stack<nptr> n_stack;
    nptr curnode;
    wptr curwire;
    n_stack.push(w->onode.front());
    unordered_set<nptr> visited;
    visited.reserve(ncktnode);
    w->value = U;
    while(!n_stack.empty()) {
      curnode = n_stack.top();
      //cout<<curnode->name<<"XX"<<n_stack.size()<<endl;
      n_stack.pop();
      //cout<<curnode->type<<endl;
      if (curnode->type != OUTPUT) {
        curwire = curnode->owire.front();
        //cout<<curwire->name<<" "<<endl;
      }
      else {
        //cout<<"keep going"<<endl;
        continue;
      }
      if (visited.find(curnode) == visited.end()) {
        visited.insert(curnode);
        // Trace until reaching PO
        if (curnode->type != OUTPUT) {
          curwire->value = U;
          //cout<<curwire->name<<" "<<endl;
          //cout<<curwire->onode.front()->type<<endl;
          for (auto& no : curwire->onode) {
              //cout<<no->type<<"..."<<endl;
              if (no->type == OUTPUT) {
                //cout<<no->name<<" , "<<endl;
                continue;
              }
              if (no->owire.front()->value != U) {
                // cout<<no->owire.front()->value<<" ";
                // cout<<no->owire.front()->name<<endl;
                n_stack.push(no);
              }
          }
          //cout<<" out "<<endl;
        }
      }
    }
    //cout<<"jump"<<endl;
}

/* Conservative backward implication */
// Try to turn unknown into certain value and eliminate suspects that is impossible   

int ATPG::C_backward_imply(const wptr current_wire, const int &desired_logic_value) {
  nptr current_node = current_wire->inode.front();
  int pi_is_reach = FALSE;
  int i, nin;
  nin = current_node->iwire.size();
  if (current_wire->is_input()) { // if PI
    if (current_wire->value != U &&
        current_wire->value != desired_logic_value) {
      return (CONFLICT); // conlict with previous assignment
    }
    current_wire->value = desired_logic_value; // assign PI to the objective value
    current_wire->set_changed();
    // CHANGED means the logic value on this wire has recently been changed
    return (TRUE);
  } else { // if not PI
    if (current_wire->value != U &&
        current_wire->value != desired_logic_value) {
      cout<<current_wire->name<<" "<<current_node->name;
      cout<<" Current value: "<<current_wire->value<<" Desired value: "<<desired_logic_value<<endl;
      return (CONFLICT); // conlict with previous assignment
    }
    current_wire->value = desired_logic_value; // assign PI to the objective value
    current_wire->set_changed();
    switch (current_node->type) {
      /* assign NOT input opposite to its objective ouput */
      /* go backward iteratively.  depth first search */
      case NOT:

        switch (C_backward_imply(current_node->iwire.front(), (desired_logic_value ^ 1))) {
          case TRUE:
            pi_is_reach = TRUE;
            break;
          case CONFLICT:
                cout<<current_wire->name<<" "<<current_node->name;
                cout<<" Current value: "<<current_wire->value<<" Desired value: "<<desired_logic_value<<endl;
            return (CONFLICT);
            break;
          case FALSE:
            break;
        }
        break;

        /* if objective is NAND output=zero, then NAND inputs are all ones
         * keep doing this back implication iteratively  */
      case NAND:
        if (desired_logic_value == 0) {
          for (i = 0; i < nin; i++) {
            switch (C_backward_imply(current_node->iwire[i], 1)) {
              case TRUE:
                pi_is_reach = TRUE;
                break;
              case CONFLICT:
                cout<<current_wire->name<<" "<<current_node->name;
                cout<<" Current value: "<<current_wire->value<<" Desired value: "<<desired_logic_value<<endl;
                return (CONFLICT);
                break;
              case FALSE:
                break;
            }
          }
        } else {    
        // if there is only one input with unknown, 
        //other value is non-controlling value, turn unknown into controlling value 
        // if find a input with controlling value, stop backward implication
          int control_idx = -1;
          bool only_control = false;
          for (i = 0; i < nin; i++) {
            if (current_node->iwire[i]->value == 0) {
              only_control = false;
              break;
            }
            else if (current_node->iwire[i]->value == U && control_idx == -1) {
              control_idx = i;
              only_control = true;
            } else if (current_node->iwire[i]->value == U && only_control == true) {
              only_control = false; 
              break;
            }
          }
          if (only_control == true) {
            switch (C_backward_imply(current_node->iwire[control_idx], 0)) {
              case TRUE:
                pi_is_reach = TRUE;
                break;
              case CONFLICT:
                cout<<current_wire->name<<" "<<current_node->name;
                cout<<" Current value: "<<current_wire->value<<" Desired value: "<<desired_logic_value<<endl; 
                return (CONFLICT);
                break;
              case FALSE:
                break;
            }
          }
        }
        break;

      case AND:
        if (desired_logic_value == 1) {
          for (i = 0; i < nin; i++) {
            switch (C_backward_imply(current_node->iwire[i], 1)) {
              case TRUE:
                pi_is_reach = TRUE;
                break;
              case CONFLICT:
                cout<<current_wire->name<<" "<<current_node->name;
                cout<<" Current value: "<<current_wire->value<<" Desired value: "<<desired_logic_value<<endl;
                return (CONFLICT);
                break;
              case FALSE:
                break;
            }
          }
        } else {    
        // if there is only one input with unknown, 
        //other value is non-controlling value, turn unknown into controlling value 
          int control_idx = -1;
          bool only_control = false;
          for (i = 0; i < nin; i++) {
            // if (current_node->name == "g291") {
            //   cout<<current_node->iwire[i]->name<<" : "<<current_node->iwire[i]->value<<" o ";
            // }
            if (current_node->iwire[i]->value == 0) {
              only_control = false;
              break;
            }
            else if (current_node->iwire[i]->value == U && control_idx == -1) {
              control_idx = i;
              only_control = true;
            } else if (current_node->iwire[i]->value == U && only_control == true) {
              only_control = false; 
              break;
            }
          }
          if (only_control == true) {
            switch (C_backward_imply(current_node->iwire[control_idx], 0)) {
              case TRUE:
                pi_is_reach = TRUE;
                break;
              case CONFLICT:
                cout<<current_wire->name<<" "<<current_node->name;
                cout<<" Current value: "<<current_wire->value<<" Desired value: "<<desired_logic_value<<endl;
                return (CONFLICT);
                break;
              case FALSE:
                break;
            }
          }
        }
        break;

      case OR:
        if (desired_logic_value == 0) {
          for (i = 0; i < nin; i++) {
            switch (C_backward_imply(current_node->iwire[i], 0)) {
              case TRUE:
                pi_is_reach = TRUE;
                break;
              case CONFLICT:
                cout<<current_wire->name<<" "<<current_node->name;
                cout<<" Current value: "<<current_wire->value<<" Desired value: "<<desired_logic_value<<endl;                
                return (CONFLICT);
                break;
              case FALSE:
                break;
            }
          }
        } else {  
          // if there is only one input with unknown,   
          //other value is non-controlling value, turn unknown into controlling value 
          int control_idx = -1;
          bool only_control = false;
          for (i = 0; i < nin; i++) {
            if (current_node->iwire[i]->value == 1) {
              only_control = false;
              break;
            } else if (current_node->iwire[i]->value == U && control_idx == -1) {
              control_idx = i;
              only_control = true;
            } else if (current_node->iwire[i]->value == U && only_control == true) {
              only_control = false; 
              break;
            }
          }
          if (only_control == true) {
            switch (C_backward_imply(current_node->iwire[control_idx], 1)) {
              case TRUE:
                pi_is_reach = TRUE;
                break;
              case CONFLICT:
                cout<<current_wire->name<<" "<<current_node->name;
                cout<<" Current value: "<<current_wire->value<<" Desired value: "<<desired_logic_value<<endl;
                return (CONFLICT);
                break;
              case FALSE:
                break;
            }
          }
        }
        break;

      case NOR:
        if (desired_logic_value == 1) {
          for (i = 0; i < nin; i++) {
            switch (C_backward_imply(current_node->iwire[i], 0)) {
              case TRUE:
                pi_is_reach = TRUE;
                break;
              case CONFLICT:
                cout<<current_wire->name<<" "<<current_node->name;
                cout<<" Current value: "<<current_wire->value<<" Desired value: "<<desired_logic_value<<endl;
                return (CONFLICT);
                break;
              case FALSE:
                break;
            }
          }
        } else {    
          // if there is only one input with unknown, 
          //other value is non-controlling value, turn unknown into controlling value 
          int control_idx = -1;
          bool only_control = false;
          for (i = 0; i < nin; i++) {
            if (current_node->iwire[i]->value == 1) {
              only_control = false;
              break;
            } else if (current_node->iwire[i]->value == U && control_idx == -1) {
              control_idx = i;
              only_control = true;
            } else if (current_node->iwire[i]->value == U && only_control == true) {
              only_control = false; 
              break;
            }
          }
          if (only_control == true) {
            switch (C_backward_imply(current_node->iwire[control_idx], 1)) {
              case TRUE:
                pi_is_reach = TRUE;
                break;
              case CONFLICT:
                cout<<current_wire->name<<" "<<current_node->name;
                cout<<" Current value: "<<current_wire->value<<" Desired value: "<<desired_logic_value<<endl;
                return (CONFLICT);
                break;
              case FALSE:
                break;
            }
          }
        }
        break;

      case BUF:
        switch (C_backward_imply(current_node->iwire.front(), desired_logic_value)) {
          case TRUE:
            pi_is_reach = TRUE;
            break;
          case CONFLICT:
            cout<<current_wire->name<<" "<<current_node->name;
            cout<<" Current value: "<<current_wire->value<<" Desired value: "<<desired_logic_value<<endl;
            return (CONFLICT);
            break;
          case FALSE:
            break;
        }
        break;
    }

    return (pi_is_reach);
  }
}/* end of C_backward_imply */
