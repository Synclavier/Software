/*	*** THIS FILE GENERATED AUTOMATICALLY -- DO NOT MODIFY ***	Copyright (c) 1994-98 Opcode Systems, Inc.*/#if OMS_MAC_CFMenum {	uppOMSNProviderDefProcInfo = kPascalStackBased					| RESULT_SIZE(kFourByteCode)				/* pascal long          */					| STACK_ROUTINE_PARAMETER(1, kTwoByteCode)	/* short msg            */					| STACK_ROUTINE_PARAMETER(2, kFourByteCode)	/* long param1          */					| STACK_ROUTINE_PARAMETER(3, kFourByteCode)	/* long param2          */};typedef UniversalProcPtr OMSNProviderDefProcUPP;#define NewOMSNProviderDefProc(userRoutine)	\		(OMSNProviderDefProcUPP)NewRoutineDescriptor((ProcPtr)(userRoutine), uppOMSNProviderDefProcInfo, GetCurrentArchitecture())#define CallOMSNProviderDefProc(userRoutine, msg, param1, param2)	\		(long)CallUniversalProc((UniversalProcPtr)(userRoutine), uppOMSNProviderDefProcInfo, (msg), (param1), (param2))#elsetypedef OMSNProviderDefProc OMSNProviderDefProcUPP;#define NewOMSNProviderDefProc(userRoutine)	\		(OMSNProviderDefProcUPP)(userRoutine)#define CallOMSNProviderDefProc(userRoutine, msg, param1, param2)	\		(*(userRoutine))((msg), (param1), (param2))#endif#if OMS_MAC_CFMenum {	uppOMSNDefineDeviceProcInfo = kPascalStackBased					| RESULT_SIZE(kTwoByteCode)					/* pascal OMSErr        */					| STACK_ROUTINE_PARAMETER(1, kTwoByteCode)	/* short devIndex0      */					| STACK_ROUTINE_PARAMETER(2, kTwoByteCode)	/* OMSUniqueID devUniqueID */					| STACK_ROUTINE_PARAMETER(3, kFourByteCode)	/* long provDevRefCon   */					| STACK_ROUTINE_PARAMETER(4, kFourByteCode)	/* OMSStringPtr deviceName */};typedef UniversalProcPtr OMSNDefineDeviceProcUPP;#define NewOMSNDefineDeviceProc(userRoutine)	\		(OMSNDefineDeviceProcUPP)NewRoutineDescriptor((ProcPtr)(userRoutine), uppOMSNDefineDeviceProcInfo, GetCurrentArchitecture())#define CallOMSNDefineDeviceProc(userRoutine, devIndex0, devUniqueID, provDevRefCon, deviceName)	\		(OMSErr)CallUniversalProc((UniversalProcPtr)(userRoutine), uppOMSNDefineDeviceProcInfo, (devIndex0), (devUniqueID), (provDevRefCon), (deviceName))#elsetypedef OMSNDefineDeviceProc OMSNDefineDeviceProcUPP;#define NewOMSNDefineDeviceProc(userRoutine)	\		(OMSNDefineDeviceProcUPP)(userRoutine)#define CallOMSNDefineDeviceProc(userRoutine, devIndex0, devUniqueID, provDevRefCon, deviceName)	\		(*(userRoutine))((devIndex0), (devUniqueID), (provDevRefCon), (deviceName))#endif#if OMS_MAC_CFMenum {	uppOMSNDefineModeProcInfo = kPascalStackBased					| RESULT_SIZE(kTwoByteCode)					/* pascal OMSErr        */					| STACK_ROUTINE_PARAMETER(1, kTwoByteCode)	/* short devIndex0      */					| STACK_ROUTINE_PARAMETER(2, kTwoByteCode)	/* short modeIndex0     */					| STACK_ROUTINE_PARAMETER(3, kFourByteCode)	/* long provModeRefCon  */					| STACK_ROUTINE_PARAMETER(4, kFourByteCode)	/* OMSStringPtr modeName */};typedef UniversalProcPtr OMSNDefineModeProcUPP;#define NewOMSNDefineModeProc(userRoutine)	\		(OMSNDefineModeProcUPP)NewRoutineDescriptor((ProcPtr)(userRoutine), uppOMSNDefineModeProcInfo, GetCurrentArchitecture())#define CallOMSNDefineModeProc(userRoutine, devIndex0, modeIndex0, provModeRefCon, modeName)	\		(OMSErr)CallUniversalProc((UniversalProcPtr)(userRoutine), uppOMSNDefineModeProcInfo, (devIndex0), (modeIndex0), (provModeRefCon), (modeName))#elsetypedef OMSNDefineModeProc OMSNDefineModeProcUPP;#define NewOMSNDefineModeProc(userRoutine)	\		(OMSNDefineModeProcUPP)(userRoutine)#define CallOMSNDefineModeProc(userRoutine, devIndex0, modeIndex0, provModeRefCon, modeName)	\		(*(userRoutine))((devIndex0), (modeIndex0), (provModeRefCon), (modeName))#endif#if OMS_MAC_CFMenum {	uppOMSNGetNumModesProcInfo = kPascalStackBased					| RESULT_SIZE(kTwoByteCode)					/* pascal short         */					| STACK_ROUTINE_PARAMETER(1, 0)				/* void                 */};typedef UniversalProcPtr OMSNGetNumModesProcUPP;#define NewOMSNGetNumModesProc(userRoutine)	\		(OMSNGetNumModesProcUPP)NewRoutineDescriptor((ProcPtr)(userRoutine), uppOMSNGetNumModesProcInfo, GetCurrentArchitecture())#define CallOMSNGetNumModesProc(userRoutine)	\		(short)CallUniversalProc((UniversalProcPtr)(userRoutine), uppOMSNGetNumModesProcInfo, ())#elsetypedef OMSNGetNumModesProc OMSNGetNumModesProcUPP;#define NewOMSNGetNumModesProc(userRoutine)	\		(OMSNGetNumModesProcUPP)(userRoutine)#define CallOMSNGetNumModesProc(userRoutine)	\		(*(userRoutine))(())#endif#if OMS_MAC_CFMenum {	uppOMSNGetModeRefConProcInfo = kPascalStackBased					| RESULT_SIZE(kFourByteCode)				/* pascal long          */					| STACK_ROUTINE_PARAMETER(1, kTwoByteCode)	/* short modeIndex0     */};typedef UniversalProcPtr OMSNGetModeRefConProcUPP;#define NewOMSNGetModeRefConProc(userRoutine)	\		(OMSNGetModeRefConProcUPP)NewRoutineDescriptor((ProcPtr)(userRoutine), uppOMSNGetModeRefConProcInfo, GetCurrentArchitecture())#define CallOMSNGetModeRefConProc(userRoutine, modeIndex0)	\		(long)CallUniversalProc((UniversalProcPtr)(userRoutine), uppOMSNGetModeRefConProcInfo, (modeIndex0))#elsetypedef OMSNGetModeRefConProc OMSNGetModeRefConProcUPP;#define NewOMSNGetModeRefConProc(userRoutine)	\		(OMSNGetModeRefConProcUPP)(userRoutine)#define CallOMSNGetModeRefConProc(userRoutine, modeIndex0)	\		(*(userRoutine))((modeIndex0))#endif#if OMS_MAC_CFMenum {	uppOMSNSetModePatchNameListProcInfo = kPascalStackBased					| RESULT_SIZE(kTwoByteCode)					/* pascal OMSErr        */					| STACK_ROUTINE_PARAMETER(1, kTwoByteCode)	/* short modeIndex0     */					| STACK_ROUTINE_PARAMETER(2, kFourByteCode)	/* OMSNNameListH nlh    */};typedef UniversalProcPtr OMSNSetModePatchNameListProcUPP;#define NewOMSNSetModePatchNameListProc(userRoutine)	\		(OMSNSetModePatchNameListProcUPP)NewRoutineDescriptor((ProcPtr)(userRoutine), uppOMSNSetModePatchNameListProcInfo, GetCurrentArchitecture())#define CallOMSNSetModePatchNameListProc(userRoutine, modeIndex0, nlh)	\		(OMSErr)CallUniversalProc((UniversalProcPtr)(userRoutine), uppOMSNSetModePatchNameListProcInfo, (modeIndex0), (nlh))#elsetypedef OMSNSetModePatchNameListProc OMSNSetModePatchNameListProcUPP;#define NewOMSNSetModePatchNameListProc(userRoutine)	\		(OMSNSetModePatchNameListProcUPP)(userRoutine)#define CallOMSNSetModePatchNameListProc(userRoutine, modeIndex0, nlh)	\		(*(userRoutine))((modeIndex0), (nlh))#endif#if OMS_MAC_CFMenum {	uppOMSNSetNoteNameListProcInfo = kPascalStackBased					| RESULT_SIZE(kTwoByteCode)					/* pascal OMSErr        */					| STACK_ROUTINE_PARAMETER(1, kTwoByteCode)	/* short modeIndex0     */					| STACK_ROUTINE_PARAMETER(2, kTwoByteCode)	/* short patchNum       */					| STACK_ROUTINE_PARAMETER(3, kFourByteCode)	/* OMSNNameListH nlh    */};typedef UniversalProcPtr OMSNSetNoteNameListProcUPP;#define NewOMSNSetNoteNameListProc(userRoutine)	\		(OMSNSetNoteNameListProcUPP)NewRoutineDescriptor((ProcPtr)(userRoutine), uppOMSNSetNoteNameListProcInfo, GetCurrentArchitecture())#define CallOMSNSetNoteNameListProc(userRoutine, modeIndex0, patchNum, nlh)	\		(OMSErr)CallUniversalProc((UniversalProcPtr)(userRoutine), uppOMSNSetNoteNameListProcInfo, (modeIndex0), (patchNum), (nlh))#elsetypedef OMSNSetNoteNameListProc OMSNSetNoteNameListProcUPP;#define NewOMSNSetNoteNameListProc(userRoutine)	\		(OMSNSetNoteNameListProcUPP)(userRoutine)#define CallOMSNSetNoteNameListProc(userRoutine, modeIndex0, patchNum, nlh)	\		(*(userRoutine))((modeIndex0), (patchNum), (nlh))#endif#if OMS_MAC_CFMenum {	uppOMSNSetCtlNameListProcInfo = kPascalStackBased					| RESULT_SIZE(kTwoByteCode)					/* pascal OMSErr        */					| STACK_ROUTINE_PARAMETER(1, kTwoByteCode)	/* short modeIndex0     */					| STACK_ROUTINE_PARAMETER(2, kTwoByteCode)	/* short patchNum       */					| STACK_ROUTINE_PARAMETER(3, kFourByteCode)	/* OMSNNameListH nlh    */};typedef UniversalProcPtr OMSNSetCtlNameListProcUPP;#define NewOMSNSetCtlNameListProc(userRoutine)	\		(OMSNSetCtlNameListProcUPP)NewRoutineDescriptor((ProcPtr)(userRoutine), uppOMSNSetCtlNameListProcInfo, GetCurrentArchitecture())#define CallOMSNSetCtlNameListProc(userRoutine, modeIndex0, patchNum, nlh)	\		(OMSErr)CallUniversalProc((UniversalProcPtr)(userRoutine), uppOMSNSetCtlNameListProcInfo, (modeIndex0), (patchNum), (nlh))#elsetypedef OMSNSetCtlNameListProc OMSNSetCtlNameListProcUPP;#define NewOMSNSetCtlNameListProc(userRoutine)	\		(OMSNSetCtlNameListProcUPP)(userRoutine)#define CallOMSNSetCtlNameListProc(userRoutine, modeIndex0, patchNum, nlh)	\		(*(userRoutine))((modeIndex0), (patchNum), (nlh))#endif#if OMS_MAC_CFMenum {	uppOMSNAddModeClassificationProcInfo = kPascalStackBased					| RESULT_SIZE(kTwoByteCode)					/* pascal OMSErr        */					| STACK_ROUTINE_PARAMETER(1, kTwoByteCode)	/* short modeIndex0     */					| STACK_ROUTINE_PARAMETER(2, kFourByteCode)	/* OMSStringPtr className */					| STACK_ROUTINE_PARAMETER(3, kFourByteCode)	/* OMSNNameListH nlh    */					| STACK_ROUTINE_PARAMETER(4, kFourByteCode)	/* char *patchGroupBits */					| STACK_ROUTINE_PARAMETER(5, kOneByteCode)	/* Boolean isGlobal     */};typedef UniversalProcPtr OMSNAddModeClassificationProcUPP;#define NewOMSNAddModeClassificationProc(userRoutine)	\		(OMSNAddModeClassificationProcUPP)NewRoutineDescriptor((ProcPtr)(userRoutine), uppOMSNAddModeClassificationProcInfo, GetCurrentArchitecture())#define CallOMSNAddModeClassificationProc(userRoutine, modeIndex0, className, nlh, patchGroupBits, isGlobal)	\		(OMSErr)CallUniversalProc((UniversalProcPtr)(userRoutine), uppOMSNAddModeClassificationProcInfo, (modeIndex0), (className), (nlh), (patchGroupBits), (isGlobal))#elsetypedef OMSNAddModeClassificationProc OMSNAddModeClassificationProcUPP;#define NewOMSNAddModeClassificationProc(userRoutine)	\		(OMSNAddModeClassificationProcUPP)(userRoutine)#define CallOMSNAddModeClassificationProc(userRoutine, modeIndex0, className, nlh, patchGroupBits, isGlobal)	\		(*(userRoutine))((modeIndex0), (className), (nlh), (patchGroupBits), (isGlobal))#endif