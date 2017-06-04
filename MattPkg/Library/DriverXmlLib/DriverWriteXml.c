/** @file
  Support for taking a parsed XML tree and printing it out as XML text to a buffer.
  
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
#include <uefi.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/BaseLib.h>
#include <DriverXmlStringHandlers.h>

typedef
EFI_STATUS
(*DRIVER_XML_DATA_PRINTER) (
  DRIVER_XML_DATA_HEADER* Element,
  XML_DOCUMENT* OutputDocument
);

//
// Trying to stream multiple writes out to a buffer we may resize in the middle
// of multiple write operations gets to be complex and error prone.
// The WC3 does not specify a max size for anything so this is just a guess.
// Adjust this define if needed.
//
#define MAX_TEMP_BUFFER_SIZE 128

// The amount to grow our output buffer by when we determine we need to grow it.
#define OUTPUT_DOCUMENT_ALLOCATE_STEP 512

//
// This is the list of debug print worker functions that will be used.
// After creating a new debug print worker, add it to the list here.
//
#define XML_PRINT_FUNCTIONS PrintAttribute,PrintTag,PrintEmptyTag,PrintPi,PrintCharData


/**
  Reallocate an XML buffer to a new size and fix up the internal operation tracking pointer. 
  

  @param[in out] OutputDocument  The XML housekeeping data structure with the pointer 
                                 to the buffer to be reallocated.
  
  @param[in] NewSize             The new size for the buffer.

  
  @retval EFI_INVALID_PARAMETER  Both a zero sized buffer and new size of 0 were requested.
  @retval EFI_OUT_OF_RESOURCES   Unable to obtain a new buffer at the specified size.
                                 The original buffer will still be valid if this happens.
  @retval EFI_SUCCESS            The reallocation was successful.
**/
EFI_STATUS
ReallocateXmlDocument (
  IN OUT XML_DOCUMENT* OutputDocument,
  IN UINTN             NewSize
)
{
  UINTN OpPtrOffset;
  VOID* TmpBuffer;
  
  if (OutputDocument->DocumentSize == 0) {
    //
    // Document has not been initialized yet
    //
    if (NewSize == 0) {
      // 
      // The caller did not pass in a size to allow the buffer to be initialized.
      //
      return EFI_INVALID_PARAMETER;
    }
    //
    // Initialize everything
    //
    OutputDocument->XmlDocument = AllocateZeroPool (NewSize);
    OutputDocument->OperationPtr = OutputDocument->XmlDocument;
    OutputDocument->DocumentSize = NewSize;
  }
  //
  // Calculate an offset for the operations pointer so we can restore it after reallocation
  //
  OpPtrOffset =  (UINTN)OutputDocument->OperationPtr - (UINTN)OutputDocument->XmlDocument;

  TmpBuffer = ReallocatePool(
                OutputDocument->DocumentSize,
                NewSize,
                OutputDocument->XmlDocument
                );
  if (TmpBuffer == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }
  OutputDocument->XmlDocument = TmpBuffer;
  OutputDocument->DocumentSize = NewSize;
  //
  // Restore the operations pointer
  //
  OutputDocument->OperationPtr = (CHAR8*)((UINTN)OutputDocument->XmlDocument + OpPtrOffset);
  return EFI_SUCCESS;
}

/**
  Insert a string into the buffer of the provided XML document. 
  This will happen where the operations pointer specifies and should be 
  after the previous string that was inserted.
  
  @param[in] String              The string to insert into the document.
  @param[in out] OutputDocument  The XML housekeeping data structure used to manage the output buffer.

  @retval EFI_SUCCESS            The insert was successful.
**/
EFI_STATUS
StringToDocument (
  CHAR8*        String,
  XML_DOCUMENT* OutputDocument
  )
{
  UINTN StringLen;
  UINTN DocumentFreeSize;
  
  StringLen = AsciiStrnLenS(String,MAX_TEMP_BUFFER_SIZE);
  DocumentFreeSize = ((UINTN)(OutputDocument->XmlDocument + OutputDocument->DocumentSize)) \
                      - (UINTN)OutputDocument->OperationPtr;
  if (StringLen > DocumentFreeSize) {
    //
    // Out of space.
    // If the string is smaller than the defined reallocation size, over-provision to prevent
    // reallocating for every string.
    // Otherwise, grow the buffer large enough to handle the very long string.
    //
    if (StringLen > OUTPUT_DOCUMENT_ALLOCATE_STEP) {
      ReallocateXmlDocument (OutputDocument, OutputDocument->DocumentSize + StringLen + 1);
    } else {
      ReallocateXmlDocument (OutputDocument, OutputDocument->DocumentSize + OUTPUT_DOCUMENT_ALLOCATE_STEP);
    }
  }
  //
  // Copy the string to the buffer
  //
  gBS->CopyMem (
        OutputDocument->OperationPtr,
        String,
        StringLen
      );
  OutputDocument->OperationPtr += StringLen;
  return EFI_SUCCESS;
  
}

/**
  XML data print handler for XML attribute data

  @param[in]     Data            The XML data to be printed 
  @param[in out] OutputDocument  The XML housekeeping data structure containing the output buffer. 
  
  @retval EFI_SUCCESS      The element type is supported and the data has been output.
  @retval EFI_UNSUPPORTED  This function does not support the XML data type passed in.

**/
EFI_STATUS 
PrintAttribute (
  DRIVER_XML_DATA_HEADER* Data,
  XML_DOCUMENT*           OutputDocument
  )
{
  DRIVER_XML_ATTRIBUTE* Attribute;
  CHAR8*                TmpBuffer;
  UINTN                 BufferOffset;
  
  if (Data->XmlDataType != XmlAttribute) {
    return EFI_UNSUPPORTED;
  }
  //
  // Use a pool to prevent any possible stack issues.
  //
  TmpBuffer = AllocateZeroPool (MAX_TEMP_BUFFER_SIZE);
  BufferOffset = 0;
  
  
  Attribute = (DRIVER_XML_ATTRIBUTE*)Data;
  
  //
  // Use of the ternary operator is to allow for an empty string
  //
  BufferOffset += AsciiSPrint (
                    &TmpBuffer[BufferOffset],
                    MAX_TEMP_BUFFER_SIZE - BufferOffset, 
                    " %a=\"%a\"",
                    Attribute->AttributeName,
                    Attribute->AttributeData == NULL?"":Attribute->AttributeData
                    );
  StringToDocument (TmpBuffer,OutputDocument);
  FreePool (TmpBuffer);
  return EFI_SUCCESS;
}

/**
  XML data print handler for XML tag data

  @param[in]     Data            The XML data to be printed 
  @param[in out] OutputDocument  The XML housekeeping data structure containing the output buffer. 
  
  @retval EFI_SUCCESS      The element type is supported and the data has been output.
  @retval EFI_UNSUPPORTED  This function does not support the XML data type passed in.

**/
EFI_STATUS 
PrintTag (
  DRIVER_XML_DATA_HEADER* Data,
  XML_DOCUMENT*           OutputDocument 
  ) 
{
  DRIVER_XML_TAG*         Tag;
  DRIVER_XML_DATA_HEADER* LocalXmlData;
  LIST_ANCHOR*            ChildList;
  LIST_ANCHOR*            AttributeList;
  CHAR8*                  TmpBuffer;
  UINTN                   BufferOffset;

  if (Data->XmlDataType != XmlTag) {
    return EFI_UNSUPPORTED;
  }
  
  TmpBuffer = AllocateZeroPool(MAX_TEMP_BUFFER_SIZE);
  BufferOffset = 0;
  
  Tag = (DRIVER_XML_TAG*)Data;
  BufferOffset = AsciiSPrint (
                   TmpBuffer,
                   MAX_TEMP_BUFFER_SIZE - BufferOffset, 
                   "<%a",
                   Tag->TagName
                   );
  //
  // Need to write the buffer before the attributes overwrite things
  //
  StringToDocument (TmpBuffer,OutputDocument);

  AttributeList = &Tag->TagAttributes;
  //
  // Run through attributes
  //
  if (AttributeList != NULL && AttributeList->ItemCount > 0) {
    LocalXmlData = (DRIVER_XML_DATA_HEADER*)&AttributeList->ListStart;
    while (GetNextXmlElement (AttributeList,LocalXmlData,&LocalXmlData) == EFI_SUCCESS) {
      if (LocalXmlData == NULL) {
        //Sanity check
        break;
      }
      PrintData(LocalXmlData,OutputDocument);
    }
  }
  StringToDocument (">", OutputDocument);
  
  ChildList = &Tag->TagChildren;
  if ( ChildList != NULL && ChildList->ItemCount > 0) {
    //
    // Print all the children of this tag
    //
    PrintWalkBranch(ChildList,OutputDocument);
  }
  //
  // Close the tag
  //
  BufferOffset = AsciiSPrint (
                   TmpBuffer,
                   MAX_TEMP_BUFFER_SIZE - BufferOffset, 
                   "</%a>",
                   Tag->TagName
                   );
  StringToDocument (TmpBuffer,OutputDocument);
  FreePool (TmpBuffer);
  return EFI_SUCCESS;
}
/**
XML data print handler for processor instruction data

@param[in]     Data            The XML data to be printed 
@param[in out] OutputDocument  The XML housekeeping data structure containing the output buffer. 

@retval EFI_SUCCESS      The element type is supported and the data has been output.
@retval EFI_UNSUPPORTED  This function does not support the XML data type passed in.

**/
EFI_STATUS 
PrintPi (
  DRIVER_XML_DATA_HEADER* Data,
  XML_DOCUMENT*           OutputDocument 
  ) 
{
  DRIVER_XML_PROCESSING_INSTRUCTION* LocalPi;
  CHAR8*                             TmpBuffer;
  UINTN                              BufferOffset;
  
  if (Data->XmlDataType != XmlPi) {
    return EFI_UNSUPPORTED;
  }
  
  TmpBuffer = AllocateZeroPool (MAX_TEMP_BUFFER_SIZE);
  BufferOffset = 0;
  LocalPi = (DRIVER_XML_PROCESSING_INSTRUCTION*)Data;
  
  BufferOffset = AsciiSPrint (
                   TmpBuffer,
                   MAX_TEMP_BUFFER_SIZE - BufferOffset, 
                   "<?%a %a?>", 
                   LocalPi->PiTargetName, 
                   LocalPi->PiTargetData
                   );
  StringToDocument (TmpBuffer, OutputDocument);
  FreePool (TmpBuffer);
  return EFI_SUCCESS;
}

/**
  XML data print handler for an empty XML tag.

  @param[in]     Data            The XML data to be printed 
  @param[in out] OutputDocument  The XML housekeeping data structure containing the output buffer. 
  
  @retval EFI_SUCCESS      The element type is supported and the data has been output.
  @retval EFI_UNSUPPORTED  This function does not support the XML data type passed in.

**/
EFI_STATUS 
PrintEmptyTag (
  DRIVER_XML_DATA_HEADER* Data,
  XML_DOCUMENT* OutputDocument
  ) 
{
  DRIVER_XML_TAG*         Tag;
  DRIVER_XML_ATTRIBUTE*   Attribute;
  DRIVER_XML_DATA_HEADER* LocalXmlData;
  LIST_ANCHOR*            AttributeList;
  CHAR8*                  TmpBuffer;
  UINTN                   BufferOffset;
  
  if (Data->XmlDataType != XmlEmptyTag) {
    return EFI_UNSUPPORTED;
  }
  
  TmpBuffer = AllocateZeroPool (MAX_TEMP_BUFFER_SIZE);
  BufferOffset = 0;
  
  Tag = (DRIVER_XML_TAG*)Data;

  BufferOffset = AsciiSPrint (
                   TmpBuffer,
                   MAX_TEMP_BUFFER_SIZE - BufferOffset, 
                   "<%a",
                   Tag->TagName
                   );
  StringToDocument (TmpBuffer, OutputDocument);
  
  AttributeList = &Tag->TagAttributes;
  
  // run through attributes
  if (AttributeList->ItemCount > 0) {
    LocalXmlData = (DRIVER_XML_DATA_HEADER*)&AttributeList->ListStart;
    while (GetNextXmlElement (AttributeList, LocalXmlData, &LocalXmlData) == EFI_SUCCESS) {
      if (LocalXmlData == NULL) {
        //Sanity check
        break;
      }
      if (LocalXmlData->XmlDataType != XmlAttribute) {
        DEBUG((DEBUG_ERROR,"Encountered a non-attribute in the attribute list.\n This is unexpected.\n"));
        continue;
      }
      Attribute = (DRIVER_XML_ATTRIBUTE*)LocalXmlData;      
      PrintData ((DRIVER_XML_DATA_HEADER*)Attribute,OutputDocument);
    }
  }
  BufferOffset = AsciiSPrint (
                   TmpBuffer,
                   MAX_TEMP_BUFFER_SIZE - BufferOffset, 
                   "/>",
                   Tag->TagName
                   );
  StringToDocument (TmpBuffer,OutputDocument);
  FreePool (TmpBuffer);
  return EFI_SUCCESS;
}

/**
XML data print handler for XML char data

@param[in]     Data            The XML data to be printed 
@param[in out] OutputDocument  The XML housekeeping data structure containing the output buffer. 

@retval EFI_SUCCESS      The element type is supported and the data has been output.
@retval EFI_UNSUPPORTED  This function does not support the XML data type passed in.

**/
EFI_STATUS 
PrintCharData (
  DRIVER_XML_DATA_HEADER* Data,
  XML_DOCUMENT*           OutputDocument 
  ) 
{
  DRIVER_XML_CHAR_DATA* LocalCharData;
  //CHAR8*                             TmpBuffer;
  //UINTN                              BufferOffset;
  
  UINTN DocumentFreeSize;

  if (Data->XmlDataType != XmlChar) {
    return EFI_UNSUPPORTED;
  }
  
  LocalCharData = (DRIVER_XML_CHAR_DATA*)Data;
  DocumentFreeSize = ((UINTN)(OutputDocument->XmlDocument + OutputDocument->DocumentSize)) \
                      - (UINTN)OutputDocument->OperationPtr;
  if (LocalCharData->DataSize > DocumentFreeSize) {
    // Out of space.
    // If the string is smaller than the defined reallocation size, over-provision to prevent
    // reallocating for every string.
    // Otherwise, grow the buffer large enough to handle the very long string.
    //
    if (LocalCharData->DataSize > OUTPUT_DOCUMENT_ALLOCATE_STEP) {
      ReallocateXmlDocument (OutputDocument, OutputDocument->DocumentSize + LocalCharData->DataSize + 1);
    } else {
      ReallocateXmlDocument (OutputDocument, OutputDocument->DocumentSize + OUTPUT_DOCUMENT_ALLOCATE_STEP);
    }
  }
  //
  // The easiest thing to do will be a straight up copy operation
  //
  gBS->CopyMem (
        OutputDocument->OperationPtr,
        LocalCharData->CharData,
        LocalCharData->DataSize
      );
  OutputDocument->OperationPtr += LocalCharData->DataSize;
  return EFI_SUCCESS;
}

/**
  Print XML data to a buffer. this will call into a list of worker functions that do the actual print.
  There is no attempt to make the output pretty. Children are not indented on new lines for example.

  @param[in]     Data            The XML data to be printed 
  @param[in out] OutputDocument  The XML housekeeping data structure containing the output buffer. 
  
  @retval EFI_SUCCESS      The element type is supported and the data has been output.
  @retval EFI_UNSUPPORTED  This function does not support the XML data type passed in.

**/
EFI_STATUS
PrintData(
  DRIVER_XML_DATA_HEADER* Data,
  XML_DOCUMENT*           OutputDocument
)
{
  DRIVER_XML_DATA_PRINTER DataPrinters[] = {XML_PRINT_FUNCTIONS,NULL};
  UINTN                   i;
  EFI_STATUS              Status;
  
  i = 0;
  while (DataPrinters[i] != NULL){
    Status = DataPrinters[i] (Data, OutputDocument);
    if (Status == EFI_SUCCESS) {
      return Status;
    }
    i++;
  }
  
  return EFI_UNSUPPORTED;
}

/**
  This is the main function for printing the data. It walks a branch starting from the provided
  node and calls PrintData on all the elements.
  
  @param[in] BranchList          The start of the list of elements
  @param[in out] OutputDocument  The XML housekeeping data structure that will contain the buffer of 
                                 output data.

**/
EFI_STATUS
PrintWalkBranch(
  LIST_ANCHOR*  BranchList,
  XML_DOCUMENT* OutputDocument
)
{
  DRIVER_XML_DATA_HEADER* BranchData;
  
  BranchData = (DRIVER_XML_DATA_HEADER*)&BranchList->ListStart;
  while (GetNextXmlElement (BranchList, BranchData, &BranchData) == EFI_SUCCESS) {
    if (BranchData == NULL){
      break;
    }
    PrintData(BranchData,OutputDocument);
  }
  return EFI_SUCCESS;
}

