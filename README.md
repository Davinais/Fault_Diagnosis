# Fault_Diagnosis

## Introduction:
	As the best test expert in our NTU-ATPG company, your team are asked to implement 
a diagnosis tool for single stuck-at faults and multiple stuck-at faults.  To test 
our tool, we will have to first generate fail log files.  Then you will need to 
diagnose these fail log files and give a ranked list of suspects.  The failing output
will then be diagnosed by your tool.  This project will be graded based on diagnosis
accuracy, and diagnosis resolution, and run time. 


## Data Structure:
### fail_vector
This is a vector which record all pattern shows in faillog.
EX: 
	In fail log:
	
	*vector* *[1]*  *23GAT*   *expect* *L,* *observe H*   *#*   *T'10111'*
	
	*vector* *[0]*  *22GAT*   *expect* *L,* *observe H*   *#*   *T'00110'*
	
	*vector* *[0]*  *23GAT*   *expect* *L,* *observe H*   *#*   *T'00110'*

	fail_vector = 10111 -> 00110 -> 00110


### pattern_to_data
This is a map. The key of it is the pattern in fail log, and in each container is a vector of pair.
Each pair contains two objects, output gate's name and observed value.
EX:
	In fail log:
	
	*vector* *[1]*  *23GAT*   *expect* *L,* *observe H*   *#*   *T'10111'*
	
	*vector* *[0]*  *22GAT*   *expect* *L,* *observe H*   *#*   *T'00110'*
	
	*vector* *[0]*  *23GAT*   *expect* *L,* *observe H*   *#*   *T'00110'*

	pattern_to_date
		10111	->	<23GAT, 1>
		00110	->	<22GAT, 1> -> <23GAT, 1>
		
### all_fail_opGate
This unordered_set will record all output failling gate.


