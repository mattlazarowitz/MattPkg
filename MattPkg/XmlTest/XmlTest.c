/** @file
  This is a basic app used to test the functionality of the XML library.
  The goal is to expand this into a proper set of unit tests for the library.
  
  Copyright (c) 2016, Matthew Lazarowitz. All rights reserved.<BR>
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the MIT License that accompanies this distribution.

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:
  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
**/
#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Protocol/EfiShell.h>
#include <Protocol/EfiShellParameters.h>
#include <Library/DebugLib.h>
#include <Library/OpenFileLib.h>
#include <Library/DriverXmlLib.h>
#include <Library/HexPrintLib.h>

EFI_SHELL_PROTOCOL*            pEfiShellProtocol;
EFI_SHELL_PARAMETERS_PROTOCOL* pEfiShellParametersProtocol;

/**
  Get the shell and shell parameter protocols. 
  
  @param[in] ImageHandle the image handle of this executable image

  @retval EFI_SUCCESS   Both protocols were found.
  @retval EFI_NOT_FOUND Failed to locate one of the protocols.
                        Error info will be printed to the console.
**/
EFI_STATUS 
InitalizeShellInterfaces (
  EFI_HANDLE ImageHandle
  )
{
  EFI_STATUS Status;
  
  Status = gBS->OpenProtocol (
                  ImageHandle,
                  &gEfiShellParametersProtocolGuid,  
                  &pEfiShellParametersProtocol,
                  NULL,
                  NULL,
                  EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL
                  );
  if (EFI_ERROR (Status)) {
    AsciiPrint ("Unable to locate Shell parameters protocol: %r(0x%x)\n",Status,Status);
    pEfiShellParametersProtocol = NULL;
    return Status;
  }

  Status = gBS->LocateProtocol(
                  &gEfiShellProtocolGuid, 
                  NULL, 
                  &pEfiShellProtocol
                  );
  if (EFI_ERROR (Status)) {
    AsciiPrint ("Unable to locate Shell protocol: %r(0x%x)\n",Status,Status);
    pEfiShellProtocol = NULL;
    return Status;
  }
  return Status;
}

VOID
DbgShowChars (
  UINTN NumChars,
  CHAR8* Chars
  );
EFI_STATUS
XmlTestEntryPoint (
    IN  EFI_HANDLE        ImageHandle,
    IN  EFI_SYSTEM_TABLE  *SystemTable
)
{
  CHAR16*    ArgStrPtr;
  UINTN      Index;
  UINTN      ArgStrLen;
  EFI_STATUS Status;
  CHAR16*    FileArgString;
  CHAR8*     FileBuffer;
  UINTN      FileSize;
  DRIVER_XML_DATA_HEADER* XmlTree;
  XML_DOCUMENT OutputDocument;
  
  FileArgString = NULL;
  ArgStrLen = 0;
  AsciiPrint("entry\n");
  DEBUG((DEBUG_ERROR,"Debug output test\n"));
  Status = InitalizeShellInterfaces (ImageHandle);
  
  //AsciiPrint("processing arguments\n");
  
  for(Index = 1; Index < pEfiShellParametersProtocol->Argc; Index++){
    ArgStrPtr = pEfiShellParametersProtocol->Argv[Index];
    ArgStrLen = StrLen (ArgStrPtr);
    //
    //parse normal dashed arguments.
    //
    if (ArgStrLen == 2 && ArgStrPtr[0] == '-'){
      switch(ArgStrPtr[1]){
        case 'B':
        case 'b':
          pEfiShellProtocol->EnablePageBreak ();
          break;
        default:
          AsciiPrint("Unexpected option %S.\n",ArgStrPtr);
          return EFI_INVALID_PARAMETER;
          break;
        }

    } else {
      //
      // Just assume a non - option is a file name
      //
      FileArgString = ArgStrPtr;
    }
  }//end for loop
  
  if (FileArgString == NULL) {
    AsciiPrint("Please specify an XML file for testing\n");
    return EFI_INVALID_PARAMETER;
  }
  Status = OpenFileFromArgument (
             FileArgString,
             &FileBuffer,
             &FileSize
             );
  if (EFI_ERROR(Status)) {
    AsciiPrint ("Unable to open file %S, %r\n",FileArgString,Status);
    return EFI_INVALID_PARAMETER;
  }
  Status = DriverXmlParse(
             FileBuffer,
             FileSize,
             &XmlTree
           );

  if (EFI_ERROR (Status)) {
    return Status;
  }
  Status = DbgPrintData (XmlTree, TRUE, 0);
  OutputDocument.DocumentSize = 0;
  OutputDocument.OperationPtr = NULL;
  OutputDocument.XmlDocument = NULL;
  Status = PrintData (XmlTree,&OutputDocument);

  AsciiPrint("\n\n");
  DbgShowChars (OutputDocument.DocumentSize, OutputDocument.XmlDocument);
  AsciiPrint("\n");
  HexPrintToConsole (OutputDocument.XmlDocument,OutputDocument.DocumentSize);
  AsciiPrint("\n");
  return EFI_SUCCESS;
}
