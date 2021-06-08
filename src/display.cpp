/**********************************************************************/
/*           Printout information                                     */
/*                                                                    */
/*           Author: Bing-Chen (Benson) Wu                            */
/*           last update : 01/21/2018                                 */
/**********************************************************************/

#include "atpg.h"

void ATPG::display_line(fptr fault) {
  int i;

  for (i = 0; i < cktin.size(); i++) {
    switch (cktin[i]->value) {
      case 0:
        fprintf(stdout, "0");
        break;
      case 1:
        fprintf(stdout, "1");
        break;
      case U:
        fprintf(stdout, "x");
        break;
      case D:
        fprintf(stdout, "D");
        break;
      case D_bar:
        fprintf(stdout, "B");
        break;
    }
  }
  fprintf(stdout, " ");
  for (i = 0; i < fault->node->iwire.size(); i++) {
    fprintf(stdout, "%s = ", fault->node->iwire[i]->name.c_str());
    switch (fault->node->iwire[i]->value) {
      case 0:
        fprintf(stdout, "0");
        break;
      case 1:
        fprintf(stdout, "1");
        break;
      case U:
        fprintf(stdout, "x");
        break;
      case D:
        fprintf(stdout, "D");
        break;
      case D_bar:
        fprintf(stdout, "B");
        break;
    }
    fprintf(stdout, "; ");
  }
  fprintf(stdout, "output = ");
  switch (fault->node->owire.front()->value) {
    case 0:
      fprintf(stdout, "0");
      break;
    case 1:
      fprintf(stdout, "1");
      break;
    case U:
      fprintf(stdout, "x");
      break;
    case D:
      fprintf(stdout, "D");
      break;
    case D_bar:
      fprintf(stdout, "B");
      break;
  }
  fprintf(stdout, "\n");
}/* end of display_line */

/*
* print primary input and output lines' values
*/
void ATPG::display_io() {
  int i;

  fprintf(stdout, "T\'");
  for (i = 0; i < cktin.size(); i++) {
    switch (cktin[i]->value) {
      case 0:
        fprintf(stdout, "0");
        break;
      case 1:
        fprintf(stdout, "1");
        break;
      case U:
        fprintf(stdout, "x");
        break;
      case D:
        fprintf(stdout, "1");
        break;
      case D_bar:
        fprintf(stdout, "0");
        break;
    }
  }
  fprintf(stdout, "'");
/*  debug use
  fprintf(stdout," ");
  for (i = 0; i < cktout.size(); i++) {
    switch (cktout[i]->value) {
      case 0: fprintf(stdout,"0"); break;
      case 1: fprintf(stdout,"1"); break;
      case U: fprintf(stdout,"x"); break;
      case D: fprintf(stdout,"D"); break;
      case B: fprintf(stdout,"B"); break;
    }
  }
*/
  fprintf(stdout, "\n");
}/* end of display_io */


void ATPG::display_undetect() {
  int i;
  wptr w;
  string ufile = filename + ".uf";

  ofstream file(ufile, std::ifstream::out | std::ofstream::app); // open the input vectors' file
  if (!file) { // if the ofstream obj does not exist, fail to open the file
    fprintf(stderr, "File %s could not be opened\n", ufile.c_str());
    exit(EXIT_FAILURE);
  }
  for (fptr f: flist_undetect) {
    switch (f->node->type) {
      case INPUT:
        file << "primary input: " << f->node->owire.front()->name << endl;
        break;
      case OUTPUT:
        file << "primary output: " << f->node->iwire.front()->name << endl;
        break;
      default:
        file << "gate: " << f->node->name << " ;";
        if (f->io == GI) {
          file << "input wire name: " << f->node->iwire[f->index]->name << endl;
        } else {
          file << "output wire name: " << f->node->owire.front()->name << endl;
        }
        break;
    }
    file << "fault_type = ";
    switch (f->fault_type) {
      case STUCK0:
        file << "s-a-0\n";
        break;
      case STUCK1:
        file << "s-a-1\n";
        break;
    }
    file <<"fault I/O = ";
    switch (f->io) {
      case GI:
        file << "GI\n";
        break;
      case GO:
        file << "GO\n";
        break;
    }
    file << "detection flag =";
    switch (f->detect) {
      case FALSE:
        file << " aborted\n";
        break;
      case REDUNDANT:
        file << " redundant\n";
        break;
      case TRUE:
        file << " internal error\n";
        break;
    }
    file << "fault no. = " << f->fault_no << endl;
    file << endl;
  }
  file.close();
}/* end of display_undetect */


void ATPG::display_fault(fptr f) {
  switch (f->node->type) {
    case INPUT:
      fprintf(stdout, "primary input: %s\n", f->node->owire.front()->name.c_str());
      break;
    case OUTPUT:
      fprintf(stdout, "primary output: %s\n", f->node->iwire.front()->name.c_str());
      break;
    default:
      fprintf(stdout, "gate: %s ;", f->node->name.c_str());
      if (f->io == GI) {
        fprintf(stdout, "input wire name: %s\n",
                f->node->iwire[f->index]->name.c_str());
      } else {
        fprintf(stdout, "output wire name: %s\n",
                f->node->owire.front()->name.c_str());
      }
      break;
  }
  fprintf(stdout, "fault_type = ");
  switch (f->fault_type) {
    case STUCK0:
      fprintf(stdout, "s-a-0\n");
      break;
    case STUCK1:
      fprintf(stdout, "s-a-1\n");
      break;
  }

  fprintf(stdout, "detection flag =");
  switch (f->detect) {
    case FALSE:
      fprintf(stdout, " aborted\n");
      break;
    case REDUNDANT:
      fprintf(stdout, " redundant\n");
      break;
    case TRUE:
      fprintf(stdout, " internal error\n");
      break;
  }
  fprintf(stdout, "\n");
}/* end of display_fault */


void ATPG::write_diagnosis_report() {
  fptr f;
  wptr w;
  int eqpos;
  double score;
  int fnum = 0;

  cout<<"The diagnosis report is generated at "<<dfile<<endl;
  ofstream file(dfile, std::ifstream::out | std::ofstream::app); // open the input vectors' file
  if (!file) { // if the ofstream obj does not exist, fail to open the file
    fprintf(stderr, "File %s could not be opened\n", dfile.c_str());
    cout<<"The outputfile should be "<<dfile<<endl;
    cout<<"Please check if the directory already exists, if not, create it...\n";
    exit(EXIT_FAILURE);
  }

  file<<"#Circuit Summary:\n";
  file<<"#---------------\n";
  file<<"#number of inputs = "<<cktin.size()<<"\n";
  file<<"#number of outputs = "<<cktout.size()<<"\n";
  file<<"#number of gates = "<<ncktnode<<"\n";
  file<<"#number of wires = "<<ncktwire<<"\n";
  file<<"#number of vectors = "<<vectors.size()<<"\n";
  file<<"#number of failing outputs = "<<pattern_to_data.size()<<"\n\n";
  file<<"Ranked suspect faults\n";
  
  for (auto pos = result.cbegin(); pos != result.cend(); ++pos) {
    fnum++;
    f = *pos;
    if (f->score < 90) break;
    w = sort_wlist[f->to_swlist];
    //file<<"No."<<fnum<<"  "<<w->name.substr(0,w->name.find("("))<<" ";
    file<<"No."<<fnum<<"  "<<w->name<<" ";
    file<<f->node->name<<" ";
    if (f->io) file<<"GO ";
    else file<<"GI ";
    if (f->fault_type) file<<"SA1,  ";
    else file<<"SA0,  ";
    file<<"TFSF="<<f->TFSF<<", TPSF="<<f->TPSF<<", TFSP="<<f->TFSP<<", score=";
    //score = static_cast<double>(f->TFSF) / static_cast<double>(f->TFSF + f->TPSF + f->TFSP);
    if (f->eqv_fault_num == -1) {
      file<<setprecision(5)<<f->score<<" [equivalent faults: ";
      //for (auto pos = f->eqv_fault_list.begin(); pos != f->eqv_fault_list.end() ++pos) {
      eqpos = 1;
      //cout<<f->eqv_fault_num<<"\n\n\n";
      for (auto ef : f->eqv_fault_list) {
        w = sort_wlist[ef->to_swlist];
        file<<w->name.substr(0,w->name.find("("))<<" "<<ef->node->name<<" ";
        if (ef->io) file<<"GO ";
        else file<<"GI ";
        if (ef->fault_type) file<<"SA1";
        else file<<"SA0";
        if (eqpos == f->eqv_fault_num) {
          file<<"]\n";
        } else {
          file<<" , ";
        }
        eqpos++;
      }
    } else {
      file<<setprecision(5)<<f->score<<"\n";
    }
  }

  double t_meas = (double) clock();
  file<<"#run time="<<(t_meas - StartTime) / CLOCKS_PER_SEC<<" s\n";

}


