import gi

gi.require_version("Gtk", "3.0")
from gi.repository import Gtk, Gio

from matplotlib.backends.backend_gtk3agg import (
    FigureCanvasGTK3Agg as FigureCanvas)
from matplotlib.backends.backend_gtk3 import (
    NavigationToolbar2GTK3 as NavigationToolbar)
from matplotlib.figure import Figure
import matplotlib.pyplot as plt

# only needed for result check
import numpy as np

from datetime import datetime

class LimeMain(Gtk.Window):
    def __init__(self):
        Gtk.Window.__init__(self, title="Lime")
        self.set_default_size(1200, 500)
        self.set_titlebar(Header(self))
        self.results = ResultsStore()
        self.add(Body(self))

class Result():
    def __init__(self, name, data, saved):
        self.name = name
        self.data = data
        self.saved = saved

    def __str__(self):
        return self.name

    def __repr__(self):
        return self.name

class ResultsStore(Gtk.TreeStore):
    def __init__(self):
        Gtk.TreeStore.__init__(self, str, bool, str)
        self.current = self.append(None)

    def append_from_types(self, result, saved, datetime):
        self.append(self.current, [str(result), saved, str(datetime)])

class Header(Gtk.HeaderBar):
    def __init__(self, parent):
        Gtk.HeaderBar.__init__(self)

        self.parent = parent

        self.set_show_close_button(True)
        self.props.title = "Lime"

        self.add_about_button()
        self.add_open_button()

    def add_about_button(self):
        about = Gtk.Button()
        icon = Gio.ThemedIcon(name="help-about")
        image = Gtk.Image.new_from_gicon(icon, Gtk.IconSize.BUTTON)
        about.add(image)
        about.connect("clicked", self.on_about_clicked)
        self.pack_end(about)

    def add_open_button(self):
        box = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL)
        Gtk.StyleContext.add_class(box.get_style_context(), "linked")

        button = Gtk.Button(label="Open")
        box.add(button)
        button.connect("clicked", self.on_open_clicked)
     #  button = Gtk.Button()
     #  button = Gtk.Button.new_from_icon_name("gtk-save-as", Gtk.IconSize.MENU)
     #  box.add(button)

        self.pack_start(box)

    def on_open_clicked(self, widget):
        dialog = FileDialog(self.parent)
        response = dialog.run()

        if response == Gtk.ResponseType.OK:
            result = dialog.read_dat(dialog.get_file())
            self.parent.results.append_from_types(*result)

        dialog.destroy()

    def on_about_clicked(self, widget):
        dialog = AboutDialog()
        dialog.run()
        dialog.destroy()

class AboutDialog(Gtk.AboutDialog):
    def __init__(self):
        Gtk.AboutDialog.__init__(self)
        self.set_authors("Daniël Emmery")
        self.set_logo(None)
        self.set_license_type(gi.repository.Gtk.License.GPL_3_0)
        self.set_program_name("Lime")
        self.set_version("1.0.0")
        self.set_copyright("Copyright (c) 2020\nDaniël Emmery")
        self.set_comments("Tool for the application of Likhtman & McLeish' reptation theory.\ndoi: 10.1021/ma0200219")
        self.set_website("https://www.github.com/dmry/lime")

class FileDialog(Gtk.FileChooserDialog):
    def __init__(self, parent):
        Gtk.FileChooserDialog.__init__(self, title="Please choose a file", parent=parent, action=Gtk.FileChooserAction.OPEN)

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
        self.data = np.loadtxt(fname=file.get_path(), comments="#")
        self.name = file.get_basename()
        self.saved = True
        self.datetime = file.query_info(Gio.FILE_ATTRIBUTE_TIME_MODIFIED, 0, None).get_modification_date_time()

        result = Result(self.name, self.data, self.saved)

        return (result, self.saved, self.datetime)

class Body(Gtk.Grid):
    def __init__(self, parent):
        Gtk.Grid.__init__(self)

        self.files_area = FilesArea(parent.results)
        self.plot_area = PlotArea()
        self.settings_area = SettingsArea()

        self.add(self.files_area)
        self.attach_next_to(self.plot_area, self.files_area, Gtk.PositionType.RIGHT, 1, 1)
        self.attach_next_to(self.settings_area, self.plot_area, Gtk.PositionType.RIGHT, 1, 1)

class PlotArea(Gtk.ScrolledWindow):
    def __init__(self):
        Gtk.ScrolledWindow.__init__(self, border_width=10)

        self.set_hexpand(True)
        self.set_vexpand(True)

        f = Figure(figsize=(500, 400), dpi=100)
        a = f.add_subplot(111)
        t = np.arange(0.0, 3.0, 0.01)
        s = np.sin(2*np.pi*t)
        a.plot(t, s)
        f.patch.set_facecolor('None')
        f.patch.set_visible(False)
        a.set_facecolor('None')

        canvas = FigureCanvas(f)

        canvas.toolbar = NavigationToolbar(canvas, self)
     #   canvas.set_size_request(400,400)
       # canvas.toolbar.setOrientation(QtCore.Qt.Vertical)
      #  canvas.toolbar.setMaximumWidth(35)

        self.add(canvas)

class FilesArea(Gtk.Box):
    def __init__(self, store):
        Gtk.Box.__init__(self)

        self.tree = Gtk.TreeView(model=store)
        self.tree.set_headers_visible(True)
        self.add(self.tree)

        self.add_columns()

    def add_columns(self):
        name_column = Gtk.TreeViewColumn(title="Filename")
        saved_column = Gtk.TreeViewColumn(title="Saved")
        date_column = Gtk.TreeViewColumn(title="Date")
        self.tree.append_column(name_column)
        self.tree.append_column(saved_column)
        self.tree.append_column(date_column)

class SettingsArea(Gtk.Box):
    def __init__(self):
        Gtk.Box.__init__(self, orientation=Gtk.Orientation.VERTICAL, spacing=6, border_width=10)

        stack = Gtk.Stack()
        stack.set_transition_type(Gtk.StackTransitionType.SLIDE_LEFT_RIGHT)
        stack.set_transition_duration(500)

        stack.add_titled(GenerateSettings(), "generate", "Generate")
        stack.add_titled(FitSettings(), "fit", "Fit")

        stack_switcher = Gtk.StackSwitcher()
        
        stack_switcher.set_halign(3)

        stack_switcher.set_stack(stack)
        self.pack_start(stack_switcher, False, True, 0)
        self.pack_start(stack, False, True, 0)

class EntryGroup(Gtk.Box):
    def __init__(self, label):
        Gtk.Box.__init__(self, orientation=Gtk.Orientation.VERTICAL, spacing=5)
        self.set_border_width(5)
        self.add(Gtk.Label(label=label))
        self.add(Gtk.Entry())

class GenerateSettings(Gtk.Grid):
    def __init__(self):
        Gtk.Grid.__init__(self)

        self.set_halign(3)

        entries = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=10)
        entries.set_spacing(15)
        self.Z = EntryGroup("Z")
        self.add(self.Z)
        #coroutines for this?
        self.M = EntryGroup("M")
        self.attach_next_to(self.M, self.Z, Gtk.PositionType.RIGHT, 1, 1)
        self.G_f_normed = EntryGroup("G_f_normed")
        self.attach_next_to(self.G_f_normed, self.Z, Gtk.PositionType.BOTTOM, 1, 1)
        self.tau_e = EntryGroup("tau_e")
        self.attach_next_to(self.tau_e, self.G_f_normed, Gtk.PositionType.RIGHT, 1, 1)
        self.G_e = EntryGroup("G_e")
        self.attach_next_to(self.G_e, self.G_f_normed, Gtk.PositionType.BOTTOM, 1, 1)
        self.tau_r = EntryGroup("tau_r")
        self.attach_next_to(self.tau_r, self.G_e, Gtk.PositionType.RIGHT, 1, 1)
        self.tau_d_0 = EntryGroup("tau_d_0")
        self.attach_next_to(self.tau_d_0, self.G_e, Gtk.PositionType.BOTTOM, 1, 1)
        self.tau_df = EntryGroup("tau_df")
        self.attach_next_to(self.tau_df, self.tau_d_0, Gtk.PositionType.RIGHT, 1, 1)
        self.tau_monomer = EntryGroup("tau_monomer")
        self.attach_next_to(self.tau_monomer, self.tau_d_0, Gtk.PositionType.BOTTOM, 1, 1)
        self.Me = EntryGroup("Me")
        self.attach_next_to(self.Me, self.tau_monomer, Gtk.PositionType.RIGHT, 1, 1)

        self.button = Gtk.Button(label="Generate")
        self.button.set_border_width(20)
        self.attach_next_to(self.button, self.tau_monomer, Gtk.PositionType.BOTTOM, 2, 1)

        self.add(entries)

class FitSettings(Gtk.Grid):
    def __init__(self):
        Gtk.Grid.__init__(self)


win = LimeMain()
win.connect("destroy", Gtk.main_quit)
win.show_all()
Gtk.main()
