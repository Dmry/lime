import sys
import random
import qdarkgraystyle
import csv
import os
import subprocess
import lime_python
from PyQt5.QtWidgets import *
from PyQt5 import QtCore

from matplotlib.backends.backend_qt5agg import FigureCanvasQTAgg as FigureCanvas
from matplotlib.backends.backend_qt5agg import NavigationToolbar2QT as NavigationToolbar
from matplotlib.figure import Figure
import matplotlib.pyplot as plt

class Main(QDialog):
    
    def __init__(self):
        super().__init__()

        operationComboBox = QComboBox(self)
        operationComboBox.addItem("Generate")
        operationComboBox.addItem("Fit")
        operationComboBox.activated[str].connect(self.onChanged) 

        operationLabeL = QLabel("&Operation:")
        operationLabeL.setBuddy(operationComboBox)

        topLayout = QHBoxLayout()
        topLayout.addWidget(operationLabeL)
        topLayout.addWidget(operationComboBox)
        topLayout.addStretch(1)

        self.plot = PlotCanvas(self, width=1, height=1)
        self.results = ResultsBox(self.plot)
        self.parameters = ParameterBox(parent=self)
        self.generator = GenerateBox(self.parameters, self.results)

        mainLayout = QGridLayout()
        mainLayout.addLayout(topLayout, 0, 0, 1, 0)
        mainLayout.addWidget(self.parameters, 2, 0)
        mainLayout.addWidget(self.generator, 3, 0)
        mainLayout.addWidget(self.results, 4, 0)

        mainLayout.setRowStretch(4, 1)

        mainLayout.setColumnStretch(0, 0)
        

        self.setLayout(mainLayout)
        self.setGeometry(600, 600, 750, 800)
        self.setWindowTitle("Lime")

    def onChanged(self, text):
        if "Fit" in text:
            print(self.parameters.layout.itemAt(0).widget())

    def createBottomLeftTabWidget(self):
        self.bottomLeftTabWidget = QTabWidget()
        self.bottomLeftTabWidget.setSizePolicy(QSizePolicy.Preferred,
                QSizePolicy.Ignored)

        charttab = QWidget()
        chart = PlotCanvas(self, width=1, height=1)

        charttabhbox = QHBoxLayout()
        charttabhbox.setContentsMargins(5, 5, 5, 5)
        charttabhbox.addWidget(chart)
        charttab.setLayout(charttabhbox)

        datatab = QWidget()
        tableWidget = QTableWidget(10, 10)

        datatabhbox = QHBoxLayout()
        datatabhbox.setContentsMargins(5, 5, 5, 5)
        datatabhbox.addWidget(tableWidget)
        datatab.setLayout(datatabhbox)

        self.bottomLeftTabWidget.addTab(charttab, "Chart")
        self.bottomLeftTabWidget.addTab(datatab, "&Data")

class ArgumentRepresentation(QWidget):
    def __init__(self, name, refvar, value=0):
        QWidget.__init__(self)

        layout = QVBoxLayout(self)
        layout.setSpacing(5)
        layout.setContentsMargins(0, 2, 0, 0)

        self.refvar = refvar

        self.box = QDoubleSpinBox()

        self.label = QLabel(name)
        self.label.setBuddy(self.box)

        self.box.valueChanged.connect(self.onValueChanged)
        self.box.setDecimals(10)
        self.box.setMaximum(100000000000000)
        self.box.setValue(value)

        layout.addWidget(self.label)
        layout.addWidget(self.box)

        self.setLayout(layout)

    def getValue(self):
        return self.refvar

    def getLabel(self):
        return self.label.text()

    def onValueChanged(self, val):
        self.refvar = val

class ParameterBox(QGroupBox):

    def __init__(self, parent=None):
        self.current_row = 0
        self.current_col = 0
        self.col_count = 2

        QGroupBox.__init__(self, "Parameters")
        self.setParent(parent)

        self.layout = QGridLayout()

        self.context = lime_python.Context()
        self.system = lime_python.System()
        self.time_factor = 1
        self.time_max = 10000000
        self.cr = 0.1

        self.context.N = 2048
        self.system.temperature = 1
        self.context.N_e = 60
        self.context.tau_monomer = 1
        self.system.density = 0.68

        lime_python.init_factories()

        self.addParameter("Final time", self.time_max, 20000000)
        self.addParameter("Time spacing factor", self.time_factor, 1.2)
        self.addParameter("Length", self.context.N, 2048)
        self.addParameter("Temperature", self.system.temperature, 1)
        self.addParameter("Monomers per segment", self.context.N_e, 60)
        self.addParameter("Monomer relaxation time", self.context.tau_monomer, 1)
        self.addParameter("Constraint release parameter", self.cr, 1.0)
        self.addParameter("Density", self.system.density, 0.68)

        self.setLayout(self.layout)

    def addParameter(self, name, refvar, defaultVal):
        param = ArgumentRepresentation(name, defaultVal)

        self.layout.addWidget(param, self.current_row, self.current_col)
        self.updateRowColCount()

    def getTime(self):
        return lime_python.generate_exponential(self.time_factor, self.time_max)

    def updateRowColCount(self):
        self.current_col += 1

        if self.current_col == self.col_count:
            self.current_col = 0
            self.current_row += 1

class GenerateBox(QWidget):

    def __init__(self, parameters, results):
        super(GenerateBox, self).__init__()

        self.results = results
        self.parameters = parameters
    
        layout = QVBoxLayout()

        self.generateButton = QPushButton("Generate")
        self.generateButton.clicked.connect(self.generate)
        layout.addWidget(self.generateButton)
    
        self.setLayout(layout)

        self.builder = lime_python.Context_builder(self.parameters.system, self.parameters.context)
        self.impl = lime_python.cr_impl.Heuzey

    def generate(self):
        self.time = self.parameters.getTime()
        self.result = lime_python.ICS_result(self.time, self.builder, self.impl)
        self.result.calculate()

class DataSet(QTableWidgetItem):
    def __init__(self, label, path, volatile=False, parent=None):
        super(QTableWidgetItem,self).__init__(path)

        self.label = label
        self.path = path
        self.volatile = volatile
        self.x = []
        self.y = []

class FileContent(DataSet):
    def __init__(self, filePath, volatile=False, parent=None):
        label = os.path.splitext(os.path.basename(filePath))[0]
        super(FileContent,self).__init__(label, filePath, volatile, parent)

    def read(self):
        f = open(self.path, 'r')
        csv_reader = csv.reader(filter(lambda row: row[0]!='#', f), delimiter=' ')

        for row in csv_reader:
            self.x.append(float(row[0]))
            self.y.append(float(row[1]))

class ResultsBox(QTabWidget):
    def __init__(self, chart):
        QTabWidget.__init__(self)
        self.setSizePolicy(QSizePolicy.Preferred, QSizePolicy.Ignored)

        self.dataSets = []
        self.lastResult = 0

        self.chart = chart
        self.chart.dataSets = self.dataSets
        self.files = FilesTable(3, 3, self)
        
        fileTools = ToolbarBox(self)

        filestabhbox = QHBoxLayout()
        filestabhbox.setContentsMargins(5, 5, 5, 5)
        filestabhbox.addWidget(self.files)
        filestabhbox.addWidget(fileTools)

        filestab = QWidget()
        filestab.setLayout(filestabhbox)

        charttabhbox = QHBoxLayout()
        charttabhbox.setContentsMargins(5, 5, 5, 5)
        charttabhbox.addWidget(self.chart)
        charttabhbox.addWidget(self.chart.toolbar)

        charttab = QWidget()
        charttab.setLayout(charttabhbox)
        
        self.addTab(charttab, "Chart")
        self.fileTabIndex = self.addTab(filestab, "Files")

    def addToFileList(self, filenames):
        rowPosition = self.files.rowCount()
        self.files.insertRow(rowPosition)

        for file in filenames:
            content = self.loadFileData(file)
            self.files.setItem(rowPosition, 0, content)
            

    def loadFileData(self, filename, volatile=False):
        content = FileContent(filename, volatile)
        content.read()
        self.dataSets.append(content)
        self.chart.plot()
        return content

    def removeSelected(self):
        for item in self.files.selectedItems():
            self.dataSets.remove(item)
            self.files.removeRow(item.row())
            self.chart.plot()

    def removeVolatile(self):
        for file in self.dataSets:
            if file.volatile:
                self.dataSets.remove(file)

class FilesTable(QTableWidget):
    def __init__(self, rows, columns, parent=None):
        QGroupBox.__init__(self, 0, 1, parent)
        self.setParent(parent)
        self.setHorizontalHeaderLabels(["File location",])
        header = self.horizontalHeader()   
        header.setSectionResizeMode(0, QHeaderView.Stretch)

class ToolbarBox(QGroupBox):

    def __init__(self, parent=None):
        QGroupBox.__init__(self, "External files")
    
        layout = QHBoxLayout()

        self.parent = parent

        self.toolBar = QToolBar()
        self.toolBar.setOrientation(QtCore.Qt.Vertical)

        addFilesButton = QToolButton()
        addFilesButton.setText("Add")
        addFilesButton.clicked.connect(self.getFiles)
        self.toolBar.addWidget(addFilesButton)

        remmoveFilesButton = QToolButton()
        remmoveFilesButton.setText("Remove")
        remmoveFilesButton.clicked.connect(self.removeSelected)
        self.toolBar.addWidget(remmoveFilesButton)

        layout.addWidget(self.toolBar)
    
        self.setLayout(layout)
		
    def getFiles(self):
        dlg = QFileDialog()
        dlg.setFileMode(QFileDialog.AnyFile)
    
        if dlg.exec_():
            filenames = dlg.selectedFiles()
            self.parent.addToFileList(filenames)

    def removeSelected(self):
        self.parent.removeSelected()

class PlotCanvas(FigureCanvas):

    def __init__(self, parent=None, width=1, height=1, dpi=100):
        plt.style.use('dark_background')
        fig = Figure(figsize=(width, height), dpi=dpi)
        fig.patch.set_facecolor('xkcd:charcoal grey')
        self.axes = fig.add_subplot(111)
        self.axes.set_facecolor('xkcd:charcoal grey')

        FigureCanvas.__init__(self, fig)
        self.setParent(parent)

        self.toolbar = NavigationToolbar(self, self)
        self.toolbar.setOrientation(QtCore.Qt.Vertical)
        self.toolbar.setMaximumWidth(35)

        FigureCanvas.setSizePolicy(self, QSizePolicy.Expanding, QSizePolicy.Expanding)
        FigureCanvas.updateGeometry(self)

        self.dataSets = []

    def plot(self):
        self.axes.clear()

        for data in self.dataSets:
            self.axes.loglog(data.x, data.y, '+', label=data.label)
            self.axes.legend()
        
        self.draw()
        
if __name__ == '__main__':
    app = QApplication(sys.argv)
    app.setStyleSheet(qdarkgraystyle.load_stylesheet())
    ics = Main()
    ics.show()
    sys.exit(app.exec_())
