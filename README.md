# Xerces C

See the doc/html/index.html for the description of the Xerces-C++
project and other documentation.

## Build Hints

- libtool is needed (libtoolize command is used)
- In the process below you may see compilation-time warnings when it
is compiling C++ code (down casts, number magnitudes, etc. )
These can safely be ignored.
No ERRORS are acceptable however.
- The steps are:

1. reconf # A script that runs all the pre-configuration steps.
If this fails for you
try running the things it does manually one line at a time.
Perhaps you are missing one of the tools it requires.
2. ./configure --disable-depenency-tracking # creates make files.
3. make # compiles and makes the xerces-c libraries
4. make check # builds and runs built in tests. Takes a few minutes to run. 

Look in test-results.log for test output.
Comparison data is in scripts/sanityTest_ExpectedResult.log

If satisfied, recommended usage is NOT to install these libraries, but rather add the
src/.libs directory to the LD_LIBRARY_PATH, in a script that runs the application that needs this
specific library.

Installing this library (via sudo make install) will overwrite whatever OS version you had installed previously, which
may be newer than the one you are building here. This is not recommended.

# Using SAX2Count as a Validator

Here are example command lines that run SAX2Count as a validator:
Note that the SAX2Count being run here is the shell-script found in samples/SAX2Count,
which sets up the LD_LIBRARY_PATH properly. 

This validates a file:

    SAX2Count -schema=personalNS.xsd personalNS.xml"

This shows that we detect an invalid file:

    SAX2Count -schema=personalNS.xsd personalNS-invalid.xml"

To use this on other large schemas, the schemas must be FLATTENED into a single 
directory, using the sbt-flowerpress plugin. 

You will probably have to use absolute paths for the -schema option and test XML 
data file, as the SAX2Count script seems to wire in some test data locations 
that are part of this project tree. 
