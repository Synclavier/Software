XCode setup

You have to define to source trees to define where the output files go and where intermediate files go.
This has to be done so the build products do not go into the GitHub source tree.

XCode - Preferences - Locations - Source Trees
   Add SYNCLAVIER_BUILD - this is where build products end up
   Add SYNCLAVIER_TEMP  - this is where intermediate files are constructed
   
   Build location could go to your Desktop for example.
   Intermediate files can go anywhere they won't get checked in and are easy to delete.