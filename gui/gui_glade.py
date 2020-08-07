import gi

gi.require_version('Gtk', '3.0')

from gi.repository import Gtk, Gio, GObject

import numpy as np
import matplotlib

from matplotlib.backends.backend_gtk3agg import (
    FigureCanvasGTK3Agg as FigureCanvas)
from matplotlib.backends.backend_gtk3 import (
    NavigationToolbar2GTK3 as NavigationToolbar)
from matplotlib.figure import Figure

import datetime

import lime_python

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

        Gtk.ScrolledWindow.__init__(self, border_width=10)

        self.set_hexpand(True)
        self.set_vexpand(True)

        self.f = Figure(figsize=(600, 400), dpi=100)
        self.f.patch.set_visible(False)
        self.a = self.f.add_subplot(111, xscale="log", yscale="log")
        self.a.set_facecolor('None')
        self.a.tick_params(labelcolor=self.text_color, color=self.text_color)
        for child in self.a.get_children():
            if isinstance(child, matplotlib.spines.Spine):
                child.set_color(self.text_color)
        self.canvas = FigureCanvas(self.f)
        self.canvas.toolbar = NavigationToolbar(self.canvas, self)
        self.add(self.canvas)

    def plot(self, store, iterator):
        self.a.loglog(store[iterator][4].time, store[iterator][4].data)
        self.f.canvas.draw()


class TimeSeries(GObject.Object):
    def __init__(self, time, data):
        GObject.Object.__init__(self)
        self.time = time
        self.data = data

class Result(GObject.Object):
    def __init__(self, time, data, name, datetime=datetime.datetime.now(), path=""):
        GObject.Object.__init__(self)
        self.time_series = TimeSeries(time, data)
        self.name = name
        self.datetime = datetime
        self.path = path
        self.checkbox = Gtk.CheckButton(active=True)

    def add_to_store(self, store, iterator):
        self.iter = store.append(iterator, [self.checkbox, self.name, self.datetime, self.path, self.time_series])
        return self.iter

    def get_iter(self):
        return self.iter


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
        datetime = file.query_info(Gio.FILE_ATTRIBUTE_TIME_MODIFIED, 0, None).get_modification_date_time().format("%H:%M:%S  %d/%m/%Y")
        path = file.get_path()

        # shape[1] == number of columns
        if data.shape[1] < 2:
            raise ValueError("That data file dit not contain 2 or more columns.")

        time = data[:,0]

        iterdatacols = iter(data.T)
        next(iterdatacols)

        return tuple(Result(time, col, name, datetime, path) for col in iterdatacols)

def generate_variable_store_dict(builder):
    return {
        "n" : builder.get_object("nStore"),
        "t" : builder.get_object("tStore"),
        "ne" : builder.get_object("neStore"),
        "taumonomer" : builder.get_object("taumonStore") ,
        "cv" : builder.get_object("cvStore"),
        "rho" : builder.get_object("rhoStore")
    }

class Handler:
    def __init__(self, store, plot_area, var_store_dict):
        self.store = store # Maybe unused
        self.current = store.get_iter_first()
        self.plot_area = plot_area
        self.var_store_dict = var_store_dict

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
            for entry in result:
                entry.add_to_store(store, self.current)

        dialog.destroy()

    def on_resultStore_row_changed(self, store, row_id, tree_iter):
        self.plot_area.plot(store, tree_iter)

    def var_store_to_number(self, var):
        return float(self.var_store_dict[var].get_text())

    def on_computeButton_clicked(self, tabs):
        self.context = lime_python.Context()
        self.system = lime_python.System()
        self.time_factor = 1.2
        self.time_max = 10000000
        self.cv = 0.1

        self.context.N = self.var_store_to_number("n")
        self.system.temperature = self.var_store_to_number("t")
        self.context.N_e = self.var_store_to_number("ne")
        self.context.tau_monomer = self.var_store_to_number("taumonomer")
        self.system.density = self.var_store_to_number("rho")

        lime_python.init_factories()

        self.time = lime_python.generate_exponential(self.time_factor, self.time_max)
        self.builder = lime_python.Context_builder(self.system, self.context)
        self.impl = lime_python.cr_impl.Heuzey

        self.result = lime_python.ICS_result(self.time, self.builder, self.impl)
        self.result.calculate()
        result = Result(self.time, self.result.get_values(), "generated_result", datetime.datetime.now().strftime("%H:%M:%S  %d/%m/%Y"), "")

        # Triggers row changed event (and thus plot)
        position = result.add_to_store(self.store, self.current)



builder = Gtk.Builder()
builder.add_from_file("lime.glade")

store = builder.get_object("resultStore")

graphic_view = builder.get_object("graphicView")
plot_area = PlotArea()
graphic_view.add(plot_area)

builder.connect_signals(Handler(store, plot_area, generate_variable_store_dict(builder)))

treeview = builder.get_object('storeView')

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

window = builder.get_object("lime")
window.show_all()

Gtk.main()