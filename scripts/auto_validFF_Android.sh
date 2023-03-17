#!/bin/bash


##input parameters
input_json_filename=$1  #input json file for replaying original trace, should at least contain parameters "collectors","file" and "frames", "preload" should be true
target_frame=$2  #target frame of FastForward
input_file_path=$(awk -F"\"" '/file/{print $4}' ${input_json_filename})  #input file name(original trace), including absolute path
result_file_dir="/data/apitrace/"  #absolute path of dir where resulting files are placed on Android device, should be ensured to be accessibale, could be edited



##prepare the input json file for FFed trace
input_json_FFed="input_FF.json"  #input json file for replaying FFed trace
rm ./${input_json_FFed}
cp ${input_json_filename} ./${input_json_FFed}
frames_ori=$(awk -F"\"" '/frames/{print $4}' ${input_json_filename})
start_f=`echo $frames_ori | awk -F"-" '{print $1}'`
end_f=`echo $frames_ori | awk -F"-" '{print $2}'`
FFstart=`expr 1 - $target_frame + $start_f `
FFend=`expr 1 - $target_frame + $end_f `
FFed_filename="FFed_file_${target_frame}_${FFstart}.pat"  ##name of FFed trace, could be edited
sed -i  's/\("frames": "\).*/\1'"${FFstart}-${FFend}"'",/g'  ${input_json_FFed}
sed -i  's!\("file": "\).*!\1'"${result_file_dir}${FFed_filename}"'",!g'  ${input_json_FFed}


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




##push input json files to corresponding dir on Android device
adb shell mkdir -p ${result_file_dir}
adb shell chmod 777 ${result_file_dir}
adb push ${input_json_filename} ${result_file_dir}
adb push ${input_json_FFed} ${result_file_dir}
adb push ${input_gpu_json} ${result_file_dir}
adb push ${input_gpu_json_FFed} ${result_file_dir}

##remove old files
adb shell rm ${result_file_dir}results_ori.json
rm results_ori.json
adb shell rm ${result_file_dir}${FFed_filename}
rm ${FFed_filename}
adb shell rm ${result_file_dir}results_FF.json
rm results_FF.json
adb shell rm ${result_file_dir}results_gpu.json
rm results_gpu.json
adb shell rm ${result_file_dir}results_gpu_FFed.json
rm results_gpu_FFed.json
adb shell rm -rf ${result_file_dir}snap
rm -rf snap
adb shell rm -rf ${result_file_dir}snap_FF
rm -rf snap_FF
adb shell mkdir ${result_file_dir}snap
adb shell mkdir ${result_file_dir}snap_FF

##function to wait till process end, need param name_of_process
wait_process_end(){
	hflag=1
	hresult=1
	while [ "$hflag" -eq 1 ]
	do
        	sleep 1
	        #echo "wait"
        	hresult=`adb shell pidof $1`
        	if [ -z "$hresult" ];then
        	#echo "process $1 finished"
        	hflag=0
        	fi
	done
}



##Android launch paretrace & replay original trace
##capture CPU counters
adb shell chmod 777 ${input_file_path}
adb shell am start -n com.arm.pa.paretrace/.Activities.RetraceActivity --es jsonData ${result_file_dir}${input_json_filename} --es resultFile ${result_file_dir}results_ori.json

wait_process_end com.arm.pa.paretrace

if `adb shell [ -f ${result_file_dir}results_ori.json ]`
then
        adb pull ${result_file_dir}results_ori.json
        echo "replay original trace finished"
else
        echo "failed to replay original trace"
fi


##Android launch paretrace & replay original trace
##snapshot
adb shell am start -n com.arm.pa.paretrace/.Activities.RetraceActivity --es fileName ${input_file_path} --ei frame_start ${start_f} --ei frame_end ${end_f} --es snapshotPrefix ${result_file_dir}snap/frame_ --es snapshotCallset ${start_f}-${end_f}/frame 

wait_process_end com.arm.pa.paretrace


##Android launch paretrace & replay original trace
##capture GPU counters
adb shell am start -n com.arm.pa.paretrace/.Activities.RetraceActivity --es jsonData ${result_file_dir}${input_gpu_json} --es resultFile ${result_file_dir}results_gpu.json

wait_process_end com.arm.pa.paretrace

if `adb shell [ -f ${result_file_dir}results_gpu.json ]`
then
        adb pull ${result_file_dir}results_gpu.json
        echo "GPU counter captured"
else
        echo "failed to capture GPU counter of original trace"
fi



##FF on Android
adb shell am start -n com.arm.pa.paretrace/.Activities.FastforwardActivity --es input ${input_file_path}  --es output ${result_file_dir}${FFed_filename}  --ei targetFrame ${target_frame}

wait_process_end com.arm.pa.paretrace

if `adb shell [ -f ${result_file_dir}${FFed_filename} ]`
then
        adb pull ${result_file_dir}${FFed_filename}
        echo "generate FFed trace finished"
else
        echo "failed to generate FFed trace"
fi



##replay FFed file
##capture CPU counters
adb shell chmod 777 ${result_file_dir}${FFed_filename}
adb shell am start -n com.arm.pa.paretrace/.Activities.RetraceActivity --es jsonData ${result_file_dir}${input_json_FFed} --es resultFile ${result_file_dir}results_FF.json

wait_process_end com.arm.pa.paretrace

if `adb shell [ -f ${result_file_dir}results_FF.json ]`
then
        adb pull ${result_file_dir}results_FF.json
        echo "replay FFed trace finished"
else
        echo "failed to replay FFed trace"
fi


##replay FFed trace
##capture GPU counters
adb shell am start -n com.arm.pa.paretrace/.Activities.RetraceActivity --es jsonData ${result_file_dir}${input_gpu_json_FFed} --es resultFile ${result_file_dir}results_gpu_FFed.json

wait_process_end com.arm.pa.paretrace

if `adb shell [ -f ${result_file_dir}results_gpu_FFed.json ]`
then
        adb pull ${result_file_dir}results_gpu_FFed.json
        echo "GPU counter captured"
else
        echo "failed to capture GPU counter of FFed trace"
fi



##replay FFed file
##snapshot
adb shell am start -n com.arm.pa.paretrace/.Activities.RetraceActivity --es fileName ${result_file_dir}${FFed_filename} --ei frame_start ${FFstart} --ei frame_end ${FFend} --es snapshotPrefix ${result_file_dir}snap_FF/frame_ --es snapshotCallset ${FFstart}-${FFend}/frame

wait_process_end com.arm.pa.paretrace


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
        echo "CPU counters differ more than 5%, differ ratio is ${ratio}"
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
fi


##report image differ
adb pull ${result_file_dir}snap
adb pull ${result_file_dir}snap_FF
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
