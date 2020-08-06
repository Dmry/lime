import gi

gi.require_version('Gtk', '3.0')

from gi.repository import Gtk, Gio

import numpy as np
import matplotlib

from matplotlib.backends.backend_gtk3agg import (
    FigureCanvasGTK3Agg as FigureCanvas)
from matplotlib.backends.backend_gtk3 import (
    NavigationToolbar2GTK3 as NavigationToolbar)
from matplotlib.figure import Figure
import matplotlib.pyplot as plt

class PlotArea(Gtk.ScrolledWindow):
    def __init__(self):
        settings=Gtk.Settings.get_default()

        colors=settings.get_property("gtk-color-scheme")
        colors=colors.split("\n")

        self.text_color = 'white'

        for color in colors:
            if 'text' in color:
                self.text_color=color.split(':')[1].strip()
                break

        matplotlib.rcParams.update({'text.color' : self.text_color,
                     'axes.labelcolor' : self.text_color})

        Gtk.ScrolledWindow.__init__(self, border_width=10)

        self.set_hexpand(True)
        self.set_vexpand(True)

        f = Figure(figsize=(600, 400), dpi=100)
        a = f.add_subplot(111)
        t = np.arange(0.0, 3.0, 0.01)
        s = np.sin(2*np.pi*t)
        a.plot(t, s)
        f.patch.set_visible(False)
        a.set_facecolor('None')
        a.tick_params(labelcolor=self.text_color, color=self.text_color)
        for child in a.get_children():
            if isinstance(child, matplotlib.spines.Spine):
                child.set_color(self.text_color)


        canvas = FigureCanvas(f)

        canvas.toolbar = NavigationToolbar(canvas, self)

        self.add(canvas)

class FileDialog(Gtk.FileChooserDialog):
    def __init__(self):
        Gtk.FileChooserDialog.__init__(self, title="Please choose a file", action=Gtk.FileChooserAction.OPEN)

        self.add_buttons(
            Gtk.STOCK_CANCEL,
            Gtk.ResponseType.CANCEL,
            Gtk.STOCK_OPEN,
            Gtk.ResponseType.OK,
        )

        self.add_filters()

    def add_filters(self):
        filter_text = Gtk.FileFilter()
        filter_text.set_name("Delimited files")
        filter_text.add_mime_type("text/plain")
        self.add_filter(filter_text)

        filter_any = Gtk.FileFilter()
        filter_any.set_name("Any files")
        filter_any.add_pattern("*")
        self.add_filter(filter_any)

    def read_dat(self, file):
        data = np.loadtxt(fname=file.get_path(), comments="#")
        name = file.get_basename()
        datetime = file.query_info(Gio.FILE_ATTRIBUTE_TIME_MODIFIED, 0, None).get_modification_date_time().format(" %H:%M:%S  %d/%m/%Y")
        path = file.get_path()

        return (name, datetime, path)

class Handler:
    def __init__(self, store):
        self.current = store.get_iter_first()

    def onDestroy(self, *args):
        Gtk.main_quit()

    def on_aboutButton_clicked(self, about):
        about.show_all()

    def on_thanksButton_clicked(self, about):
        about.hide()

    def on_openButton_clicked(self, store):
        dialog = FileDialog()
        response = dialog.run()

        if response == Gtk.ResponseType.OK:
            result = dialog.read_dat(dialog.get_file())
            store.append(self.current, [Gtk.CheckButton(), *result])

        dialog.destroy()

builder = Gtk.Builder()
builder.add_from_file("lime.glade")

store = builder.get_object("resultStore")
store.results = {}

builder.connect_signals(Handler(store))

treeview = builder.get_object('storeView')
treeview.model = store

cell = Gtk.CellRendererToggle()
col = Gtk.TreeViewColumn("Show", cell)
treeview.append_column(col)

columns = ["Name",
           "Timestamp",
           "Path"]

for i, column in enumerate(columns):
    # cellrenderer to render the text
    cell = Gtk.CellRendererText()
    # the column is created
    col = Gtk.TreeViewColumn(column, cell, text=i+1)

    # and it is appended to the treeview
    treeview.append_column(col)

graphic_view = builder.get_object("graphicView")
graphic_view.add(PlotArea())

window = builder.get_object("lime")
window.show_all()

Gtk.main()