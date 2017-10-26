//--------------------------------------------------------------------------------------------
// XML Utilities
//--------------------------------------------------------------------------------------------

// C of some kind
#include <StdIO.h>

// Local includes
#include "XMLUtilities.h"

void XU_RetreiveSettingFromTree (CFTreeRef tree, CFStringRef setting, CFStringRef& outString)
{
	int i;
	
    // get the XML node for this tree node
    CFXMLNodeRef  node = CFXMLTreeGetNode (tree);

    // get the data string
    CFStringRef string = CFXMLNodeGetString (node);
    
    // get the type code
    CFXMLNodeTypeCode type = CFXMLNodeGetTypeCode (node);
	
	// If found what we are looking for, return data from first child which had better be there
	if (type == kCFXMLNodeTypeElement && CFEqual(string, setting))
	{
        CFTreeRef	  childTree   = CFTreeGetChildAtIndex (tree, 0);
		CFXMLNodeRef  childNode   = CFXMLTreeGetNode (childTree);
		CFStringRef   childString = CFXMLNodeGetString (childNode);
		
		outString = childString;
		return;
	}

    // Else walk the children and recursively look for desired element
    for (i = 0; i < CFTreeGetChildCount(tree); i++) {
        CFTreeRef child = CFTreeGetChildAtIndex (tree, i);
        XU_RetreiveSettingFromTree (child, setting, outString);
		
		// Pop return stack once desired item is found
		if (outString)
			return;
    }
}

#if 0
void XU_PrintTreeContents (CFTreeRef tree, int level)
{
// indent the output
    int i;
    for (i = 0; i < level; i++) {
        printf ("    ");
    }

    // get the XML node for this tree node
    CFXMLNodeRef  node;
    node = CFXMLTreeGetNode (tree);

    // get the data string
    CFStringRef string;
    string = CFXMLNodeGetString (node);
    
    // get the type code
    CFXMLNodeTypeCode type;
    type = CFXMLNodeGetTypeCode (node);

    // extract the string data so we can printf it
    char buffer[4096];
    if (CFStringGetCString (string, buffer, 
                            4096, kCFStringEncodingUTF8)) 
    {
        printf ("%s (%d)\n", buffer, type);
    }

    // walk the children and recursively display the
    // descendants
    for (i = 0; i < CFTreeGetChildCount(tree); i++) {
        CFTreeRef child;
        child = CFTreeGetChildAtIndex (tree, i);
        XU_PrintTreeContents (child, level + 1);
    }
}
#endif
