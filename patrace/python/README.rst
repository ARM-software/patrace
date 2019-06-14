====================
PATrace Python Tools
====================

PATrace provides python tools that can be used to modify trace files.

Install
=======

First, make sure that you have built patrace according to the manual. A successful build produces two wheel packages:

::

    # Install pypatrace
    pip install ./pypatrace/dist/pypatrace-*.whl
    # Install patracetools
    pip install ./patracetools/dist/patracetools-*.whl

Usage
=====

You will have a set of tools in your path that start with ``pat-``.


Description
===========

+-----------------------------+------------------------------------------------------------+
| Command                     | Description                                                |
+=============================+============================================+++=============+
| pat-dump-header             | Dumps the json data from a .pat trace header to stdout     |
+-----------------------------+------------------------------------------------------------+
| pat-edit-header             | Edit the header of a trace, in-place.                      |
+-----------------------------+------------------------------------------------------------+
| pat-minimize-texture-usage  | This script converts an input trace into another trace     |
|                             | ('output.pat') which minimizes the texture bandwidth.      |
|                             |                                                            |
|                             | It creates a 1x1 2D texture (with name 1000) and           |
|                             | another 1x1 cubemap texture (with name 1001);              |
|                             | So it assumes that the count of textures created by        |
|                             | the original trace is less than 1000;                      |
|                             | Then for each drawcall, it forces every texture unit       |
|                             | (up to 8) to just use the trivial textures.                |
+-----------------------------+------------------------------------------------------------+
| pat-remap-tid               | Remap thread ids in a trace                                |
+-----------------------------+------------------------------------------------------------+
| pat-upgrade-version         | Upgrade binary format of trace to newest version           |
+-----------------------------+------------------------------------------------------------+
| pat-remove-frames           | Remove eglSwapbuffers calls and all draw calls targeting   |
|                             | FBO0                                                       |
+-----------------------------+------------------------------------------------------------+
| pat-resize-fbos             | This script converts an input trace into another trace     |
|                             | ('output.pat') in which all FBOs are used with size 16x16. |
|                             | It modifies all glViewport calls to change the resolution  |
|                             | to 16x16. This means it also sets the onscreen FBO (FBO0)  |
|                             | resolution to 16x16.                                       |
+-----------------------------+------------------------------------------------------------+
| pat-set-header-size         | Adjusts the size of the header of a trace.                 |
+-----------------------------+------------------------------------------------------------+
| pat-trim                    | Trim a trace. Note, you will not be able to playback the   |
|                             | trace if the range does not include resources (texture,    |
|                             | shader, vbo) needed for selected range.                    |
+-----------------------------+------------------------------------------------------------+
| pat-update-md5              | Update the md5 sum in the meta file of a .pat trace.       |
+-----------------------------+------------------------------------------------------------+
| pat-version                 | Prints the file version of a .pat file.                    |
+-----------------------------+------------------------------------------------------------+
