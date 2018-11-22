#!/usr/bin/env python
from __future__ import print_function
import argparse
import json
import sys
import os


def main():
    qualcomm, mali = parse_data()
    #interleaved_data = interleaved_format(qualcomm, mali)
    #needs_attribute_handling = find_traces_that_need_attribute_handling(interleaved_data)
    #directories_to_copy(needs_attribute_handling)
    #list_files(needs_attribute_handling)
    #modify_meta(mali, needs_attribute_handling)
    header_format(mali)
    #header_format(mali, needs_attribute_handling)


def mkdirpf(directory):
    try:
        os.makedirs(directory)
    except os.error, e:
        if e.errno != os.errno.EEXIST:
            raise e


def parse_data():
    parser = argparse.ArgumentParser(
        description='Convert attributes information from result file to trace header',
    )
    parser.add_argument('resultfile',
                        help='the path to the result.json file')

    args = parser.parse_args()

    with open(args.resultfile) as f:
        data = json.load(f)

    targets = []

    for target in data["targets"]["com.arm.pa.paretrace"]:
        if "result" not in target:
            print(
                "Result is missing for target: {t}".format(
                    t=target['target']['com.arm.pa.paretrace']['files'][0]['path']
                )
            )
            continue

        targets.append(target)

    results = [
        {
            'status': target['status'],
            'tracefile': target['target']['com.arm.pa.paretrace']['files'][0]['path'],
            #'metafile': target['target']['com.arm.pa.paretrace']['metafile'],
            'target': target['target']['com.arm.pa.paretrace'],
            'result': target['result']['com.arm.pa.paretrace'],
            'device': target['device']
        }
        for target in targets
    ]

    outdata_qualcomm = []
    outdata_mali = []

    for r in results:
        if 'result' not in r['result']:
            print("No results for {f} on device {d}".format(f=r['tracefile'], d=r['device']), file=sys.stderr)
            r['result']['result'] = [{}]

        result = r['result']['result'][0]

        if 'programInfos' not in result:
            print("No programInfos for {f} on device {d}".format(f=r['tracefile'], d=['device']), file=sys.stderr)
            result['programInfos'] = []

        r["programInfo"] = result['programInfos']

        if r['device'] == "N9005-S800-1":
            outdata_qualcomm.append(r)
        elif r['device'] == "N900-E5420-1":
            outdata_mali.append(r)

    return outdata_qualcomm, outdata_mali


def modfiy_meta(data, needs_attribute_handling):
    trace_repo = '.'

    tracefiles = set([
        r['target']['files'][0]['path']
        for r in data
    ])

    for tracefile in tracefiles:
        metafile = os.path.splitext(tracefile)[0] + '.meta'
        metafile = trace_repo + metafile

        if os.path.exists(metafile):
            with open(metafile) as f:
                jsondata = json.load(f)

            jsondata["needs_vertex_attribute_handling"] = tracefile in needs_attribute_handling

            with open(metafile, 'w') as f:
                print("Writing {file}".format(file=metafile))
                jsondata = json.dump(jsondata, f, sort_keys=True, indent=2)


def list_files(files):
    for f in files:
        print(f)


def directories_to_copy(tracefiles):
    files_to_copy = []
    for tracefile in tracefiles:
        additional_files = [
            os.path.splitext(tracefile)[0] + '.meta',
            os.path.join(os.path.dirname(tracefile), 'default.meta'),
            os.path.join(os.path.dirname(tracefile), os.pardir, 'default.meta'),
            os.path.join(os.path.dirname(tracefile), os.pardir, os.pardir, 'default.meta'),
            os.path.join(os.path.dirname(tracefile), os.pardir, os.pardir, os.pardir, 'default.meta'),
        ]

        files_to_copy.append(tracefile)
        for f in additional_files:
            files_to_copy.append(f)

    for f in files_to_copy:
        if os.path.exists(f):
            print("Error: {f} does not exists".format(f=f))


def header_format(data, include_list=None):
    header_info = {}

    for r in data:
        if r['status'] != "COMPLETE":
            print("IGNORING", r['tracefile'])
            continue

        threadId = r['target']['threadId']
        attrs = [{}] * (threadId + 1)
        attr = [
            [attr for attr in p['activeAttributeNames']]
            for p in r['programInfo']
        ]

        attrs[threadId] = {"attributes": attr}

        tracefile = r['target']['files'][0]['path']
        header_info[tracefile] = {"threads": attrs}

        # Print sizes
        #print(str(len(json.dumps(attrs, separators=(',', ':')))) + ' ' + tracefile)

    patchdir = './patches'

    for key, value in header_info.iteritems():
        if include_list and key not in include_list:
            continue

        filename = os.path.join(patchdir, '.' + key + ".patch.json")
        directory = os.path.dirname(filename)
        mkdirpf(directory)

        print(filename)
        with open(filename, 'w') as f:
            f.write(
                json.dumps(value,
                           separators=(',', ':'))
            )


def find_traces_that_need_attribute_handling(interleaved_data):
    result = set()
    for key, value in interleaved_data.iteritems():
        for program in value:
            if program['attributes']['only_on_qualcomm']:
                result.add(key)
                #print(program['metafile'])
                break
    return result


def interleaved_format(outdata_qualcomm, outdata_mali):
    output_data = {}

    for i in range(len(outdata_qualcomm)):
        qualcomm_status = outdata_qualcomm[i]['status']
        mali_status = outdata_mali[i]['status']

        if qualcomm_status != "COMPLETE":
            print("{file}: Test failed for Qualcom".format(file=outdata_qualcomm[i]['tracefile']), file=sys.stderr)
            continue

        if mali_status != "COMPLETE":
            print("{file}: Test failed for Mali".format(file=outdata_qualcomm[i]['tracefile']), file=sys.stderr)
            continue

        qualcomm_pi = outdata_qualcomm[i]['programInfo']
        mali_pi = outdata_mali[i]['programInfo']
        for j in range(len(qualcomm_pi)):
            qualcomm_count = qualcomm_pi[j]['activeAttributeCount']
            qualcomm_link_status = qualcomm_pi[j]['linkStatus']
            mali_count = mali_pi[j]['activeAttributeCount']
            mali_link_status = mali_pi[j]['linkStatus']

            #if qualcomm_link_status != mali_link_status:
            #    print "{file}: {programid}. Mali {m} and Adreno: {a}: Link statuses differ".format(
            #        file=outdata_qualcomm[i]['file'],
            #        programid=outdata_qualcomm[i]['programInfo'][j]['id'],
            #        m=mali_link_status,
            #        a=qualcomm_link_status
            #    )

            #if qualcomm_count != mali_count:
            names_q = set(qualcomm_pi[j]['activeAttributeNames'])
            names_m = set(mali_pi[j]['activeAttributeNames'])

            file_ = outdata_qualcomm[i]['target']['files'][0]['path']
            if file_ not in output_data:
                output_data[file_] = []

            output_data[file_].append({
                'programid': outdata_qualcomm[i]['programInfo'][j]['id'],
                'metafile': outdata_qualcomm[i]['metafile'],
                'attributes': {
                    'count': {
                        'qualcomm': qualcomm_count,
                        'mali': mali_count,
                    },
                    'only_on_qualcomm': list(names_q - names_m),
                },
                'link': {
                    'status': {
                        'mali': mali_link_status,
                        'qualcomm': qualcomm_link_status,
                    }
                }

            })

    return output_data
                #print "{file}: {programid_q} - {programid_m}: Number of Active attributes differs ({q} - {m}): {names}".format(
                #    file=outdata_qualcomm[i]['file'],
                #    programid_q=outdata_qualcomm[i]['programInfo'][j]['id'],
                #    programid_m=outdata_mali[i]['programInfo'][j]['id'],
                #    q=qualcomm_count,
                #    m=mali_count,
                #    names=names_q - names_m
                #)

    #with open("output_qualcomm.json", "w") as f:
    #    json.dump(outdata_qualcomm, f, sort_keys=True, indent=2)
    #with open("output_mali.json", "w") as f:
    #    json.dump(outdata_mali, f, sort_keys=True, indent=2)

if __name__ == "__main__":
    main()
    #sys.exit(main())
