#!/bin/bash


##input parameters
input_json_filename=$1  #input json file for replaying original trace and capture CPU counters, should at least contain parameters "collectors","file" and "frames", "preload" should be true
target_frame=$2  #target frame of FastForward
input_file_path=$(awk -F"\"" '/file/{print $4}' ${input_json_filename})  #input file name(original trace), including path


##prepare the input json file for FFed trace
input_json_FFed="input_FF.json"  #input json file for replaying FFed trace
rm ./${input_json_FFed}
cp ${input_json_filename} ./${input_json_FFed}
frames_ori=$(awk -F"\"" '/frames/{print $4}' ${input_json_filename})
start_f=`echo $frames_ori | awk -F"-" '{print $1}'`
end_f=`echo $frames_ori | awk -F"-" '{print $2}'`
FFstart=`expr 1 - $target_frame + $start_f `
FFend=`expr 1 - $target_frame + $end_f `
FFed_filename="FFed_file_${target_frame}_${FFstart}.pat"
sed -i  's/\("frames": "\).*/\1'"${FFstart}-${FFend}"'",/g'  ${input_json_FFed}
sed -i  's/\("file": "\).*/\1'"${FFed_filename}"'",/g'  ${input_json_FFed}


##prepare for the input json file for capturing GPU counters
#original
input_gpu_json="input_gpu.json"
rm ./${input_gpu_json}
touch ./${input_gpu_json}
cat ${input_json_filename} | python3 -c "import sys, json; S=json.load(sys.stdin); del S['collectors']; S['perfmon']=True; json.dump(S,open('${input_gpu_json}', 'w'),indent=2)"
#FFed
input_gpu_json_FFed="input_gpu_FFed.json"
rm ./${input_gpu_json_FFed}
touch ./${input_gpu_json_FFed}
cat ${input_json_FFed} | python3 -c "import sys, json; S=json.load(sys.stdin); del S['collectors']; S['perfmon']=True; json.dump(S,open('${input_gpu_json_FFed}', 'w'),indent=2)"



##remove old files
rm results_ori.json
rm ${FFed_filename}
rm results_FF.json
rm results_gpu.json
rm results_gpu_FFed.json
rm -rf snap
rm -rf snap_FF
mkdir snap
mkdir snap_FF



##function to wait till process end, need param name_of_process
wait_process_end(){
        hflag=1
        hresult=1
        while [ "$hflag" -eq 1 ]
        do
                sleep 1
                #echo "wait"
                hresult=`pidof $1`
                if [ -z "$hresult" ];then
                #echo "process $1 finished"
                hflag=0
                fi
        done
}




##Linux replay original trace and capture CPU counters
./paretrace -jsonParameters ${input_json_filename} ./results_ori.json .

wait_process_end paretrace

if [ -f ./results_ori.json ]
then
        echo "replay original trace finished"
else
        echo "failed to replay original trace"
fi


##Linux replay original trace and snapshot
./paretrace  -framerange ${start_f} ${end_f} -snapshotprefix ./snap/frame_ -s ${start_f}-${end_f}/frame ${input_file_path}

wait_process_end paretrace



##Linux replay original trace and capture GPU counters
./paretrace -jsonParameters ${input_gpu_json} ./results_gpu.json .

wait_process_end paretrace

if [ -f ./results_gpu.json ]
then
        echo "GPU counter captured"
else
        echo "failed to caputure GPU counter of original trace"
fi


##FF on Linux
./fastforward  --input ${input_file_path} --output ./${FFed_filename} --targetFrame ${target_frame}

wait_process_end fastforward

if [ -f ./${FFed_filename} ]
then
        echo "generate FFed trace finished"
else
        echo "failed to generate FFed trace"
fi


##replay FFed file and capture CPU counters
chmod 777 ./${FFed_filename}
./paretrace -jsonParameters ${input_json_FFed} ./results_FF.json .

wait_process_end paretrace

if [ -f ./results_FF.json ]
then
        echo "replay FFed trace finished"
else
        echo "failed to replay FFed trace"
fi



##Linux replay FFed trace and capture GPU counters
./paretrace -jsonParameters ${input_gpu_json_FFed} ./results_gpu_FFed.json .

wait_process_end paretrace

if [ -f ./results_gpu_FFed.json ]
then
        echo "GPU counter captured"
else
        echo "failed to caputure GPU counter of FFed trace"
fi


##Linux replay FFed trace and snapshot
./paretrace -framerange ${FFstart} ${FFend} -snapshotprefix ./snap_FF/frame_ -s ${FFstart}-${FFend}/frame ${FFed_filename}

wait_process_end paretrace




##report if CPU counters differ more that 5%
sum_ori=`cat results_ori.json | python3 -c "import sys, json; S=json.load(sys.stdin)['result'][0]['frame_data']['perf']['thread_data'][2]['SUM']; a=[S[key] for key in S]; print(a)"`
sum_FF=`cat results_FF.json | python3 -c "import sys, json; S=json.load(sys.stdin)['result'][0]['frame_data']['perf']['thread_data'][2]['SUM']; a=[S[key] for key in S]; print(a)"`
data_ori=(`echo $sum_ori | sed 's/,//g' | sed 's/\[//g'| sed 's/\]//g'`)
data_FF=(`echo $sum_FF | sed 's/,//g' | sed 's/\[//g'| sed 's/\]//g'`)
#echo number of CPU counters is ${#data_FF[@]}
for i in $(seq 1 ${#data_ori[@]})
do
        tem_dif=`expr ${data_ori[\`expr $i - 1 \`]} - ${data_FF[\`expr $i - 1 \`]} `
        tem_dif=${tem_dif#-}  #abs(tem_dif)
        #echo differ in CPU counter "`expr $i - 1 `" is ${tem_dif}
        divisor_num=${data_ori[` expr $i - 1 `]}
        #echo CPU counter in original trace is ${divisor_num}
        ratio=`awk 'BEGIN{printf "%.2f\n",'${tem_dif}'/'${divisor_num}'}'`
        if [ `echo "$ratio > 0.05" | bc` -eq 1 ];then
        echo "CPU counters differ more than 5%, ratio is ${ratio}"
        ###break
        fi
done


##report if GPU counters differ more that 5%
gpu_ori=`cat results_gpu.json | python3 -c "import sys, json; S=json.load(sys.stdin)['result'][0]['gpu_active']; print(S)"`
gpu_FF=`cat results_gpu_FFed.json | python3 -c "import sys, json; S=json.load(sys.stdin)['result'][0]['gpu_active']; print(S)"`
tem_dif=` expr ${gpu_ori} - ${gpu_FF} `
tem_dif=${tem_dif#-}  #abs(tem_dif)
#echo differ in GPU counter is ${tem_dif}
divisor_num=${gpu_ori}
#echo GPU counter in original trace is ${divisor_num}
ratio=`awk 'BEGIN{printf "%.2f\n",'${tem_dif}'/'${divisor_num}'}'`
if [ `echo "$ratio > 0.05" | bc` -eq 1 ];then
echo "GPU counters differ more than 5%, ratio is ${ratio}"
###break
fi


##report image differ
##list1
ima_index=0
for ima_file in ./snap/*
do
    if test -f $ima_file
    then
        #echo $ima_file is file in snap folder
        ima_list1[${ima_index}]=$ima_file
        ima_index=`expr ${ima_index} + 1 `
    fi
done
##list2
ima_index=0
for ima_file in ./snap_FF/*
do
    if test -f $ima_file
    then
        #echo $ima_file is file in snap_FF folder
        diff $ima_file ${ima_list1[${ima_index}]}
        ima_index=`expr ${ima_index} + 1 `
    fi
done
echo "image comparison finished"
