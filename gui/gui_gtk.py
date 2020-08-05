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

class LimeMain(Gtk.Window):

    def __init__(self):
        Gtk.Window.__init__(self, title="Lime")
        self.set_titlebar(Header())
        self.add(Body(self))

class Header(Gtk.HeaderBar):
    def __init__(self):
        Gtk.HeaderBar.__init__(self)
        self.set_show_close_button(True)
        self.props.title = "Lime"

        self.add_save_button()
        self.add_open_button()

    def add_save_button(self):
        save = Gtk.Button()
        icon = Gio.ThemedIcon(name="mail-send-receive-symbolic")
        image = Gtk.Image.new_from_gicon(icon, Gtk.IconSize.BUTTON)
        save.add(image)
        self.pack_end(save)

    def add_open_button(self):
        box = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL)
        Gtk.StyleContext.add_class(box.get_style_context(), "linked")

        button = Gtk.Button(label="Open")
        box.add(button)
      #  button = Gtk.Button()
      #  button = Gtk.Button.new_from_icon_name("gtk-save-as", Gtk.IconSize.MENU)
     #   box.add(button)

        self.pack_start(box)

class Body(Gtk.Grid):
    def __init__(self, parent):
        Gtk.Grid.__init__(self)

        self.add(FilesArea())
        self.add(PlotArea(parent))

class PlotArea(Gtk.Box):
    def __init__(self, parent):
        Gtk.Box.__init__(self)

        f = Figure(figsize=(500, 400), dpi=100)
        a = f.add_subplot(111)
        t = np.arange(0.0, 3.0, 0.01)
        s = np.sin(2*np.pi*t)
        a.plot(t, s)

        plt.style.use('dark_background')
        f.patch.set_facecolor('xkcd:charcoal grey')
        a.set_facecolor('xkcd:charcoal grey')

        canvas = FigureCanvas(f)

        canvas.toolbar = NavigationToolbar(canvas, self)
        canvas.set_size_request(400,400)
       # canvas.toolbar.setOrientation(QtCore.Qt.Vertical)
      #  canvas.toolbar.setMaximumWidth(35)

        self.add(canvas)

class FilesArea(Gtk.Box):
    def __init__(self):
        Gtk.Box.__init__(self)

        self.tree = Gtk.TreeView()
        self.tree.set_headers_visible(True)
        self.add(self.tree)

        self.add_columns()

    def add_columns(self):
        name_column = Gtk.TreeViewColumn(title="Filename")
        self.tree.append_column(name_column)


win = LimeMain()
win.connect("destroy", Gtk.main_quit)
win.show_all()
Gtk.main()
