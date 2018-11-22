#!/usr/bin/env python2
import argparse
import json
import os
from patrace import (
    InputFile,
    OutputFile,
    Call,
    CreateInt32Value,
)


class Arg:
    def __init__(self, type, name, value):
        self.type = type
        self.name = name
        self.value = value

    def get(self):
        arg = self.type(self.value)
        if self.name:
            arg.mName = self.name
        return arg


class Function:
    def __init__(self, name, args):
        self.name = name
        self.args = args

    def write(self, output, tid):
        call = Call(self.name)
        call.thread_id = tid

        for arg in self.args[1:]:
            call.args.push_back(arg.get())

        call.return_value = self.args[0].get()

        output.WriteCall(call)


class Remapper:
    def __init__(self):
        self.num_calls_remapped = 0

    def run(self, input, output):
        # Modify header, if we are remaping the default tid
        header = json.loads(input.jsonHeader)
        default_tid = header['defaultTid']
        output.jsonHeader = json.dumps(header)

        print 'Searching for relevant calls...'
        call_lists = {
            'eglMakeCurrent': [],
            'eglCreateContext': [],
            'eglDestroyContext': [],
        }

        context_calls = []
        highest_thread_id = -1
        for call in input.Calls():
            highest_thread_id = max(call.thread_id, highest_thread_id)
            # call_list = call_lists.get(call.name, None)
            if call.name in call_lists.keys():
                context_calls.append({
                    'name': call.name,
                    'tid': call.thread_id,
                    'params': call.GetArgumentsDict().copy(),
                    'retval': call.GetReturnValue(),
                    'number': call.number,
                })
            # if call_list is not None:
            #     call_list.append({
            #         'call_name': call.name,
            #         'tid': call.thread_id,
            #         'params': call.GetArgumentsDict(),
            #         'retval': call.GetReturnValue(),
            #         'number': call.number,
            #     })
        num_threads = highest_thread_id + 1

        print "Renumbering context ids..."
        # Sometimes, contexts can get the same pointer values
        # Hence, the contexts pointers will not be unique. Therefor,
        # we create an unique, sequential id.
        context_sequential_id = 1
        # Maps original context id with sequential context id.
        contexts_idmap = {0: 0}
        for call in context_calls:
            if call['name'] == 'eglCreateContext':
                contexts_idmap[call['retval']] = context_sequential_id
                call['retval'] = context_sequential_id
                context_sequential_id += 1
            elif call['name'] == 'eglDestroyContext':
                old_id = call['params']['ctx']
                seq_id = contexts_idmap[old_id]
                del contexts_idmap[old_id]
                call['params']['ctx'] = seq_id
            elif call['name'] == 'eglMakeCurrent':
                # Change ctx parameter to our new sequential id
                call['params']['ctx'] = contexts_idmap[call['params']['ctx']]

        print "Finding relevant context and surfaces..."
        make_current_args = [
            (call['params']['draw'], call['params']['ctx'])
            for call in context_calls
            if (
                call['name'] == 'eglMakeCurrent'
                # Excluding the following test made things work for GunJack
                # call['tid'] in [default_tid, 0]
            )
        ]

        import pprint
        pprint.pprint(make_current_args)

        surfaces = []
        contexts = []

        for draw, ctx in make_current_args:
            if draw:
                surfaces.append(draw)

            if ctx:
                contexts.append(ctx)

        # Find all relevant shared contexts
        shared_contexts = []

        for context in contexts:
            for context_call in context_calls:
                if context_call['name'] != 'eglCreateContext':
                    continue

                if context_call['retval'] == context:
                    shared_contexts.append(context_call['params']['share_context'])

        for share_context in shared_contexts:
            contexts.append(share_context)

        contexts = set(contexts)
        surfaces = set(surfaces)
        print "Surfaces {}".format(surfaces)
        print "Contexts: {}".format(contexts)

        class Thread:
            def __init__(self):
                self.current_ctx_seq = 0
                self.current_ctx_old = 0
                self.remap = 0

        threads = [Thread() for i in range(num_threads)]
        # Used to indicate if inside a relevant "eglMakeCurrent-block"

        print "Remap calls..."

        contextid_to_use = None

        contexts_idmap = {0: 0}
        context_sequential_id = 1
        active_thread = -1
        for call in input.Calls():
            current_thread = call.thread_id
            thread_switch = False
            if active_thread != current_thread:
                thread_switch = True
                active_thread = current_thread

            if call.name == 'eglCreateContext':
                oldid = call.GetReturnValue()
                contexts_idmap[oldid] = context_sequential_id
                if context_sequential_id in contexts:
                    contextid_to_use = oldid
                    print "We will map all calls of the context:", contextid_to_use
                    self.remap(call, default_tid)
                context_sequential_id += 1
            elif call.name == 'eglDestroyContext':
                ad = call.GetArgumentsDict()
                oldid = ad['ctx']
                # seqid = contexts_idmap[oldid]
                del contexts_idmap[oldid]
            elif (
                call.name.startswith('eglCreateWindowSurface') or
                call.name == 'eglCreatePbufferSurface'
            ):
                if call.GetReturnValue() in surfaces:
                    self.remap(call, default_tid)
            elif call.name == 'eglDestroySurface':
                ad = call.GetArgumentsDict()
                if ad['surface'] in surfaces:
                    self.remap(call, default_tid)
            elif call.name == 'eglMakeCurrent':
                t = threads[call.thread_id]
                ad = call.GetArgumentsDict()
                t.current_dpy = ad['dpy']
                t.current_draw = ad['draw']
                t.current_read = ad['read']
                t.current_ctx_old = ad['ctx']
                t.current_ctx_seq = contexts_idmap[ad['ctx']]

                if t.current_ctx_seq in contexts:
                    # call.SetArgument(3, contextid_to_use)
                    t.remap = True

                if ad['ctx'] == 0:
                    t.remap = False

            if threads[call.thread_id].remap:
                # If a context is already active on the default thread
                # We need to inject an eglMakeCurrent the first time
                if thread_switch and call.name != 'eglMakeCurrent':
                    t = threads[call.thread_id]
                    Function(
                        'eglMakeCurrent', [
                            Arg(CreateInt32Value, '', 1),
                            Arg(CreateInt32Value, 'dpy', t.current_dpy),
                            Arg(CreateInt32Value, 'draw', t.current_draw),
                            Arg(CreateInt32Value, 'read', t.current_read),
                            Arg(CreateInt32Value, 'ctx', t.current_ctx_old),
                        ]
                    ).write(output, default_tid)

                self.remap(call, default_tid)

            output.WriteCall(call)

    def remap(self, call, newtid):
        call.thread_id = newtid
        self.num_calls_remapped += 1


def remap(oldfile, newfile):
    remapper = Remapper()

    if not os.path.exists(oldfile):
        print("File does not exists: {}".format(oldfile))
        return

    with InputFile(oldfile) as input:
        with OutputFile(newfile) as output:
            remapper.run(input, output)

    return remapper.num_calls_remapped


def main():
    parser = argparse.ArgumentParser(description='Automatically remap thread ids in a .pat trace. This should be used if an eglContext is used by more threads than the default thread.')
    parser.add_argument('oldfile', help='Path to the .pat trace file')
    parser.add_argument('newfile', help='New .pat file to create')

    args = parser.parse_args()
    num = remap(args.oldfile, args.newfile)

    print("Number of calls remapped {num}".format(num=num))


if __name__ == '__main__':
    main()
