import json, os, sys, ctypes, cPickle
from PyQt4.QtGui import *
from PyQt4.QtCore import *

class HeaderEditor(QWidget):

    def __init__(self, parent = None):

        super(HeaderEditor, self).__init__(parent)

        headerViewLayout = QGridLayout(self)
        headerViewLayout.addWidget(QLabel('Header Version :', self), 0, 0)
        self.headerVersionEdit = QLineEdit(self)
        self.headerVersionEdit.setReadOnly(True)
        self.headerVersionEdit.setAlignment(Qt.AlignHCenter)
        headerViewLayout.addWidget(self.headerVersionEdit, 0, 1)
        headerViewLayout.addWidget(QLabel('JSON String :', self), 1, 0)
        self.jsonHeaderEdit = QPlainTextEdit(self)
        headerViewLayout.addWidget(self.jsonHeaderEdit, 2, 0, 1, 2)

    @property
    def header_version(self):
        pass
    @header_version.setter
    def header_version(self, version):
        self.headerVersionEdit.setText('v%d' % version)

    @property
    def json_string(self):
        return str(self.jsonHeaderEdit.toPlainText())
    @json_string.setter
    def json_string(self, str):
        self.jsonHeaderEdit.setPlainText(str)

class MainWindow(QMainWindow):

    def __init__(self, trace_path):

        QMainWindow.__init__(self)
        self.trace_path = trace_path
        self.setWindowTitle(trace_path)

        self.create_menus()
        self.create_tabs()

        from patrce import InputFile
        self.trace = InputFile()
        self.trace.Open(self.trace_path, True)
        self.header_version = self.trace.version
        self.json_header = self.trace.jsonHeader
        self.trace.Close()
        self.populateUI()

    def closeEvent(self, event):
        super(MainWindow, self).closeEvent(event)

    def saveAs(self):
        from patrace import InputFile, OutputFile
        filename = str(QFileDialog.getSaveFileName(parent=self, caption="Save As Trace", directory='', filter='*.pat'))
        if filename:

            if not filename.endswith('.pat'):
                filename += '.pat'

            input_trace = InputFile()
            input_trace.Open(self.trace_path)

            output_trace = OutputFile()
            output_trace.Open(filename)
            output_trace.jsonHeader = json.dumps(json.loads(self.headerEditor.json_string))

            for call in input_trace.Calls():
                output_trace.WriteCall(call)

            input_trace.Close()
            output_trace.Close()

    def create_menus(self):

        fileMenu = self.menuBar().addMenu(self.tr('&File'))

        saveAsAction = QAction(self.tr('&Save As'), self)
        saveAsAction.setShortcuts(QKeySequence.SaveAs)
        saveAsAction.triggered.connect(self.saveAs)
        fileMenu.addAction(saveAsAction)

    def create_tabs(self):

        # Set a tab widget as the central widget
        tabWidget = QTabWidget()
        self.setCentralWidget(tabWidget)

        # First tab for header editing
        headerView = QWidget()
        headerViewLayout = QGridLayout(headerView)

        self.headerEditor = HeaderEditor(self)
        tabWidget.addTab(self.headerEditor, 'Header')

    def populateUI(self):
        self.headerEditor.header_version = self.header_version

        if self.json_header:
            self.headerEditor.json_string = json.dumps(json.loads(self.json_header), indent=4, sort_keys = True)
