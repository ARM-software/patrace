echo "Generating source files"
cd ${PROJECT_DIR}/../../../../patrace/src/common && python api_info.py
cd ${PROJECT_DIR}/../../../../patrace/src/common && python call_parser.py
cd ${PROJECT_DIR}/../../../../patrace/src/dispatch && python eglproc.py
cd ${PROJECT_DIR}/../../../../patrace/src/retracer && python retrace.py
cd ${PROJECT_DIR}/../../../../patrace/src/specs/ && python glxml_header.py
