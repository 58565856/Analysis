Installation and Integration
============================

SDK
---

For each platform there is an SDK that contains:

  * Static library
  * Headers
  * Examples

Nightly build can be downloaded here: https://lief-project.github.io/packages/sdk while releases are available here: https://github.com/lief-project/LIEF/releases.


Python
------

.. _v10-label:

Since 0.10.0
************

To install nightly build (master):

.. code-block:: console

  $ pip install [--user] --index-url  https://lief-project.github.io/packages lief

Python packages can be found here: https://lief-project.github.io/packages/lief


To install **release** package

.. code-block:: console

  $ pip install lief

Release packages can be found here: `Releases <https://github.com/lief-project/LIEF/releases>`_

Using ``setup.py``, one can build and install lief as follows:

.. code-block:: console

   $ python ./setup.py [--user] install

LIEF modules can also be parameterized using the following options:

   $ python ./setup.py --help
   ...

   --lief-test         Build and make tests
   --ninja             Use Ninja as build system
   --sdk               Build SDK package
   --lief-no-json      Disable JSON module
   --lief-no-logging   Disable logging module
   --lief-no-elf       Disable ELF module
   --lief-no-pe        Disable PE module
   --lief-no-macho     Disable Mach-O module
   --lief-no-android   Disable Android formats
   --lief-no-art       Disable ART module
   --lief-no-vdex      Disable VDEX module
   --lief-no-oat       Disable OAT module
   --lief-no-dex       Disable DEX module

From 0.8.0 to 0.9.0
*******************


To install **release** package


.. code-block:: console

  $ pip install pylief-VERSION.zip

Release packages can be found here: `Releases <https://github.com/lief-project/LIEF/releases>`_


Before 0.8.0
************

To install the Python API (example with ``Python 3.5``):

.. code-block:: console

  $ pip install lief-XX.YY.ZZ_py35.tar.gz


Visual Studio Integration
-------------------------

The pre-built SDK is compiled in release configuration with the *Multi-threaded* runtime library.

As example we compile the following snippet with Visual Studio 2015

.. code-block:: cpp

  #include "stdafx.h"

  #include <LIEF/LIEF.hpp>

  int main()
  {
    std::unique_ptr<LIEF::PE::Binary> pe_binary = LIEF::PE::Parser::parse("C:\\Windows\\explorer.exe");
    std::cout << *pe_binary << std::endl;
    return 0;
  }

First the build type must be set to ``Release``:

.. figure:: _static/windows_sdk/s1.png
  :align: center

  Build type set to ``Release``


Then we need to specify the location of the LIEF include directory:

.. figure:: _static/windows_sdk/s2.png
  :align: center

  LIEF include directory

and the location of the ``LIEF.lib`` library:


.. figure:: _static/windows_sdk/s5.png
  :align: center

  LIEF library

As ``LIEF.lib`` was compiled with the ``\MT`` flag we have to set it:

.. figure:: _static/windows_sdk/s3.png
  :align: center

  *Multi-threaded* as runtime library

LIEF makes use of ``and, or, not`` C++ keywords. As **MSVC** doesn't support these keywords by default, we need to add the special file ``iso646.h``:

.. figure:: _static/windows_sdk/s4.png
  :align: center

  Add ``iso646.h`` file

XCode Integration
-----------------

To integrate LIEF within a XCode project, one needs to follow these steps:

First we create a new project:

.. figure:: _static/xcode_integration/step1.png
  :align: center

  New Project

For this example we select a *Command Line Tool*:

.. figure:: _static/xcode_integration/step2.png
  :align: center

  Command Line Tool


.. figure:: _static/xcode_integration/step3.png
  :align: center

  Project options

Then we need to add the static library ``libLIEF.a`` or the shared one (``libLIEF.dylib``)

.. figure:: _static/xcode_integration/step4.png
  :align: center

  Project configuration - Build Phases


.. figure:: _static/xcode_integration/step5.png
  :align: center

  Project configuration - Build Phases


.. figure:: _static/xcode_integration/step6.png
  :align: center

  Project configuration - Build Phases

In the `Build Settings - Search Paths` one needs to specify the paths to the **include directory** and to location of the LIEF libraries (``libLIEF.a`` and/or ``libLIEF.dylib``)

.. figure:: _static/xcode_integration/step7.png
  :align: center

  Libraries and Include search paths

Once the new project configured we can use LIEF:


.. figure:: _static/xcode_integration/code.png
  :align: center

  Source code

and run it:

.. figure:: _static/xcode_integration/result.png
  :align: center

  Output


CMake Integration
-----------------


External Project
****************

Using `CMake External Project <https://cmake.org/cmake/help/v3.0/module/ExternalProject.html>`_:

.. literalinclude:: _static/CMakeExternalProject.cmake
   :language: cmake
   :lines: 1-42

And now, to be integrated within a project:

.. literalinclude:: _static/CMakeExternalProject.cmake
   :language: cmake
   :lines: 47-

For the compilation:

.. literalinclude:: _static/ReadmeExternalProject.rst
   :language: rst
   :lines: 1-42

A *full* example is available in the ``examples/cmake/external_project`` directory.


find_package()
**************

Using `CMake find_package() <https://cmake.org/cmake/help/v3.0/command/find_package.html>`_:

.. literalinclude:: _static/CMakeFindPackage.cmake
   :language: cmake
   :lines: 5-19

And now, to be integrated within a project:

.. literalinclude:: _static/CMakeFindPackage.cmake
   :language: cmake
   :lines: 20-

For the compilation:

.. literalinclude:: _static/ReadmeFindPackage.rst
   :language: rst

A *full* example is available in the ``examples/cmake/find_package`` directory.
