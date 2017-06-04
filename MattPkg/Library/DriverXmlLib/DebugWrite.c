/** @file
  Support for taking a parsed XML tree and printing it out as XML via the debug output pipe.
  
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
#include <Library/UefiBootServicesTableLib.h>
#include <Library/DriverXmlLib.h>
#include <Library/BaseLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DebugLib.h>
#include <DriverXmlStringHandlers.h>

/**
  A worker function to print an XML element. This allows for individual function calls
  to handle specific XML data types. Data is output directed based on the debug library.

  @param [in]Element    The XML element data to be printed.
  @param [in]Recursive  If an element has children, this can prevent a recursive call exploring the branch.
  @param [in]TreeLevel  Used to indicate what level of recursion we are at. 
                        This is used to set the indent level and beautify the output.

  @retval EFI_SUCCESS      The element type is supported and the data has been output.
  @retval EFI_UNSUPPORTED  This function does not support the XML data type passed in.


**/
typedef
EFI_STATUS
(*DRIVER_XML_DBG_DATA_PRINTER) (
  DRIVER_XML_DATA_HEADER* Element,
  BOOLEAN Recursive,
  UINTN TreeLevel
);

//
// This is the list of debug print worker functions that will be used.
// After creating a new debug print worker, add it to the list here.
//
#define DEBUG_PRINT_FUNCTIONS DbgPrintAttribute,DbgPrintTag,DbgPrintEmptyElementTag,DbgPrintPi,DbgPrintCharData
/**
  Print a specified number of characters.

  @param[in] NumChars  The number of characters desired to be printed
  @param[in] Chars     Pointer the array of chars to view.
**/
VOID
DbgShowChars (
  UINTN  NumChars,
  IN CHAR8* Chars
  )
{
  UINTN Index;
  for (Index = 0; Index < NumChars; Index++){
    //
    // The range of printable characters based on the ASCII table.
    //
    if((Chars[Index] >= ' ' && Chars[Index] <= '~')) {
      DEBUG ((DEBUG_ERROR, "%c", Chars[Index]));
    } else {
      //
      // Just print a period for unprintable characters.
      //
      DEBUG ((DEBUG_ERROR, "."));
    }
  }
}

/**
  Print the XMNL data type of an element.

  @param[in] Data  The element to print the type of.
**/
VOID
PrintXmlType (
    IN DRIVER_XML_DATA_HEADER* Data
  )
{
  switch(Data->XmlDataType) {
    case XmlTag:
      DEBUG ((DEBUG_ERROR, "XmlTag\n"));
      break;
    case XmlCloseTag:
      DEBUG ((DEBUG_ERROR, "XmlCloseTag\n"));
      break;
    case XmlEmptyTag:
      DEBUG ((DEBUG_ERROR, "XmlEmptyTag\n"));
      break;
    case XmlChar:
      DEBUG ((DEBUG_ERROR, "XmlChar\n"));
      break;
    case XmlPi:
      DEBUG ((DEBUG_ERROR, "XmlPi\n"));
      break;
    case XmlDecl:
      DEBUG ((DEBUG_ERROR, "XmlDecl\n"));
      break;
    case XmlComment:
      DEBUG ((DEBUG_ERROR, "XmlComment\n"));
      break;
    case XmlAttribute:
      DEBUG ((DEBUG_ERROR, "XmlAttribute\n"));
      break;
    case XmlNothing:
      DEBUG ((DEBUG_ERROR, "XmlNothing\n"));
      break;
  }
}

/**
  Print an attribute. This generally gets called directly y the tag element print function.

  @param [in]Element    The XML element data to be printed.
  @param [in]Recursive  If an element has children, this can prevent a recursive call exploring the branch.
  @param [in]TreeLevel  Used to indicate what level of recursion we are at. 
                        This is used to set the indent level and beautify the output.

  @retval EFI_SUCCESS      The element type is supported and the data has been output.
  @retval EFI_UNSUPPORTED  This function does not support the XML data type passed in.

**/

EFI_STATUS 
DbgPrintAttribute (
  IN DRIVER_XML_DATA_HEADER* Data,
  IN BOOLEAN                 Recursive,
  IN UINTN                   TreeLevel
  ) 
{
  DRIVER_XML_ATTRIBUTE* Attribute;
  //
  // Not our data type, tell the dispatcher.
  //
  if (Data->XmlDataType != XmlAttribute) {
    return EFI_UNSUPPORTED;
  }

  Attribute = (DRIVER_XML_ATTRIBUTE*)Data;
  DEBUG((DEBUG_ERROR," %a=\"%a\"",Attribute->AttributeName,Attribute->AttributeData == NULL?"":Attribute->AttributeData));
  return EFI_SUCCESS;
}


/**
  Print an XML tag to the debug pipe.

  @param [in]Element    The XML element data to be printed.
  @param [in]Recursive  If an element has children, this can prevent a recursive call exploring the branch.
  @param [in]TreeLevel  Used to indicate what level of recursion we are at. 
                        This is used to set the indent level and beautify the output.

  @retval EFI_SUCCESS      The element type is supported and the data has been output.
  @retval EFI_UNSUPPORTED  This function does not support the XML data type passed in.

**/
EFI_STATUS 
DbgPrintTag (
  IN DRIVER_XML_DATA_HEADER* Data,
  IN BOOLEAN                 Recursive,
  IN UINTN                   TreeLevel
  ) 
{
  DRIVER_XML_TAG*         Tag;
  DRIVER_XML_ATTRIBUTE*   Attribute;
  DRIVER_XML_DATA_HEADER* LocalXmlData;
  LIST_ANCHOR*            ChildList;
  LIST_ANCHOR*            AttributeList;
  CHAR8*                  Prefix;
  UINTN                   LeadingSpaces;
  
  //
  // Not our data type, tell the dispatcher.
  //
  if (Data->XmlDataType != XmlTag) {
    return EFI_UNSUPPORTED;
  }
  //
  // As with EDK2 coding standard, indent by 2 spaces
  //
  LeadingSpaces = TreeLevel * 2;
  //
  // Set up our prefex to provide a level of indentation that makes the output more readable.
  //
  Prefix = AllocateZeroPool(LeadingSpaces + 1);
  gBS->SetMem(Prefix,LeadingSpaces + 1,0x20);
  Prefix[LeadingSpaces] = 0;
  Tag = (DRIVER_XML_TAG*)Data;
  DEBUG((DEBUG_ERROR,"%a<%a",Prefix,Tag->TagName));
  AttributeList = &Tag->TagAttributes;
  //
  // Run through attributes if there are any.
  //
  if (AttributeList != NULL && AttributeList->ItemCount > 0) {
    LocalXmlData = (DRIVER_XML_DATA_HEADER*)&AttributeList->ListStart;
    while (GetNextXmlElement (AttributeList,LocalXmlData,&LocalXmlData) == EFI_SUCCESS) {
      if (LocalXmlData == NULL) {
        //Sanity check
        break;
      }
      if (LocalXmlData->XmlDataType != XmlAttribute) {
        DEBUG((DEBUG_ERROR,"Encountered a non-attribute in the attribute list.\n This is unexpected.\n"));
        continue;
      }
      Attribute = (DRIVER_XML_ATTRIBUTE*)LocalXmlData;
      DbgPrintData ((DRIVER_XML_DATA_HEADER*)Attribute,FALSE, TreeLevel + 1);
    }
  }
  DEBUG((DEBUG_ERROR, ">\n", Tag->TagName));
  ChildList = &Tag->TagChildren;
  //
  // Does this tag have children that need to be printed?
  //
  if (Recursive && ChildList != NULL && ChildList->ItemCount > 0) {
    DbgWalkBranch (ChildList,TreeLevel + 1);
    
  }
  DEBUG ((DEBUG_ERROR, "%a</%a>\n", Prefix, Tag->TagName));
  FreePool (Prefix);
  return EFI_SUCCESS;
}

/**
  Print an XML empty tag to the debug pipe.

  @param [in]Element    The XML element data to be printed.
  @param [in]Recursive  If an element has children, this can prevent a recursive call exploring the branch.
  @param [in]TreeLevel  Used to indicate what level of recursion we are at. 
                        This is used to set the indent level and beautify the output.

  @retval EFI_SUCCESS      The element type is supported and the data has been output.
  @retval EFI_UNSUPPORTED  This function does not support the XML data type passed in.

**/
EFI_STATUS 
DbgPrintEmptyElementTag (
  IN DRIVER_XML_DATA_HEADER* Data,
  IN BOOLEAN                 Recursive,
  IN UINTN                   TreeLevel
  ) 
{
  DRIVER_XML_TAG*         Tag;
  DRIVER_XML_ATTRIBUTE*   Attribute;
  LIST_ANCHOR*            AttributeList;
  DRIVER_XML_DATA_HEADER* LocalXmlData;
  CHAR8*                  Prefix;
  UINTN                   LeadingSpaces;
  
  if (Data->XmlDataType != XmlEmptyTag) {
    return EFI_UNSUPPORTED;
  }
  //
  // As with EDK2 coding standard, indent by 2 spaces
  //
  LeadingSpaces = TreeLevel * 2;
  //
  // Set up our prefex to provide a level of indentation that makes the output more readable.
  //
  Prefix = AllocateZeroPool (LeadingSpaces + 1);
  gBS->SetMem (Prefix,LeadingSpaces + 1,0x20);
  Prefix[LeadingSpaces] = 0;
  
  Tag = (DRIVER_XML_TAG*)Data;
  DEBUG ((DEBUG_ERROR, "%a<%a", Prefix, Tag->TagName));
  AttributeList = &Tag->TagAttributes;
  //
  // run through attributes if there are any.
  //
  if (AttributeList->ItemCount > 0) {

    LocalXmlData = (DRIVER_XML_DATA_HEADER*)&AttributeList->ListStart;
    while (GetNextXmlElement (AttributeList,LocalXmlData,&LocalXmlData) == EFI_SUCCESS) {
      if (LocalXmlData == NULL) {
        //Sanity check
        break;
      }
      if (LocalXmlData->XmlDataType != XmlAttribute) {
        DEBUG ((DEBUG_ERROR, "Encountered a non-attribute in the attribute list.\n This is unexpected.\n"));
        continue;
      }
      Attribute = (DRIVER_XML_ATTRIBUTE*)LocalXmlData;
      DbgPrintData((DRIVER_XML_DATA_HEADER*)Attribute,FALSE, TreeLevel + 1);
    }
  }
  DEBUG ((DEBUG_ERROR, "/>\n",Tag->TagName));
  FreePool (Prefix);
  return EFI_SUCCESS;
}

/**
  Print an XML processor instruction to the debug pipe.

  @param [in]Element    The XML element data to be printed.
  @param [in]Recursive  If an element has children, this can prevent a recursive call exploring the branch.
  @param [in]TreeLevel  Used to indicate what level of recursion we are at. 
                        This is used to set the indent level and beautify the output.

  @retval EFI_SUCCESS      The element type is supported and the data has been output.
  @retval EFI_UNSUPPORTED  This function does not support the XML data type passed in.

**/
EFI_STATUS 
DbgPrintPi (
  IN DRIVER_XML_DATA_HEADER* Data,
  IN BOOLEAN                 Recursive,
  IN UINTN                   TreeLevel
  ) 
{
  DRIVER_XML_PROCESSING_INSTRUCTION* LocalPi;
  CHAR8*                             Prefix;
  UINTN                              LeadingSpaces;
  
  if (Data->XmlDataType != XmlPi) {
    return EFI_UNSUPPORTED;
  }
  //
  // As with EDK2 coding standard, indent by 2 spaces
  //
  LeadingSpaces = TreeLevel * 2;
  //
  // Set up our prefex to provide a level of indentation that makes the output more readable.
  //
  Prefix = AllocateZeroPool (LeadingSpaces + 1);
  gBS->SetMem (Prefix,LeadingSpaces + 1,0x20);
  Prefix[LeadingSpaces] = 0;
  LocalPi = (DRIVER_XML_PROCESSING_INSTRUCTION*)Data;
  DEBUG ((DEBUG_ERROR, "<?%a %a?>\n", LocalPi->PiTargetName, LocalPi->PiTargetData));

  FreePool (Prefix);
  return EFI_SUCCESS;
}

/**
  Print XML char data to the debug pipe.

  @param [in]Element    The XML element data to be printed.
  @param [in]Recursive  If an element has children, this can prevent a recursive call exploring the branch.
  @param [in]TreeLevel  Used to indicate what level of recursion we are at. 
                        This is used to set the indent level and beautify the output.

  @retval EFI_SUCCESS      The element type is supported and the data has been output.
  @retval EFI_UNSUPPORTED  This function does not support the XML data type passed in.

**/
EFI_STATUS 
DbgPrintCharData (
  IN DRIVER_XML_DATA_HEADER* Data,
  IN BOOLEAN                 Recursive,
  IN UINTN                   TreeLevel
  ) 
{
  DRIVER_XML_CHAR_DATA* LocalCharData;
  CHAR8*                Prefix;
  UINTN                 LeadingSpaces;
  
  if (Data->XmlDataType != XmlChar) {
    return EFI_UNSUPPORTED;
  }
  //
  // As with EDK2 coding standard, indent by 2 spaces
  //
  LeadingSpaces = TreeLevel * 2;
  //
  // Set up our prefex to provide a level of indentation that makes the output more readable.
  //
  Prefix = AllocateZeroPool (LeadingSpaces + 1);
  gBS->SetMem (Prefix,LeadingSpaces + 1,0x20);
  Prefix[LeadingSpaces] = 0;
  LocalCharData = (DRIVER_XML_CHAR_DATA*)Data;
  DEBUG ((DEBUG_ERROR, "%a",Prefix));
  DbgShowChars(LocalCharData->DataSize, LocalCharData->CharData);
  DEBUG ((DEBUG_ERROR, "\n"));
  FreePool (Prefix);
  return EFI_SUCCESS;
}

/**
  The call that determines what data printer to use. This calls through the list of debug data print functions
  until one returns success.

  @param [in]Element    The XML element data to be printed.
  @param [in]Recursive  If an element has children, this can prevent a recursive call exploring the branch.
  @param [in]TreeLevel  Used to indicate what level of recursion we are at. 
                        This is used to set the indent level and beautify the output.

  @retval EFI_SUCCESS      The element type is supported and the data has been output.
  @retval EFI_UNSUPPORTED  This function does not support the XML data type passed in.

**/
EFI_STATUS
DbgPrintData(
  IN DRIVER_XML_DATA_HEADER* Data,
  IN BOOLEAN                 Recursive,
  IN UINTN                   TreeLevel
){
  DRIVER_XML_DBG_DATA_PRINTER DataPrinters[] = {DEBUG_PRINT_FUNCTIONS, NULL};
  UINTN i = 0;
  EFI_STATUS Status;
  while (DataPrinters[i] != NULL){
    Status = DataPrinters[i] (Data, Recursive, TreeLevel);
    if (Status == EFI_SUCCESS) {
      return Status;
    }
    i++;
  }
  
  return EFI_UNSUPPORTED;
}

/**
  This is the main debug print function that will iterate over each XML element
  and call the function that located the correct debug print worker.

  @param[in] BranchDataList  The branch of the tree to print data on. 
  @param [in]TreeLevel  Used to indicate what level of recursion we are at. 
                        This is used to set the indent level and beautify the output.
                        
  @retval EFI_SUCCESS      All supported data types were printed.
**/
EFI_STATUS
DbgWalkBranch(
  IN LIST_ANCHOR*  BranchDataList,
  IN UINTN         TreeLevel
)
{
  DRIVER_XML_DATA_HEADER* BranchDataItem;
  EFI_STATUS              Status;
  
  if (BranchDataList->ItemCount == 0) {
    return EFI_SUCCESS;
  }
  BranchDataItem = (DRIVER_XML_DATA_HEADER*)&BranchDataList->ListStart;
  while (GetNextXmlElement (BranchDataList, BranchDataItem, &BranchDataItem) == EFI_SUCCESS) {
    if (BranchDataItem == NULL) {
      break;
    }
    Status = DbgPrintData((DRIVER_XML_DATA_HEADER*)BranchDataItem,TRUE,TreeLevel);
    if (EFI_ERROR (Status)) { 
      DEBUG((DEBUG_ERROR, "%a encountered unsupported data\n", __FUNCTION__));
    }
  }
  return EFI_SUCCESS;
}
