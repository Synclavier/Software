Synclavier Software

Briefly -
   1. You will need XCode to browse the Synclavier sources.
   2. If you don't believe me refer to rule # 1.
   3. The XCode project has several scripts. One builds the entire W0:
      hard drive for Synclavier3. You could set up Synclavier3 to use
      that image file as W1:. That way you could leave Synclavier3
      unmodified, and ENT W1: to run any executables you have built.
   4. You have to build and install the XPL Compiler on your Mac
      before you can build the Real Time Software. And you have to
      build and install AbleDiskTool to create the W0: disk image file.

Not so briefly -

The Able folder contains all the source for the Synclavier Software.
The files were imported from a SCSI hard drive used at N.E.D.
The Able folder preserves the directory structure from the SCSI hard drive.

Notable, very few of the source files have a file name extension.

For a short while I was using TextWrangler as the source editor and modified
TextWrangler to recognize the .xpl file name extension. When the XPL Compiler
looks for a source file it searches for a .xpl version if the extension-less
version does not exist.

XCode is a far better browser for viewing the Synclavier Software. The AbleSourceBrowser
project has most of the source files grouped by module. It is extremely fast to
navigate.

For example, to find the definition of a procedure just search for the "name:".
Practically instantaneous.

XCode currently has no provision for foreign language syntax coloring. Setting the
file type to Pascal is a good solution.

Many files have not have their type converted to Pascal in the XCode project. Just
change the type, click to a different file, then back to the original file and you
will see the syntax coloring applied. Once set it is remembered in the project.

Since the file names do not have extensions, it is not practical to browse the source
files from the Finder.

At this point XCode is by far the best editor/browser for viewing (and compiling) the
Synclavier software.

Cameron Jones, October 27, 2017.


