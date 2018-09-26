# Python

Python is used to run the ONNX converters.

## How is it installed

The dashboard needs a Python distribution to install the converter and its dependencies. An isolated Python environment is used to avoid interactions with the system-wide Python distribution.

There are two possibilities to create a Python environment. On the first run of the converter, it will ask the user to choose one:

* Use an existing Python installation. The dashboard looks for executables `python3`, `py` and `python` and, for any of them with `venv` support, lists it as a candidate for installation.
    * If this option is chosen, a virtual environment is created using `venv`. All packages are installed in the environment and system-wide packages are not acessible.
* Download an embedded version of Python (Windows only)
    * If this option is chosen, the embeddable zip file release of Python is downloaded from the [Windows releases page](https://www.python.org/downloads/windows/). This version is a zip file with Python and its standard libraries. It's only available for Windows (due to its lack of a standardized package manager to install Python from).
    * The file is unzipped. The embedded version comes without `pip`, so we install `pip` into it manually. We also change the Python library search path to include pip packages, so that packages can be imported from scripts run with this interpreter.

After either of the above choices, `pip install -r requirements.txt` is run to install the packages specified in `requirements.txt`.
