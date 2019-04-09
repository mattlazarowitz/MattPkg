Apologies for the very basic documentation to accompany this. It will evolve.
This is an initial check-in to get this work into Github so it will not be lost. 

This package contains libraries and projects I (Matthew Lazarowitz) have been working on. 

OpenFileLib:

This was created to streamline the process of opening a file in a shell application.
It checks for the presence of a volume name such as FS0: in a path and assumes the string is a full path
from the volume's root to the file.
If no volume name is specified but there are backslashes indicating a path that included directories
this assumes the path is relative the root of the same volume containing the executable image that contains
this library.
The third possibility is that just a file name is specified and the assumption is that the file will be in 
the same directory as the executable image. 

DebugToConsoleLib:

In certain situations, a programmer may not have a serial console for debug information to get piped out to.
This is a very simple replacement for the DebugLib that redirects the data to the console instead.

PrintHexLib:

This is for visualizing a buffer in a format similar to a hex editor. It's very useful for examining strings containing characters that are not easily visible and in being able to examine buffers as if a programmer was using a debugger to view memory.

DriverXmlLib:

This is a basic XML parser meant to be usable in a driver.
It is not full features as it only supports ASCII, does not have support for any elements that begin with '<!' yet, it does not consume XML specific processor instructions, and does not substitute entity references.

See the TestXml.xml file for some examples of what the parser can support.
Each XML element type can be initially treated as a DRIVER_XML_DATA_HEADER structure.
There is an enum that allows the user to determine the proper data structure to cast a pointer to in order to get the correct element type. 

The tree is based around DRIVER_XML_TAG structures. The tree structure comes from the list TagChildren. 
This is a list of child elements which can be char data, other tags, or other XML elements once they are supported.
The list TagAttributes is a list of DRIVER_XML_ATTRIBUTE which are simply key-value pairs.
The structure DRIVER_XML_PROCESSING_INSTRUCTION is also a key-value pair and allows processor instructions to exist in the tree.
The structure DRIVER_XML_CHAR_DATA is used to contain XML char data.

The LIST_ANCHOR structure is used to bolster the EDK2's existing linked list structures by creating a better defined list head and keeping a total item count.

The library also contains functions to write a tree back out as XML text. The debug versions of the function will try to produce output with each element on its own line and each child indented.
The other set of print functions creates a buffer and prints the text into that. The buffer could them be written to a file or transmitted over an interface.

XmlTest:

This is a shell app that can be used to perform quick tests on the parser. It can take in a file name, open it, bass that on to the parser, then put the tree through both sets of print functions to verify the parser.
The code should be simple enough to understand reasonably quickly.

TODO:
1) Convert XML parser to use hashes rather than copies of the strings. 
  This works fine as a parser for small documents with a single encoding onm reasonably fast systems. 
  Using a hash table to refer to search a tree node would work better. Change nodes to point to the original document.
  One challenge is identifying a good hashing algorithm with an even distrobution.
  Implementing chaining shouldn't be too big a challenge.
2) Add more tree traversal options for searching the tree directly.
3) Improve the API overall for the capability that currently exists.
4) Add entity reference substitution.
5) Begin testing the <! elements (possibly starting with comments and CDATA with conditionals being last)
6) Perform well-formedness checks.
7) Consider how to support other encodings than ASCII (note that hashing is part of this)

I would also like to expand the test app so it performs unit testing on the library's functions. 
