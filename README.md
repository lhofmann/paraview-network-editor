# ParaView Network Editor Plugin

![CI](https://github.com/lhofmann/paraview-network-editor/workflows/CI/badge.svg)

This is an attempt to create a 2D network editor for ParaView, that replaces the built-in tree view pipeline editor.

![Demo](demo.gif)

This project is in large parts based on inviwo (https://github.com/inviwo/inviwo), which is licensed under the BSD 2-Clause license. See `LICENSE.inviwo` for details.

## Usage

[GitHub Actions ](https://github.com/lhofmann/paraview-network-editor/actions) (Github login required) is set up to automatically build Linux binaries compatible with ParaView v5.9.1.

Download [ParaView-5.9.1-MPI-Linux-Python3.8-64bit](https://www.paraview.org/files/v5.9/ParaView-5.9.1-MPI-Linux-Python3.8-64bit.tar.gz) and [build artifacts](https://github.com/lhofmann/paraview-network-editor/actions), and extract both in the same location.

### Building from Source

CMake &ge; 3.8, ParaView &ge; 5.8.0, and optionally graphviz are required.

ParaView needs to be build from source, with `BUILD_SHARED_LIBS`, `PARAVIEW_BUILD_QT_GUI` and `PARAVIEW_INSTALL_DEVELOPMENT_FILES` enabled.

Clone and build the plugin
```bash
git clone https://github.com/lhofmann/paraview-network-editor.git
mkdir paraview-network-editor-build
cd paraview-network-editor-build
cmake ../paraview-network-editor -DCMAKE_BUILD_TYPE=Release
cmake --build .
```
Run paraview with
```bash
paraview-network-editor-build/paraview.sh
```
This script runs paraview and automatically loads the plugin.
You can also manually load the file `paraview-network-editor-build/lib/NetworkEditor.so` as plugin.


## Features / "Documentation"

* Can swap places with the main render view
* Indicators for visibility of output ports and color legends for active view
* Indicator for modified pipeline items
* Synchronize selection of sources and output ports
* Add/remove connections
  * drag new connections from output ports
  * drag existing connections from input port to change them
  * drag an existing connection to duplicate it
  * select connections and press delete to remove connections
* Valid/invalid connections are indicated during drag/drop by color (can be overridden by holding Shift key) 
* Hide/show selected sources and color legends (context menu or double click)
* Copy/paste parts of the pipeline (context menu or Ctrl+C/Ctrl+V)
  * including representations (except color maps)
  * Crtl+Shift+V to preserve connections to sources outside of the selection
  * preserving connections works across different ParaView instances
* Support for filters with missing input connection
  * missing first input connection is temporarily replaced with temporary trivial producer to prevent crashes
* Automatic graph layout using optional dependency graphviz
* Use context menu or press Ctrl+Space to place a new source or filter at the last mouse click position
* Node positions are saved/loaded in state files
* All actions can be undone/redone, including moving nodes and graph layout 
* Network can be saved as image (png or svg)
    * Automatically save an image of the pipeline when state files are saved or loaded (default:off)
* Settings menu (`Edit > Settings... > Network Editor`)
* Custom quick launch menu (Ctrl+Space while network editor in focus, or Meta+Space)
  * automatically assigns selected sources to multiple input ports
  * skips dialog for filters with multiple inputs
  * filters unsuitable to current selection are prefixed with ~, but can be added
* CI builds static graphviz libraries
* Editor viewport and scroll position are stored and loaded from state files
* Sticky notes for adding documentation within the network (`Network Editor Sticky Note` source)
  * note can be resized by dragging its edges
  * full text shown in tooltip
  * supports HTML

## TODO / Known Issues

* Add search, that selects pipeline items by name, type, ...
* Add a dock widget, that contains a filterable list of sources/filters, that can be drag/dropped into the network editor
* Display and edit property links
* Add support for grouping/ungrouping parts of the pipeline (custom filters / vtkSMCompoundSourceProxy)
* Add tooltips, that show information about sources and their output ports
* Custom graph layout algorithm, that takes grid into account (not possible with graphviz)
* Choose better node positions when inserting new sources
* Key shortcuts do not work when the dock widget is detached from the main window
