    if (Cbd->Iopb->MajorFunction == IRP_MJ_READ &&
        Cbd->Iopb->TargetFileObject->FileName.Length == 0)
    {
        LARGE_INTEGER offset = Cbd->Iopb->Parameters.Read.ByteOffset;
        ULONG length = Cbd->Iopb->Parameters.Read.Length;
        PFILE_OBJECT fileObject = Cbd->Iopb->TargetFileObject;
        PDEVICE_OBJECT deviceObject = IoGetRelatedDeviceObject(fileObject);
        UCHAR majorFunction = Cbd->Iopb->MajorFunction;

        DbgPrint("=============================================\n");
        DbgPrint("RAW READ 감지!\n");
        status = CtxGetFileNameInformation(Cbd, &nameInfo);
        if (!NT_SUCCESS(status)) {
            return FLT_PREOP_SUCCESS_NO_CALLBACK;
        }
        DbgPrint("경로 : %wZ , %wZ", &nameInfo->Name,&nameInfo->Volume);
        DbgPrint("---------------------------------------------\n");
        DbgPrint("요청자 모드: %s\n", (Cbd->RequestorMode == UserMode) ? "UserMode" : "KernelMode");
        DbgPrint("MajorFunction: 0x%X (IRP_MJ_READ)\n", majorFunction);
        DbgPrint("읽기 요청 오프셋: %I64d\n", offset.QuadPart);
        DbgPrint("읽기 요청 길이: %lu bytes\n", length);
        DbgPrint("FileObject: 0x%p\n", fileObject);
        DbgPrint("DeviceObject: 0x%p\n", deviceObject);
        DbgPrint("Volume 관련 여부: %s\n", FltObjects->Volume ? "O" : "X");
        DbgPrint("Instance: 0x%p\n", FltObjects->Instance);
        DbgPrint("SectionObjectPointer: 0x%p\n", fileObject->SectionObjectPointer);
        DbgPrint("Flags: 0x%08X\n", fileObject->Flags);
        DbgPrint("FsContext: 0x%p\n", fileObject->FsContext);
        DbgPrint("=============================================\n");
    }