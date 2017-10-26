/* XMLUtilities.h */

#ifndef	XML_UTILITIES_H
	
void	XU_RetreiveSettingFromTree	(CFTreeRef tree, CFStringRef setting, CFStringRef& outString);	// Scan a CFtree for a key/value pair
void	XU_PrintTreeContents		(CFTreeRef tree, int level);									// Debug print of XML tree

#endif