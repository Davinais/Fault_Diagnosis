/**********************************************************************/
/*           main function for atpg                */
/*                                                                    */
/*           Author: Bing-Chen (Benson) Wu                            */
/*           last update : 01/21/2018                                 */
/**********************************************************************/

#include "atpg.h"

void usage();

int main(int argc, char *argv[]) {
  string inpFile, vetFile, faultin, flogFile;
  int i, j;
  ATPG atpg; // create an ATPG obj, named atpg

  atpg.timer(stdout, "START");
  atpg.detected_num = 1;
  i = 1;

/* parse the input switches & arguments */
  while (i < argc) {
    // number of test generation attempts for each fault.  used in podem.cpp
    if (strcmp(argv[i], "-anum") == 0) {
      atpg.set_total_attempt_num(atoi(argv[i + 1]));
      i += 2;
    } else if (strcmp(argv[i], "-bt") == 0) {
      atpg.set_backtrack_limit(atoi(argv[i + 1]));
      i += 2;
    } else if (strcmp(argv[i], "-fsim") == 0) {
      vetFile = string(argv[i + 1]);
      atpg.set_fsim_only(true);
      i += 2;
    } else if (strcmp(argv[i], "-tdfsim") == 0) {
      vetFile = string(argv[i + 1]);
      atpg.set_tdfsim_only(true);
      i += 2;
    }
    ////////////////////////  final  ////////////////////////
    else if (strcmp(argv[i], "-genFailLog") == 0) {
      vetFile = string(argv[i + 1]);
      atpg.set_genFailLog_only(true);
      i += 2;
    }
    else if (strcmp(argv[i], "-fault") == 0) {
      faultin = string(argv[i + 1]);
      atpg.set_genFailLog_Wire(string(argv[i + 1]));
      atpg.set_genFailLog_Gate(string(argv[i + 2]));
      atpg.set_genFailLog_IO(string(argv[i + 3]));
      atpg.set_genFailLog_Type(string(argv[i + 4]));
      i += 5;
    }
    else if (strcmp(argv[i], "-diag") == 0) {
      vetFile = string(argv[i + 1]);
      atpg.set_diag_only(true);
      i += 2;
    }
    /////////////////////////////////////////////////////////

      // for N-detect fault simulation
    else if (strcmp(argv[i], "-ndet") == 0) {
      atpg.detected_num = atoi(argv[i + 1]);
      i += 2;
    } else if (argv[i][0] == '-') {
      j = 1;
      while (argv[i][j] != '\0') {
        if (argv[i][j] == 'd') {
          j++;
        } else {
          fprintf(stderr, "atpg: unknown option\n");
          usage();
        }
      }
      i++;
    } 
    else {
      string temp = string(argv[i]);
      i++;
      if (temp.compare(temp.size()-3,3,"ckt") == 0) {
        inpFile = temp;
      }
      if (temp.compare(temp.size()-7,7,"failLog") == 0) { 
        flogFile = temp;
        atpg.diagname = temp.substr(temp.find("failLog/")+7,temp.size());
      }
    }

  }

  /* an input file was not specified, so describe the proper usage */
  if (inpFile.empty()) { usage(); }
  else { 
    fstream in;
    in.open(inpFile, ios::in);
    atpg.input(inpFile); // input.cpp
  }
  /* read in and parse the input file */
  if (!flogFile.empty()) {
    fstream in;
    in.open(flogFile, ios::in);
    atpg.parse_diag_log(in);   
  }
    // if(inpFile.compare(inpFile.size()-3,3,"ckt") == 0) {
    //   fstream in;
    //   in.open(inpFile, ios::in);
    //   atpg.input(inpFile); // input.cpp
    //         cout<<inpFile;
    //     cout<<"xxxxx";
    // } 
    // else if (inpFile.compare(inpFile.size()-7,7,"failLog") == 0) {    // parse in faillog
    //   fstream in;
    //   in.open(inpFile, ios::in);
    //   atpg.parse_diag_log(in);   
    //         cout<<inpFile;
    //     cout<<"======================";  
    // }



/* if vector file is provided, read it */
  if (!vetFile.empty()) { atpg.read_vectors(vetFile); }
  if(!atpg.get_genFailLog_only()) atpg.timer(stdout, "for reading in circuit");

  atpg.level_circuit();  // level.cpp
  if(!atpg.get_genFailLog_only())atpg.timer(stdout, "for levelling circuit");

  atpg.rearrange_gate_inputs();  //level.cpp
  if(!atpg.get_genFailLog_only())atpg.timer(stdout, "for rearranging gate inputs");

  atpg.create_dummy_gate(); //init_flist.cpp
  if(!atpg.get_genFailLog_only())atpg.timer(stdout, "for creating dummy nodes");

  if ((!atpg.get_tdfsim_only()) && (!atpg.get_genFailLog_only()) && (!atpg.get_diag_only())) atpg.generate_fault_list(); //init_flist.cpp
  else if(!atpg.get_tdfsim_only() && (!atpg.get_diag_only())) atpg.generate_genFailLog_list(); //defined in genFailLog.cpp
  else if(!atpg.get_diag_only()) atpg.generate_tdfault_list();


  if(!atpg.get_genFailLog_only() && (!atpg.get_diag_only()))atpg.timer(stdout, "for generating fault list");

  if (!atpg.get_diag_only()) atpg.test(); //defined in atpg.cpp
  else {  
    // TODO : diagnosis
    atpg.diagnosis();
    // END TODO
  }

  if ((!atpg.get_tdfsim_only()) && (!atpg.get_genFailLog_only()) && (!atpg.get_diag_only()))atpg.compute_fault_coverage(); //init_flist.cpp
  if(!atpg.get_genFailLog_only() && (!atpg.get_diag_only()))atpg.timer(stdout, "for test pattern generation");
  exit(EXIT_SUCCESS);
}

void usage() {

  fprintf(stderr, "usage: atpg [options] infile\n");
  fprintf(stderr, "Options\n");
  fprintf(stderr, "    -fsim <filename>: fault simulation only; filename provides vectors\n");
  fprintf(stderr, "    -anum <num>: <num> specifies number of vectors per fault\n");
  fprintf(stderr, "    -bt <num>: <num> specifies number of backtracks\n");
  exit(EXIT_FAILURE);

} /* end of usage() */




void ATPG::set_fsim_only(const bool &b) {
  this->fsim_only = b;
}

void ATPG::set_tdfsim_only(const bool &b) {
  this->tdfsim_only = b;
}

void ATPG::set_genFailLog_only(const bool &b) {
  this->genFailLog_only = b;
}
void ATPG::set_diag_only(const bool &b) {
  this->diag_only = b;
}

void ATPG::set_total_attempt_num(const int &i) {
  this->total_attempt_num = i;
}

void ATPG::set_backtrack_limit(const int &i) {
  this->backtrack_limit = i;
}
