import subprocess


def retrace(trace_file):
    retracer_path = '/projects/pr368/patrace_releases/extracted/latest/patrace/12.04_x11_x64/bin/paretrace'
    cmd = [retracer_path, trace_file]

    print(' '.join(cmd))
    print("Retracing {}...".format(trace_file))
    try:
        subprocess.check_output(cmd)
    except subprocess.CalledProcessError, e:
        print("Error while retracing:")
        print(e)
        return False
    return True
