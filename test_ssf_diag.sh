#!/bin/bash

# circuits=("c17" "c432" "c499" "c880" "c1355" "c2670" "c3540" "c6288" "c7552")
if [ $# -ne 2 ]; then
    echo "Usage: $0 <circuit_name> <sample_rate>"
    echo "  Ex.  $0 c17 1.0"
    exit 1
fi

circuits=($1)
sample_rate=$2

# flist_dir="failLog/flist"
flog_dir="failLog"
diag_dir="Diag_Report"

# For advanced pattern matching in string substitution
shopt -s extglob

mkdir -p ${flog_dir} ${diag_dir}
for circuit in "${circuits[@]}"; do
    mkdir -p ${flog_dir}/${circuit} ${diag_dir}/${circuit}
    diagfail_log="${diag_dir}/${circuit}_diagfail.log"
    # Clean old faillogs if exists
    rm -f ${flog_dir}/${circuit}/*.failLog
    rm -f ${diag_dir}/${circuit}/*.rpt
    rm -f ${diagfail_log}

    # Parse all faults
    readarray -t fault_list_ori < <(awk 'NF {
        if($1 != "i" && $1 != "o" && $1 != "name"){
            gi=3;
            while($gi != ";") {
                print $gi" "$1" GI SA0";
                print $gi" "$1" GI SA1";
                gi=gi+1;
            }
            go=gi+1;
            print $go" "$1" GO SA0";
            print $go" "$1" GO SA1";
        }
    }' sample_circuits/${circuit}.ckt | sort -V)

    # Parse all PIs
    readarray -t pi_list < <(awk 'NF { if($1 == "i") print $2 }' sample_circuits/${circuit}.ckt | sort -V)

    # Remove duplicate faults on fanout-free wire
    # E.g. 19GAT g3 GO and 19GAT g5 GI in c17 circuit

    # First, we count "$WIRE $IO_TYPE" in the original fault list, and only left items with counts=2
    # Then we count "$WIRE", and only left items with counts=2 again
    # The rest are wires with 2 GI and GO SAFs, which are fanout-free wires
    # Be careful that we should `sort` before `uniq`, in this case, the fault list is already sorted
    readarray -t fofree_wires < <(IFS=$'\n'; echo "${fault_list_ori[*]}" |
    awk '{ print $1" "$3 }' | uniq -c |
    awk '{ if($1 == "2") print $2 }' | uniq -c |
    awk '{ if($1 == "2") print $2 }')

    # Build a new fault list without duplicate faults
    declare -a fault_list
    declare -i fofree_idx=0
    fo_segment=false
    for f in "${fault_list_ori[@]}"; do
        fofree=false
        if (( ${fofree_idx} < ${#fofree_wires[@]} )); then
            if [[ "${f}" == "${fofree_wires[${fofree_idx}]} "* ]]; then
                fofree=true
                fo_segment=true
            elif [[ "${f}" == "${fofree_wires[${fofree_idx}+1]} "* ]]; then
                fofree=true
                fofree_idx+=1
                fo_segment=true
            elif [[ "${fo_segment}" == true ]]; then
                fofree_idx+=1
                fo_segment=false
            fi
        fi
        if [[ "${fofree}" == false ]] || [[ "${f}" == *"GO"* ]]; then
            fault_list+=("${f}")
        fi
    done

    # Sample faults if needed
    # To round the number to integer in bc, we add 0.5 to it and divide it by 1 (since division uses scale)
    sample_counts=$(echo "scale=0; (${sample_rate}*${#fault_list[@]}+0.5)/1" | bc)
    if [ "${sample_counts}" != "${#fault_list[@]}" ]; then
        readarray -t fault_list < <(shuf -n ${sample_counts} -e "${fault_list[@]}")
    fi

    #  Iteratively insert every fault in the fault list
    declare -i pi_idx=0
    declare -i empty_num=0
    declare -i fail_num=0
    pi_segment=false
    for fault in "${fault_list[@]}"; do
        # Replace space with underscore for filename
        f_filename="${fault// /_}"
        # Simplified version that without '(XXX)' and '*'
        f_str_s=${fault//\(*\)?(\*)/}

        ptn_file="patterns/${circuit}.ptn"
        ckt_file="sample_circuits/${circuit}.ckt"
        flog_file="${flog_dir}/${circuit}/${circuit}_${f_filename}.failLog"
        drpt_file="${diag_dir}/${circuit}/${circuit}_${f_filename}.failLo.rpt"

        echo "Diagnose SSF ${f_str_s}..."
        src/atpg -genFailLog ${ptn_file} ${ckt_file} -fault ${fault} > ${flog_file}
        if [ -s ${flog_file} ]; then
            src/atpg -diag ${ptn_file} ${ckt_file} ${flog_file} > /dev/null

            if [[ ${pi_idx} < ${#pi_list[@]} ]]; then
                if [[ "${fault}" == "${pi_list[${pi_idx}]} "* ]]; then
                    pi_segment=true
                elif [[ "${fault}" == "${pi_list[${pi_idx}+1]} "* ]]; then
                    pi_idx+=1
                    pi_segment=true
                elif [[ "${pi_segment}" == true ]]; then
                    pi_idx+=1
                    pi_segment=false
                fi
            fi

            ret=$(grep "${f_str_s}" ${drpt_file})
            if [ ! -z "${ret}" ]; then
                echo "    ${ret}"
            elif [[ "${pi_segment}" == true ]]; then
                f_pi_str=$(echo ${f_str_s} | awk '{ print $1" dummy_gate[0-9]+ GO "$4 }')
                ret=$(grep -E "${f_pi_str}" ${drpt_file})
                if [ ! -z "${ret}" ]; then
                    echo "    ${ret}"
                else
                    echo ${f_str_s} >> ${diagfail_log}
                    fail_num+=1
                fi
            else
                echo ${f_str_s} >> ${diagfail_log}
                fail_num+=1
            fi
        else
            echo "    Empty Faillog, skip this fault..."
            empty_num+=1
        fi
    done
    
    success_num=$((${#fault_list[@]}-${empty_num}-${fail_num}))
    echo ""
    echo "[Circuit ${circuit}]========================================="
    echo "    Total tested faults: ${#fault_list[@]}"
    echo "        Diagnose passed: ${success_num}"
    echo "        Diagnose failed: ${fail_num}"
    echo "          Empty faillog: ${empty_num}"
done