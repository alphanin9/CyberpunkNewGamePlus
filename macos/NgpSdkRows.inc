        // New Game Plus. Mac RVAs for hashes this plugin names. Format: { hash, RVA },  // name.
        // RaiseAssert is mapped so Resolve does not trap. The Mac wrappers never call it.
        // 2577866805 (buffered stream dtor) is inlined into OpenFileStream. No row. Apple code does not call it.
        { 3923848734, 0x27f3bec },  // Telemetry_LoadUsedFactsForImportantFactsList
        { 2393518559, 0x4bc5734 },  // CharacterCustomizationSystem_OnNewGame
        { 4285205681, 0x2d80 },  // Assert_RaiseAssert
        { 4293661415, 0x1c3a544 },  // GetSaveFolder. No args. CString& in x0 (global 0x7a4d3f0, the saves folder).
        { 836113218, 0x4e2f40c },  // MergeDirToPath. x0 src CString*, x1 char*, w2 length, x8 dest. Ensures a trailing '/'.
        { 1194333091, 0x4e2f440 },  // MergeFileToPath. Same registers as MergeDir. No trailing slash. Read the x8 object, not x0.
        { 3788966949, 0x9cc5908 },  // RedFileManager instance. Address of the pointer. RawPtr loads the qword.
        { 1917464012, 0x1c3bcf4 },  // OpenFileStream. Slot +0x10. x1 path, w2 flags, x8 out BaseStream**. x0 unused. Not returned in x0.
        { 4051576167, 0x1c3c410 },  // FindFiles. Slot +0x48. x0 this, x1 root, x2 char*, w3 length, x4 DynArray*, w5 recurse.
        { 1617892594, 0x2a0c3cc },  // QuestsSystem::AddQuest. QuestsSystem vtable +0x1f8 (Windows +0x1f0). x0 this, x1 ResourcePath.
        { 188748408, 0x157b3c0 },  // SessionData::AddArgument thunk (body 0x3f0e900). x0 DynArray*, x1 CName, x2 IType*, x3 object.
        { 1357913210, 0x159f244 },  // StartSession. x0 requests handler, x1 SessionData*. Copies the session to handler +0x6f8; does not destroy the source.
        { 1002050525, 0x1598f28 },  // GameDefinition::ToWorldID. x0 GameDefinition*, x8 WorldID out (shared::mac::ToWorldID).
        { 2635337807, 0x8ece4d0 },  // InkSystem instance. Address of the pointer (the getter 0x488b8dc loads it).
        { 2549221846, 0x9d0e288 },  // CGameEngine instance (address of the pointer). The engine-creation function 0x3d8bfc4 ("Initialize Engine") stores CreateInstance's result here, as Windows stores 0x342ac00.
