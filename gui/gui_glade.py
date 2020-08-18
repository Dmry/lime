#!/usr/bin/env python3

import gi

gi.require_version('Gtk', '3.0')

from gi.repository import Gtk, Gdk, GLib, Gio, GObject

import numpy as np

import matplotlib
from matplotlib.backends.backend_gtk3agg import (
    FigureCanvasGTK3Agg as FigureCanvas)
from matplotlib.backends.backend_gtk3 import (
    NavigationToolbar2GTK3 as NavigationToolbar)
from matplotlib.figure import Figure

import datetime
import concurrent.futures
import time
import csv
import os

from decimal import Decimal

import lime_python

class PlotArea(Gtk.ScrolledWindow):
    '''
    Displays matplotlib plots when the graphic view is selected.
    '''

    def __init__(self, text_color):
        # Init prarent and set up container
        Gtk.ScrolledWindow.__init__(self, border_width=10)
        
        self.text_color = text_color

        self.set_hexpand(True)
        self.set_vexpand(True)

        box = Gtk.Box(orientation=Gtk.Orientation.VERTICAL)

        but = Gtk.Box()

        # Convert string to tuple
        self.text_color = tuple(map(float, self.text_color.strip('rgba()').split(','))) 

        # Normalize between 0 and 1 for matplotlib
        self.text_color = tuple(x / 255 for x in self.text_color)

        self.f = Figure(figsize=(600, 400), dpi=100)

        # Set figure to be transparent
        self.f.patch.set_visible(False)

        self.a = self.f.add_subplot(111, xscale="log", yscale="log", xlabel="t", ylabel="G(t)")

        self.a.xaxis.label.set_color(self.text_color)
        self.a.yaxis.label.set_color(self.text_color)

        # Set the background to be transparent
        self.a.set_facecolor('None')

        # Set the colors of the axes to the font colors of the theme
        self.a.tick_params(labelcolor=self.text_color, color=self.text_color, which='both')

        for child in self.a.axes.get_children():
            if isinstance(child, matplotlib.spines.Spine):
                child.set_color(self.text_color)

        # Finalize canvas
        self.canvas = FigureCanvas(self.f)

        # Set up toolbar
        toolbar = NavigationToolbar(self.canvas, self)
        toolbar.set_orientation(Gtk.Orientation.HORIZONTAL)
        
        box.pack_start(toolbar, False, False, 1)
        box.pack_start(self.canvas, True, True, 0)

        self.add(box)

    def plot(self, store, iterator):
        '''
        Plot all selected rows in the treeview.
        '''
        timeseries = timeseries_from_store_row(store, iterator)
        name = name_from_store_row(store, iterator)
        self.a.loglog(timeseries.time, timeseries.data, label=name)
        self.set_legend()

        self.a.set_xlabel('t')
        self.a.set_ylabel('G(t)')

        self.a.xaxis.label.set_color(self.text_color)
        self.a.yaxis.label.set_color(self.text_color)

        self.f.canvas.draw()

    def set_legend(self):
        handles, labels = self.a.get_legend_handles_labels()
        legend = self.a.legend(handles, labels, facecolor='None')

        for text in legend.get_texts():
            text.set_color(self.text_color)

    def clear(self):
        '''
        Clear the canvas, used to refresh the plot.
        '''
        self.a.clear()
        self.f.canvas.draw()

class TimeSeries(GObject.Object):
    def __init__(self, time, data):
        GObject.Object.__init__(self)
        self.time = time
        self.data = data

class ContextWrapper(GObject.Object):
    def __init__(self, context, system, cv, view):
        GObject.Object.__init__(self)
        self.context = context
        self.system = system
        self.cv = cv
        self.context_view = view

class Result(GObject.Object):
    '''
    Container class for all (meta)data of a result that has been computed or loaded from file.
    '''
    def __init__(self, time, data, name, datetime=datetime.datetime.now(), path="", context_wrapper=None):
        GObject.Object.__init__(self)
        self.time_series = TimeSeries(time, data)
        self.name = name
        self.datetime = datetime
        self.path = path
        self.context_wrapper = context_wrapper

    def add_to_store(self, store, iterator):
        self.iter = store.append(iterator, [True, self.name, self.datetime, self.path, self.time_series, self.context_wrapper])
        return self.iter

    def get_iter(self):
        return self.iter

class OpenFileDialog(Gtk.FileChooserDialog):
    '''
    Dialog to open space delimited files, commented by '#'
    '''
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

    def read_dat(self):
        file = self.get_file()
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

class SaveFileDialog(Gtk.FileChooserDialog):
    def __init__(self):
        Gtk.FileChooserDialog.__init__(self, title="Please choose a file", action=Gtk.FileChooserAction.SAVE)

        self.add_buttons(
            Gtk.STOCK_CANCEL,
            Gtk.ResponseType.CANCEL,
            Gtk.STOCK_SAVE,
            Gtk.ResponseType.OK,
        )

    def run(self, view):
        try:
            self.selected = get_selected(view)
            return Gtk.FileChooserDialog.run(self)
        except:
            return Gtk.ResponseType.CANCEL

    def write_dat(self):
        '''
        Write context and result from selected row to space delimited .dat
        '''
        path = self.get_file().get_path()
        root, ext = os.path.splitext(path)
        if not ext:
           ext = '.dat'
        path = root + ext

        if os.path.exists(path) == True:
            confirm = ConfirmOverwrite(self, path)
            response = confirm.run()
            confirm.destroy()
            if response == Gtk.ResponseType.CANCEL:
                return False
        
        self.write_selected(path)
        return True

    def write_selected(self, path):
        '''
        Internal utility function to do the actual write. Use write_dat instead, which e.g. asks for overwrite confirmation
        '''
        store, tree_iter = self.selected

        context_wrapper = context_wrapper_from_store_row(store, tree_iter)
        name = name_from_store_row(store, tree_iter)

        with open(path, "wt") as fp:
            fp.write(lime_python.context_view_to_comment(context_wrapper.context_view))

            writer = csv.writer(fp, delimiter=' ')

            timeseries = timeseries_from_store_row(store, tree_iter)

            for row in zip(timeseries.time, timeseries.data):
                writer.writerow(row)

class ConfirmOverwrite(Gtk.MessageDialog):
    def __init__(self, parent, path): 
        Gtk.MessageDialog.__init__(self,
                transient_for=parent,
                flags=0,
                message_type=Gtk.MessageType.QUESTION,
                text="The file " + path + " exists.\nOverwrite?",
            )

        self.add_buttons(
            Gtk.STOCK_CANCEL,
            Gtk.ResponseType.CANCEL,
            Gtk.STOCK_SAVE,
            Gtk.ResponseType.OK,)

        self.show_all()

class VarStore:
    '''
    Maps entry buffers to a dictionary for easy retrieval
    '''
    def __init__(self, builder):
        self.dict = {
            "ge" : builder.get_object("geStore"),
            "n" : builder.get_object("nStore"),
            "t" : builder.get_object("tStore"),
            "ne" : builder.get_object("neStore"),
            "taumonomer" : builder.get_object("taumonStore") ,
            "cv" : builder.get_object("cvStore"),
            "rho" : builder.get_object("rhoStore"),
            "gen_impl" : builder.get_object("crSelectorGenerate"),
            "fit_impl" : builder.get_object("crSelectorFit"),
            "gen_exponent" : builder.get_object("gen_exponentStore"),
            "gen_max" : builder.get_object("gen_maxStore"),
            "fit_weighting" : builder.get_object("fit_weightStore"),
        }

        self.impl_dict = {
            "Rubinstein" : lime_python.cr_impl.Rubinsteincolby,
            "Heuzey" : lime_python.cr_impl.Heuzey,
            "CLF" : lime_python.cr_impl.CLF
        }

    def to_number(self, var):
        '''
        Converts values from entry buffers to floatin gpoint
        '''
        try:
            return float(self.dict[var].get_text())
        except Exception as ex:
            dialog = Gtk.MessageDialog(
                transient_for=self.window,
                flags=0,
                message_type=Gtk.MessageType.INFO,
                buttons=Gtk.ButtonsType.OK,
                text="Please enter a valid number",
            )
            dialog.format_secondary_text(
                f"For variable {var}"
            )
            response = dialog.run()
            dialog.destroy()
            raise ex

    def get_impl(self, tag):
        return self.impl_dict[self.dict[f'{tag}_impl'].get_active_text()]

class Builder:
    '''
    Controls builder functions from the lime library.
    '''
    def __init__(self, window, decouple, var_store):
        self.window = window
        self.var_store = var_store
        self.decouple = decouple

    def get(self):
        context = lime_python.Context()
        system = lime_python.System()
        
        try:
            context.N = self.var_store.to_number("n")
            system.temperature = self.var_store.to_number("t")
            context.N_e = self.var_store.to_number("ne")
            context.tau_monomer = self.var_store.to_number("taumonomer")
            system.density = self.var_store.to_number("rho")
            context.G_e = self.var_store.to_number("ge")
        except:
            return
        
        if self.decouple:
            return lime_python.Decoupled_context_builder(system, context)
        else:
            return lime_python.Context_builder(system, context)

class Generator:
    '''
    Controls generator functions from the lime library.
    '''
    def __init__(self, window, decouple, var_store):
        lime_python.init_factories()

        time_factor = var_store.to_number("gen_exponent")
        time_max = var_store.to_number("gen_max")
        self.time = lime_python.generate_exponential(time_factor, time_max)

        self.impl = var_store.get_impl("gen")

        self.builder = Builder(window, decouple, var_store).get()
        self.cv = var_store.to_number("cv")

    def generate(self):
        self.result = lime_python.ICS_result(self.time, self.builder, self.impl)
        self.result.set_cv(self.cv)

        self.result.calculate()

        context_wrapper = ContextWrapper(self.builder.get_context(), self.builder.get_system(), self.cv, self.builder.get_context_view())

        return Result(self.time, self.result.get_values(), "generated_result", datetime.datetime.now().strftime("%H:%M:%S  %d/%m/%Y"), "", context_wrapper)

class Fit:
    '''
    Controls fit functions from the lime library.
    '''

    def __init__(self, window, decouple, var_store, input_timeseries, callback):
        lime_python.init_factories()

        self.weighting = var_store.to_number("fit_weighting")
        self.decouple = decouple
        self.input_timeseries = input_timeseries
        self.impl = var_store.get_impl("fit")
        self.builder = Builder(window, decouple, var_store).get()
        self.cv = var_store.to_number("cv")
        self.callback = callback

    def fit(self, dialog = None):
        self.result = lime_python.ICS_result(self.input_timeseries.time, self.builder, self.impl)
        self.result.set_cv(self.cv)

        try:
            lime_python.fit(self.decouple, self.result, self.input_timeseries.data, self.weighting, self.callback)
        except Exception as ex:
            raise ex
        finally:
            if dialog:
                dialog.destroy()

        context_wrapper = ContextWrapper(self.builder.get_context(), self.builder.get_system(), self.cv, self.builder.get_context_view())

        return Result(self.input_timeseries.time, self.result.get_values(), "fit_result", datetime.datetime.now().strftime("%H:%M:%S  %d/%m/%Y"), "", context_wrapper)

class ContextViewColumn:
    def __init__(self, grid, column_index):
        self.grid = grid
        self.column_index = column_index

        self.name = self.create_label(0)
        self.T = self.create_label(1)
        self.rho = self.create_label(2)
        self.cv = self.create_label(3)
        self.G_e = self.create_label(4)
        self.N = self.create_label(5)
        self.N_e = self.create_label(6)
        self.Z = self.create_label(7)
        self.tau_monomer = self.create_label(8)
        self.tau_e = self.create_label(9)
        self.tau_r = self.create_label(10)
        self.tau_d_0 = self.create_label(11)
        self.tau_df = self.create_label(12)

    def __del__(self):
        self.grid.remove_column(self.column_index)

    def set_context(self, context_wrapper):
        context = context_wrapper.context
        self.display(context.N, self.N)
        self.display(context.G_e, self.G_e)
        self.display(context.tau_monomer, self.tau_monomer)
        self.display(context.N_e, self.N_e)
        self.display(context.Z, self.Z)
        self.display(context.tau_e, self.tau_e)
        self.display(context.tau_r, self.tau_r)
        self.display(context.tau_d_0, self.tau_d_0)
        self.display(context.tau_df, self.tau_df)

        system = context_wrapper.system
        self.display(system.temperature, self.T)
        self.display(system.density, self.rho)

        cv = context_wrapper.cv
        self.display(cv, self.cv)

    def convert(self, input):
        if input >= 10000:
            out = str(round(input, 4))
            return '%.2E' % Decimal(out)
        else:
            return str(round(input, 4))

    def display(self, input, out):
        out.set_text(self.convert(input))

    def set_name(self, name):
        self.name.set_text(name)

    def create_label(self, row_index):
        label = Gtk.Label()
        self.grid.attach(label, self.column_index, row_index, 1, 1)
        return label

class ContextGrid:
    def __init__(self, initial_grid, store):
        self.grid = initial_grid
        self.store = store
        self.columns = []

    def update(self):
        self.clear()
        loop_tree_active(self.store, self.update_column)
        self.grid.show_all()

    def clear(self):
        del self.columns
        self.columns = []

    def update_column(self, store, tree_iter):
        context_wrapper = context_wrapper_from_store_row(store, tree_iter)

        if context_wrapper:
            name = name_from_store_row(store, tree_iter)

            column = ContextViewColumn(self.grid, self.new_column_index())
            column.set_context(context_wrapper)
            column.set_name(name)

            self.columns.append(column)

    def new_column_index(self):
        return len(self.columns)+1

class Handler:
    '''
    Handles signals emitted by widgets that have been comfigured in glade
    '''
    def __init__(self, window, spinner, decouplers, store, plot_area, treeview, var_store, context_grid):
        self.window = window
        self.spinner = spinner
        self.generate_decoupler, self.fit_decoupler = decouplers
        self.store = store
        self.current = store.get_iter_first()
        self.plot_area = plot_area
        self.treeview = treeview
        self.var_store = var_store
        self.context_grid = context_grid

    def onDestroy(self, *args):
        Gtk.main_quit()

    def on_aboutButton_clicked(self, about):
        about.show_all()

    def on_thanksButton_clicked(self, about):
        about.hide()

    def on_openButton_clicked(self, store):
        dialog = OpenFileDialog()
        response = dialog.run()

        if response == Gtk.ResponseType.OK:
            result = dialog.read_dat()
            for entry in result:
                entry.add_to_store(store, self.current)

        dialog.destroy()

    def on_exportButon_clicked(self, view):
        dialog = SaveFileDialog()
        response = dialog.run(view)

        try:
            if response == Gtk.ResponseType.OK:
                dialog.write_dat()
        finally:
            dialog.destroy()

    def on_resultStore_row_changed(self, store, row_id, tree_iter):
        redraw_plot(store, self.plot_area)
        self.context_grid.update()

    def spin(self):
        GLib.timeout_add(10, self.spinner.start)
        time.sleep(1)

    def on_computeButton_pressed(self, button):
        self.spinner.start()

    def on_computeButton_released(self, tabs):
        '''
        Main control flow for any calculation.
        Works asynchronously and selects operation based on selected tab.
        '''

        i = tabs.get_current_page()

        # Generate
        if i == 0:
            generator = Generator(self.window, self.generate_decoupler.get_active(), self.var_store)

            with concurrent.futures.ThreadPoolExecutor() as executor:
                dialog = None
                self.future = executor.submit(generator.generate)
        # Fit
        elif i == 1:
            # Find selected row in treestore, or show error dialog
            try:
                store, iterator = get_selected(self.treeview)
            except:
                GLib.idle_add(self.spinner.stop)
                return
            
            # Initialize fit object with data points
            timeseries = timeseries_from_store_row(store, iterator)
            fit = Fit(self.window, self.fit_decoupler.get_active(), self.var_store, timeseries, lambda: None)

            with concurrent.futures.ThreadPoolExecutor(max_workers=2) as executor:
                # Dialog to show while computing
                dialog = Gtk.MessageDialog(
                    transient_for=self.window,
                    flags=0,
                    message_type=Gtk.MessageType.INFO,
                    text="Computing ...",)

                # Call fit function asynchronously
                self.future = executor.submit(fit.fit, dialog)

                while not self.future.running():
                    print('waiting for computation to start..')
                    time.sleep(1)

                # Display "Computing ..." dialog while running
                if self.future.running():
                    response = dialog.run()

        # Wait for future to return the result.
        # Display discriptive error dialog if the library throws an exception
        # E.g. when convergence fails
        try:
            return_value = self.future.result()    
            return_value.add_to_store(self.store, self.current)
        except Exception as ex:
            error_dialog = Gtk.MessageDialog(
                    transient_for=self.window,
                    flags=0,
                    message_type=Gtk.MessageType.INFO,
                    buttons=Gtk.ButtonsType.OK,
                    text=f"{ex}"
                )
            error_dialog.run()
            error_dialog.destroy()
        finally:
            GLib.idle_add(self.spinner.stop)

    def on_decouplegeSwitch_activate(self, entry, state):
        '''
        Shows or hides the entry boxes and labels (via signal) for g_e
        '''
        if state == True:
            entry.show()
        else:
            entry.hide()

    def on_geEntry_hide(self, label):
        label.hide()

    def on_geEntry_show(self, label):
        label.show()

##################################
# Programmatically added signals #
##################################

def cell_edited(renderer, path, new_text, treeview):
    if len(new_text) > 0:
        model = treeview.get_model()
        iter = model.get_iter_from_string(path)
        if iter:
            model.set(iter, 1, new_text)

# When the 'show' toggle is clicked, redraw plot accordingly
def on_cell_toggled(widget, path, treeview, plot_area, context_grid):
    # Get the row that was toggled
    model = treeview.get_model()
    tree_iter = model.get_iter_from_string(path)
    # Get the current setting
    val = active_from_store_row(model, tree_iter)
    # Toggle the setting to the other value
    model.set(tree_iter, 0, not val)

    # Redraw plot
    redraw_plot(model, plot_area)

    # Refresh context grid
    context_grid.update()

# Clear plot area and redraw according to the result store
def redraw_plot(store, plot_area):
    plot_area.clear()
    loop_tree_active(store, lambda store, treeiter : plot_area.plot(store, treeiter))

# Loop over all active rows in the store and execute func(store, rowiter)
def loop_tree_active(store, func):
    treeiter = store.get_iter_first()

    while treeiter is not None:
        if active_from_store_row(store, treeiter):
            func(store, treeiter)
            if store.iter_has_child(treeiter):
                childiter = store.iter_children(treeiter)
                func(store, childiter)
        treeiter = store.iter_next(treeiter)

# Assumes setting correctly set to select a single row
def get_selected(treeview):
    try:
        selection = treeview.get_selection()
        (model, path) = selection.get_selected_rows()
        return (model, model.get_iter(path))
    except Exception as ex:
        error_dialog = Gtk.MessageDialog(
            #transient_for=self.window,
            flags=0,
            message_type=Gtk.MessageType.INFO,
            buttons=Gtk.ButtonsType.OK,
            text="Please select a dataset"
        )
        error_dialog.run()
        error_dialog.destroy()
        raise ex

def active_from_store_row(store, iterator):
    return store[iterator][0]

def name_from_store_row(store, iterator):
    return store[iterator][1]

def timeseries_from_store_row(store, iterator):
    return store[iterator][4]

def context_wrapper_from_store_row(store, iterator):
    return store[iterator][5]

# Allow key press events, such as deleting a result from the store
def key_press_event(widget, event, treeview, plot_area, context_grid):
    keyval = event.keyval
    keyval_name = Gdk.keyval_name(keyval)
    state = event.state

    if keyval_name == 'Delete':
        model, tree_iter = get_selected(treeview)
        model.remove(tree_iter)
        plot_area.clear()
        redraw_plot(model, plot_area)
        context_grid.update()


##################################
#             Main               #
##################################

def main():
    # Load glade file
    builder = Gtk.Builder()
    dir, bin = os.path.split(os.path.abspath(__file__))
    builder.add_from_file(f"{dir}/../share/lime/lime.glade")

    # Collect dependencies for the signal handler
    window = builder.get_object("lime")
    spinner = builder.get_object('spinner')
    generate_decoupler = builder.get_object("decouplegeSwitchGenerate")
    fit_decoupler = builder.get_object("decouplegeSwitchFit")
    store = builder.get_object("resultStore")
    plot_area = PlotArea(builder.get_object("nLabelGenerate").get_style_context().get_color(Gtk.StateFlags.NORMAL).to_string())
    treeview = builder.get_object('storeView')
    varstore = VarStore(builder)
    context_grid = ContextGrid(builder.get_object('contextGrid'), store)

    # Construct and connect signal handler
    handler = Handler(window, spinner, (generate_decoupler, fit_decoupler), store, plot_area, treeview, varstore, context_grid)
    builder.connect_signals(handler)

    # Set up the area that will display plots
    graphic_view = builder.get_object("graphicView")
    graphic_view.add(plot_area)

    # Allow key press events, such as deleting a result from the store
    window.connect("key-press-event", key_press_event, treeview, plot_area, context_grid)

    # Populate treeview..
    # .. With a toggle to hide or show graphs
    cell = Gtk.CellRendererToggle()
    cell.set_property('activatable', True)
    cell.set_active(True)
    cell.connect("toggled", on_cell_toggled, treeview, plot_area, context_grid)
    col = Gtk.TreeViewColumn("Show", cell, active=0)
    treeview.append_column(col)
    # .. With an editable name to identify data
    cell = Gtk.CellRendererText()
    cell.set_property("editable", True)
    cell.connect("edited", cell_edited, treeview)
    col = Gtk.TreeViewColumn("Name", cell, text=1)
    treeview.append_column(col)
    # .. With immutable columns to hold metadata
    columns = ["Timestamp",
               "Path"]

    for i, column in enumerate(columns):
        cell = Gtk.CellRendererText()
        col = Gtk.TreeViewColumn(column, cell, text=i+2)
        treeview.append_column(col)

    # Make the program exit when the window is closed
    window.connect("destroy", Gtk.main_quit)
    window.show_all()

    Gtk.main()

if __name__=="__main__":
    main()