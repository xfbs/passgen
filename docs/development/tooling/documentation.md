# Documentation

## Website

This website is generated using `mkdocs`. In order to build it, you must install some dependencies:

    pip3 install mkdocs
    pip3 install mkdocs-material
    pip3 install mkdocs-kroki-plugin

You can then generate it with:

    mkdocs build

You can also launch a server that will hot-reload any changes you make:

    mkdocs serve

## Source Documentation

Documentation is generated using doxygen. Both `doxygen` and the `graphviz`
packages need to be installed for it to work. To generate it, run doxygen in
the root of the project:

    doxygen Doxyfile

The resulting documentation is placed in `docs/html`.

