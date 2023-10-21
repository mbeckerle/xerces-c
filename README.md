# Xerces C

See the doc/html/index.html for the description of the Xerces-C++
project and other documentation.

## Build Hints

The instructions on the Xerces C web site for building work using cmake. 

# Using SAX2Count as a Validator

Here are example command lines that run SAX2Count as a validator:

Note that the SAX2Count being run here is samples/SAX2Count.

The actual library lives under src/libxerces-c-3.2.so, and so you may have to set 
LD_LIBRARY_PATH to include that directory. Though if libxerces-c-3.2.so is installed 
on your system that will also work. 

The changes made here are only to SAX2Count sample application. 

This validates a file:

    SAX2Count -schema=personalNS.xsd personalNS.xml

If the local relative path names do not work, try absolute path names. That definitely works. 

This shows that we detect an invalid file:

    SAX2Count -schema=personalNS.xsd personalNS-invalid.xml

To use this on other large schemas, the schemas must be FLATTENED into a single 
directory, using the sbt-flowerpress plugin. 

You will probably have to use absolute paths for the -schema option and test XML 
data file.



