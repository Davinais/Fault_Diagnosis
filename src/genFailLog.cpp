/**********************************************************************/
/*                  Generate Faillog                                  */
/*                                                                    */
/*           Author: I-Wei Chiu                                       */
/*           last update : 05/08/2021                                 */
/**********************************************************************/

#include "atpg.h"
#include <unordered_map>


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

    if (fault_IO[index] == "GO"){
        for (auto faultgate : w->inode){
            if (faultgate->name == fault_Gate_Pos[index]){
                n = faultgate;
                break;
            }
        }
    }
    else {
        for (auto faultgate : w->onode){
            if (faultgate->name == fault_Gate_Pos[index]){
                n = faultgate;
                break;
            }
        }
    }




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
    
    //  these are representative fault in the equivalent fault class
    //  For a wire w,  w.onode is a vector of w's fanout nodes.
    //   w->onode.size is the number of fanout nodes.  if w->onode.size >1, w is a fanout stem.
    //   w->onode.front()->type is the gate type at the output of w (assume w has only 1 fanout)
    //   w->onode.front()->type = OR means w is an input of OR gate.
    // if (w->onode.size() > 1 || w->onode.front()->type == EQV || w->onode.front()->type == OR
    //     || w->onode.front()->type == NOR || w->onode.front()->type == XOR || w->onode.front()->type == OUTPUT) {
    //   num_of_gate_fault += f->eqv_fault_num; // accumulate total uncollapsed faults
    //   flist_undetect.push_front(f.get()); // initial undetected fault list contains all faults
    //   flist.push_front(move(f));  // push into the fault list
    // } else {
    //   num_of_eqv_sa0[w] = f->eqv_fault_num;
    // }

    /* for each gate, create a gate output stuck-at one (SA1) fault */
    // f = move(fptr_s(new(nothrow) FAULT));

    // if (f == nullptr) error("No more room!");
    // f->eqv_fault_num = 1;
    // f->node = n;
    // f->io = GO;
    // f->fault_type = STUCK1;
    // f->to_swlist = w->wlist_index;
    // /* for OR NAND NOT BUF, their GI fault is equivalent to GO SA1 fault */
    // switch (n->type) {
    //   case OR:
    //   case BUF:
    //     for (wptr wptr_ele: w->inode.front()->iwire) {
    //       if (num_of_eqv_sa1.find(wptr_ele) == num_of_eqv_sa1.end())
    //         cerr << wptr_ele << " is not in hashmap." << endl;
    //       f->eqv_fault_num += num_of_eqv_sa1[wptr_ele];
    //     }
    //     break;
    //   case NAND:
    //   case NOT:
    //     for (wptr wptr_ele: w->inode.front()->iwire) {
    //       if (num_of_eqv_sa0.find(wptr_ele) == num_of_eqv_sa0.end())
    //         cerr << wptr_ele << " is not in hashmap." << endl;
    //       f->eqv_fault_num += num_of_eqv_sa0[wptr_ele];
    //     }
    //     break;
    //   case INPUT:
    //   case AND:
    //   case NOR:
    //   case EQV:
    //   case XOR:
    //     break;
    // }
    // if (w->onode.size() > 1 || w->onode.front()->type == EQV || w->onode.front()->type == AND
    //     || w->onode.front()->type == NAND || w->onode.front()->type == XOR || w->onode.front()->type == OUTPUT) {
    //   num_of_gate_fault += f->eqv_fault_num; // accumulate total fault count
    //   flist_undetect.push_front(f.get()); // initial undetected fault list contains all faults
    //   flist.push_front(move(f));  // push into the fault list
    // } else {
    //   num_of_eqv_sa1[w] = f->eqv_fault_num;
    // }

    /*if w has multiple fanout branches */
    // if (w->onode.size() > 1) {
    //   num_of_eqv_sa0[w] = num_of_eqv_sa1[w] = 1;
    //   for (nptr nptr_ele: w->onode) {
    //     /* create SA0 for OR NOR EQV XOR gate inputs  */
    //     switch (nptr_ele->type) {
    //       case OUTPUT:
    //       case OR:
    //       case NOR:
    //       case EQV:
    //       case XOR:
    //         f = move(fptr_s(new(nothrow) FAULT));
    //         if (f == nullptr) error("No more room!");
    //         f->node = nptr_ele;
    //         f->io = GI;
    //         f->fault_type = STUCK0;
    //         f->to_swlist = w->wlist_index;
    //         f->eqv_fault_num = 1;
    //         /* f->index is the index number of gate input,
    //            which GI fault is associated with*/
    //         for (int k = 0; k < nptr_ele->iwire.size(); k++) {
    //           if (nptr_ele->iwire[k] == w) f->index = k;
    //         }
    //         num_of_gate_fault++;
    //         flist_undetect.push_front(f.get());
    //         flist.push_front(move(f));
    //         break;
    //     }
    //     /* create SA1 for AND NAND EQV XOR gate inputs  */
    //     switch (nptr_ele->type) {
    //       case OUTPUT:
    //       case AND:
    //       case NAND:
    //       case EQV:
    //       case XOR:
    //         f = move(fptr_s(new(nothrow) FAULT));
    //         if (f == nullptr) error("No more room!");
    //         f->node = nptr_ele;
    //         f->io = GI;
    //         f->fault_type = STUCK1;
    //         f->to_swlist = w->wlist_index;
    //         f->eqv_fault_num = 1;
    //         for (int k = 0; k < nptr_ele->iwire.size(); k++) {
    //           if (nptr_ele->iwire[k] == w) f->index = k;
    //         }
    //         num_of_gate_fault++;
    //         flist_undetect.push_front(f.get());
    //         flist.push_front(move(f));
    //         break;
    //     }
    //   }
    // }
  }
  flist.reverse();
  flist_undetect.reverse();
  /*walk through all faults, assign fault_no one by one  */
  fault_num = 0;
  for (fptr f: flist_undetect) {
    f->fault_no = fault_num;
    fault_num++;
    //cout << f->fault_no << f->node->name << ":" << (f->io?"O":"I") << (f->io?9:(f->index)) << "SA" << f->fault_type << endl;
  }

  fprintf(stdout, "#number of equivalent faults = %d\n", fault_num);
}/* end of generate_fault_list */
