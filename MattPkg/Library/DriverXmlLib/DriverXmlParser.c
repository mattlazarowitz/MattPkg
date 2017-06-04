/** @file
  This file contains the main logic for working through a raw XML text document and
  turning it into a tree for further professing.
  
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
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/BaseLib.h>
#include <Library/DriverXmlLib.h>
#include <DriverXmlStringHandlers.h>

VOID
PrintXmlType(
  DRIVER_XML_DATA_HEADER* Data
);

/**
  Create a new attribute from the provided data and add it to the 
  provided list of attributes.

  @param[in out] ParentElement    The element to add the attribute to
  @param[in]     AtrributeName    The name of the attribute.
  @param[in]     AttributeData    The data portion of the attribute.
  
  @return  The new attribute that was allocated and filled out.
**/
DRIVER_XML_ATTRIBUTE*
DriverXmlAddAttribute (
  DRIVER_XML_TAG* ParentElement,
  CHAR8*                  AtrributeName,
  CHAR8*                  AttributeData
  )
{
  DRIVER_XML_ATTRIBUTE* LocalAttribute;
  LIST_ANCHOR*          AttributeList;
  
  LocalAttribute = AllocateZeroPool (sizeof (DRIVER_XML_ATTRIBUTE));
  ASSERT (LocalAttribute != NULL);
  
  AttributeList = &(ParentElement->TagAttributes);
  LocalAttribute->XmlDataType = XmlAttribute;
  LocalAttribute->AttributeName = AtrributeName;
  LocalAttribute->AttributeData = AttributeData;
  
  InsertTailList (&(AttributeList->ListStart), &(LocalAttribute->DataLink));
  AttributeList->ItemCount++;
  
  return LocalAttribute;
}

/**
  Delete an attribute from the list of attributes. 
  This frees all memory associated with the attribute.
 

  @param[in out] AttribList  The attribute list from the parent.
  @param[in]     Attribute   The attribute to delete.
**/
VOID
DriverXmlDeleteAttribute (
  LIST_ANCHOR*          AttribList,
  DRIVER_XML_ATTRIBUTE* Attribute
  )
{
  
  RemoveEntryList (&(Attribute->DataLink));
  AttribList->ItemCount--;
  
  gBS->FreePool (Attribute->AttributeName);
  gBS->FreePool (Attribute->AttributeData);
  gBS->FreePool (Attribute);
  
  return;
}

/**
  Worker function for deleting a list of attributes.

  @param[in out] AttributeList  The list of attributes to delete.

  @retval EFI_SUCCESS The list was successfully deleted.
  @retval 
**/

EFI_STATUS
DeleteAttributeList (
  LIST_ANCHOR* AttributeList
)
{
  DRIVER_XML_DATA_HEADER* ChildData;
  DRIVER_XML_ATTRIBUTE*   Attribute;
  
  while (GetNextXmlElement (AttributeList, ChildData, &ChildData) == EFI_SUCCESS) {
    if (ChildData == NULL) {
      //
      // Sanity check
      //
      return  EFI_NOT_FOUND;
    }
    if (ChildData->XmlDataType != XmlAttribute) {
      DEBUG((DEBUG_ERROR,"Encountered a non-attribute in the attribute list.\n This is unexpected.\n"));
      continue;
    }
    Attribute = (DRIVER_XML_ATTRIBUTE*)ChildData;
    DriverXmlDeleteAttribute (AttributeList, Attribute);
  }
  if (AttributeList->ItemCount != 0) {
    return EFI_ABORTED;
  }
  return EFI_SUCCESS;
}

/**
  Worker function for deleting a list of elements.

  @param[in out] ElementList  The list of elements to delete.

  @retval EFI_SUCCESS The list was successfully deleted.
  @retval 
**/
EFI_STATUS
DeleteElementList (
    LIST_ANCHOR* ElementList
)
{
  DRIVER_XML_DATA_HEADER* ChildData;
  
  if (ElementList != NULL && ElementList->ItemCount > 0) {
    ChildData = (DRIVER_XML_DATA_HEADER*)&ElementList->ListStart;
    while (GetNextXmlElement (ElementList, ChildData, &ChildData) == EFI_SUCCESS) {
      if (ChildData == NULL) {
        return EFI_NOT_FOUND;
      }
      DriverXmlDeleteElement(ElementList, ChildData);
    }
  }
  if (ElementList->ItemCount != 0) {
    return EFI_ABORTED;
  }
  return EFI_SUCCESS;
}


/**
  Worker function for DriverXmlDeleteElement. 
  This handles empty elements that may only have an attribute list.

  @param[in out] ElementList  The list of XML elements to add a new one to.
  @param[in]     Element      The XML tag element to be deleted.
  
**/
EFI_STATUS
DriverXmlDeleteEmptyTag (
  LIST_ANCHOR*            ElementList,
  DRIVER_XML_DATA_HEADER* Element
)
{
  LIST_ANCHOR* AttributeList;
  
  if (Element->XmlDataType != XmlEmptyTag){
    return EFI_INVALID_PARAMETER;
  }
  AttributeList = &((DRIVER_XML_TAG*)Element)->TagAttributes;
  DeleteAttributeList (AttributeList);
  return EFI_SUCCESS;
}

EFI_STATUS
DriverXmlDeleteTagAndChildren (
  LIST_ANCHOR*            ElementList,
  DRIVER_XML_DATA_HEADER* Element
)
{
  LIST_ANCHOR* ListToDelete;
  
  if (Element->XmlDataType != XmlTag){
    return EFI_INVALID_PARAMETER;
  }
  ListToDelete = &((DRIVER_XML_TAG*)Element)->TagAttributes;
  DeleteAttributeList (ListToDelete);
  ListToDelete = &((DRIVER_XML_TAG*)Element)->TagChildren;
  DeleteElementList (ListToDelete);
  
  
  return EFI_SUCCESS;
}
/**
  Delete an element from a list of elements. 
  This will also free all the memory associated with the element
  This will also free all the children and attributes

  @param[in out] ElementList  The list of XML elements to add a new one to.
  @param[in]     Element      The XML tag element to be deleted.
  
**/
EFI_STATUS
DriverXmlDeleteElement (
  LIST_ANCHOR*            ElementList,
  DRIVER_XML_DATA_HEADER* Element
  )
{
  
  
  
  if (Element->XmlDataType == XmlEmptyTag){
    DriverXmlDeleteEmptyTag (ElementList, Element);
  }
  
  if (Element->XmlDataType == XmlTag){
    DriverXmlDeleteTagAndChildren (ElementList, Element);
  }
  

  RemoveEntryList(&(Element->DataLink));
  ElementList->ItemCount--;
  gBS->FreePool(Element);
  return EFI_SUCCESS;
}

/**
  Create a new element data structure and add it to a list of elements.

  @param[in out] ElementList  The list of elements to add the new element to
  @param[in]     TagName  The name of the element to be added.
  @param[in]     DataType     The element type to be added to the list.
  
  @return  The XML element that was allocated with the XML element name filled out.
**/
DRIVER_XML_TAG*
DriverXmlCreateTag (
  LIST_ANCHOR*  ElementList,
  CHAR8*        TagName,
  XML_DATA_TYPE DataType
  )
{
  DRIVER_XML_TAG* Tag;

  Tag = AllocateZeroPool (sizeof(DRIVER_XML_TAG));
  ASSERT (Tag != NULL);
  
  Tag->XmlDataType = DataType;
  Tag->TagName = TagName;
  //
  // Initialize the 2 sub-lists
  //
  InitializeListHead(&Tag->TagChildren.ListStart);
  InitializeListHead(&Tag->TagAttributes.ListStart);
  Tag->TagChildren.ItemCount = 0;
  Tag->TagAttributes.ItemCount = 0;
  
  InsertTailList (&(ElementList->ListStart), &(Tag->DataLink));
  ElementList->ItemCount++;
  return Tag;
}

/**
  Add an XML element to the child list of a provided XML element.
  A new XML element will be allocated and returned to the caller.

  @param[in out] ParentElement       The parent XML element to add a child to.
  @param[in]     ChildTagName    The element name parsed out of the XML data for the child. See the XML spec.
  
  @return  The XML element that was allocated with the XML element name filled out.
**/
DRIVER_XML_TAG*
DriverXmlCreateChildTag (
  DRIVER_XML_TAG* ParentElement,
  CHAR8*                  ChildTagName,
  XML_DATA_TYPE           ChildDataType
)
{
  DRIVER_XML_TAG* ChildElement;
  
  ChildElement = DriverXmlCreateTag (
                   &ParentElement->TagChildren,
                   ChildTagName,
                   ChildDataType
                   );

  return ChildElement;
}

/**
  Add an XML element to the child list of a provided XML element.
  A new XML element will be allocated and returned to the caller.
  
  @param[in out] ParentElement    The parent XML element to add a child to.
  @param[in]     AtrributeName    The element name parsed out of the XML data for the child. See the XML spec.
  
  @return  The XML element that was allocated with the XML element name filled out.
**/
VOID
DriverXmlDeleteChildElement (
  DRIVER_XML_TAG* ParentElement,
  DRIVER_XML_DATA_HEADER* Element
  )
{
  LIST_ANCHOR* ElementList;

  ElementList = &((DRIVER_XML_TAG*)ParentElement)->TagChildren;
  DriverXmlDeleteElement (ElementList,Element);
  return;
}

EFI_STATUS
ParseAttributes (
  DRIVER_XML_TAG* Element,
  CHAR8* Chunk
){
  EFI_STATUS Status;
  CHAR8* Attribute;
  CHAR8* AttributeName;
  CHAR8* AttributeData;
  DRIVER_XML_ATTRIBUTE* LocalXmlAttributes;
  
  
  Status = EFI_SUCCESS;

  
  if (!IsAsciiXmlTagWithAttributes(Chunk,&Attribute)){
    return EFI_SUCCESS;
  }
  //
  // Run through the attributes that may be part of the element
  // There can be N elements so we need to loop until we don't have any more.
  //
  while (!EFI_ERROR(Status)){
    Status = AsciiExtractAttribute(
        &Attribute,
        &AttributeName,
        &AttributeData
        );
    if (!EFI_ERROR(Status)){
      //DEBUG((DEBUG_ERROR, "Got attribute %a:%a\n",AttributeName,AttributeData));
      LocalXmlAttributes = DriverXmlAddAttribute(Element,AttributeName,AttributeData);
    } else {
      //
      // Aborted means we hit the end of the element. 
      // EFI_NOUT_FOUND was used to 
      //
      if (Status != EFI_NOT_FOUND){
        //
        // The format of the attributes is bad.
        // Let the caller know so the element can be disregarded.
        //
        DEBUG((DEBUG_ERROR,"Error getting attributes, bad element!\n"));
        return EFI_INVALID_PARAMETER;
      }
      return Status;
    }
  }  //while (!EFI_ERROR(Status));
  return EFI_SUCCESS;
}

/**
  Add a block of XML chars/content to the list of elements. See StringHandlers.c for the definition of 
  "content".

  @param[in out] ElementList    The list of XML elements to add a new one to.
  @param[in]     CharData       The XML content block to be added.
  
  @return  The XML content element that was allocated with the pointer to the content storage set up.
**/
DRIVER_XML_DATA_HEADER*
DriverXmlAddCharData (
  LIST_ANCHOR* ElementList,
  CHAR8* CharData,
  UINTN CharDataLen
  )
{
  DRIVER_XML_CHAR_DATA *LocalCharData = AllocateZeroPool (sizeof(DRIVER_XML_CHAR_DATA));

  ASSERT(LocalCharData!=NULL);
  
  LocalCharData->XmlDataType = XmlChar;
  //
  // extra +1 is to make sure there is a terminating null if anyone prints it as a string
  //
  LocalCharData->CharData = AllocateZeroPool(CharDataLen + 1); 
  LocalCharData->DataSize = CharDataLen;
  
  gBS->CopyMem (
      LocalCharData->CharData,
        CharData,
        CharDataLen
       );
  InsertTailList(&(ElementList->ListStart), &(LocalCharData->DataLink));
  ElementList->ItemCount++;
  return (DRIVER_XML_DATA_HEADER*)LocalCharData;
}

/**
  This will add either a start tag, or and empty tag to the supplied list.
  This will extract the name and all attributes on the tag. All necessary memory
  is allocated by the worker functions. 

  @param[in out] ParentList  The list of XML elements to add a new one to.
  @param[in]     XmlString   The war XML string containing all the markup
  
  @return  The XML tag data structure that was created.
**/
DRIVER_XML_DATA_HEADER*
DriverXmlAddTag (
    DRIVER_XML_DATA_HEADER* ParentElement,
    CHAR8* XmlString,
    XML_DATA_TYPE DataType
){
  CHAR8* TagName;
  DRIVER_XML_TAG* LocalElement;
  DRIVER_XML_TAG* ParentTag;
  EFI_STATUS Status;

  if (ParentElement->XmlDataType != XmlTag 
      && ParentElement->XmlDataType != XmlEmptyTag)
  {
    return NULL;
  }
  ParentTag = (DRIVER_XML_TAG*)ParentElement;
  Status = AsciiGetTagNameFromElement (
             XmlString,
             &TagName
             );
  LocalElement = DriverXmlCreateChildTag (
                   ParentTag,
                   TagName,
                   DataType
                   );
  Status = ParseAttributes (LocalElement, XmlString);
  if (EFI_ERROR (Status) && Status != EFI_NOT_FOUND) {
    DEBUG((DEBUG_ERROR, "Deleting bad attribute\n"));
    DriverXmlDeleteChildElement(
      ParentTag,
      (DRIVER_XML_DATA_HEADER*)LocalElement
      );
  }
  return (DRIVER_XML_DATA_HEADER*)LocalElement;
}

DRIVER_XML_DATA_HEADER*
DriverXmlAddPI (
    DRIVER_XML_DATA_HEADER* ParentElement,
    CHAR8* XmlString,
    XML_DATA_TYPE DataType
){
  CHAR8* PiTargetName;
  CHAR8* PiTargetData;
  DRIVER_XML_TAG* ParentTag;
  DRIVER_XML_PROCESSING_INSTRUCTION* LocalPi;
  EFI_STATUS Status;
  
  PiTargetName = NULL;
  PiTargetData = NULL;
  ParentTag = (DRIVER_XML_TAG*)ParentElement;
  Status = AsciiGetPIData (
      XmlString,
      &PiTargetName,
      &PiTargetData
      );
  LocalPi = AllocateZeroPool(sizeof(DRIVER_XML_PROCESSING_INSTRUCTION));
  LocalPi->XmlDataType = XmlPi;
  LocalPi->PiTargetName = PiTargetName;
  LocalPi->PiTargetData = PiTargetData;
  
  InsertTailList(&(ParentTag->TagChildren.ListStart), &(LocalPi->DataLink));
  ParentTag->TagChildren.ItemCount++;
  return (DRIVER_XML_DATA_HEADER*)LocalPi;
}

/**
  Actual parser code. The process is as follows:
  1) Extract something from < to the closing > 
     Extraction is aware of <? and <! directives and will act accordingly.
  2) Extracted data (a chunk) is checked if it is a valid element as per the XML spec
  3) Element is checked for the presence of attributes, 
     if it is an empty element, or if it is a close tag.
  4) Attributes are extracted and added to a list of attributes on the element.
  5) If the element is not empty or a close tag, 
     this function is recursively called to parse discovered branches.
  6) Control gets returned to the caller when the close tag for the parent element is found, 
     or the EOF is found. Caller can tell if EOF is expected or not.
  

  @param[in] Xml           The XML document data and stream pointers to assist in parsing..
  @param[in] EndOfData     The end of the data as determined by a caller further up in the process.
  @param[in out] Parent    The parent XML element for the branch.
  
  @return Status information from the parsing process.
  @retval EFI_DEVICE_ERROR  There was a mismatch between start tag and end tag.
  @retval EFI_END_OF_FILE   The end of the document was reached before all start tags were closed,
                            or the end of file was reached in the middle of a chunk of data.
  @retval EFI_DEVICE_ERROR  Malformed data was detected during extraction.
**/
EFI_STATUS
ParseBranch (
  XML_DOCUMENT *Xml,
  CHAR8* EndOfData,
  DRIVER_XML_DATA_HEADER* Parent
  )
{
  CHAR8* Chunk;
  XML_DATA_TYPE DataType;
  CHAR8* TagName;
  CHAR8* ParentName;
  DRIVER_XML_DATA_HEADER* LocalXmlData;
  EFI_STATUS Status;
  CHAR8* OldOpPtr;
  
  // We only expect tags to contain children to parse through.
  if (Parent->XmlDataType != XmlTag) {
    return EFI_INVALID_PARAMETER;
  }
  while (Xml->OperationPtr < EndOfData) {
    OldOpPtr = Xml->OperationPtr;
    Status = AsciiExtractMarkupOrText(
               Xml,
               &Chunk,
               &DataType
               );
    if (!(EFI_ERROR(Status))) {
      switch (DataType) {
      case XmlPi:
        //add handling
        DriverXmlAddPI (
            Parent,
            Chunk,
            DataType
        );
        break;
      case XmlDecl:
        //this is a broad category, break it down more?
        break;
      case XmlChar:
        // Need solid handling here because data can be very long
        // use the operations pointer to get the data size.
        DriverXmlAddCharData (
            &((DRIVER_XML_TAG*)Parent)->TagChildren,
            Chunk,
            Xml->OperationPtr - OldOpPtr
            );
        break;
      case XmlTag:
        LocalXmlData = DriverXmlAddTag(
                         Parent,
                         Chunk,
                         DataType
                         );
        Status = ParseBranch(
            Xml,
            EndOfData,
            LocalXmlData
            );
        break;
      case XmlEmptyTag:
        LocalXmlData = DriverXmlAddTag(
                         Parent,
                         Chunk,
                         DataType
                         );
        break;
      case XmlCloseTag:
        Status = AsciiGetTagNameFromElement(
            Chunk,
            &TagName
            );
        //
        // check if this element matches our parent
        //
        ParentName = ((DRIVER_XML_TAG*)Parent)->TagName;
        if (AsciiStrCmp(TagName, ParentName) != 0){
          DEBUG((DEBUG_ERROR,"Close tag mismatch: Parent: %a current: %a\n", ParentName, TagName));
          gBS->FreePool(TagName);
          gBS->FreePool(Chunk);
          return EFI_DEVICE_ERROR;
        } else {
          gBS->FreePool(TagName);
          gBS->FreePool(Chunk);
          return EFI_SUCCESS; 
        }
        break;
      default:
        break;
      }

      gBS->FreePool(Chunk);
    } else {
      DEBUG((DEBUG_ERROR, "Error %r\n", Status));
      if (Status == EFI_END_OF_FILE) {
        
        //
        // TODO: fix this. We need to make sure we hit the real EOF and we don't have an incomplete file
        // 
        ParentName = ((DRIVER_XML_TAG*)Parent)->TagName;
        if (AsciiStrCmp(ParentName,"Root") == 0) {
        	DEBUG((DEBUG_ERROR,"End of file reached, all done!\n"));
        	return EFI_SUCCESS;
        } else {
          DEBUG((DEBUG_ERROR,"Unclosed tag %a\n",ParentName));
          return EFI_END_OF_FILE;
        }
      }
      DEBUG((DEBUG_ERROR,"Failed to extract next chunk\n"));
      DEBUG((DEBUG_ERROR,"Status returned is %r\n",Status));
      return Status;
    }
  }

  return EFI_SUCCESS;
}

/**
  This is the main function call to parse an XML document.
  It will create a root element and if the caller does not want it, they will need to get the first 
  element in the child list.  
  

  @param[in] DriverXml    The XML document to be parsed.
  @param[in] DocSize      The size of he XML text document
  @param[in out] XmlTree  A pointer to return the root element on.
  
  @retval EFI_DEVICE_ERROR  There was a tag mismatch somewhere in the document.
                            Details will be in debug output.
  @retval EFI_END_OF_FILE   The end of the document was reached before the proper end of an element.
  @retval EFI_DEVICE_ERROR  Some other malformed data was detected.
  
**/
EFI_STATUS
DriverXmlParse(
  VOID* XmlText,
  UINTN DocSize,
  DRIVER_XML_DATA_HEADER** XmlTree
  )
{
  EFI_STATUS Status;  
  XML_DOCUMENT Xml;
  CHAR8* EndOfData;
  DRIVER_XML_TAG* Root = AllocateZeroPool (sizeof (DRIVER_XML_TAG));
  CHAR8* RootStr = AllocateZeroPool (5);
  
  // 
  // XML talks of a root. Create one, and maybe use for metadata in the future.
  // If root were a global, we might need a rocket too.
  //
  Root = AllocateZeroPool (sizeof (DRIVER_XML_TAG));
  RootStr[0] = 'R';
  RootStr[1] = 'o';
  RootStr[2] = 'o';
  RootStr[3] = 't';
  Root->TagName = RootStr;
  Root->XmlDataType = XmlTag;
  
  Xml.XmlDocument = (CHAR8*)XmlText;
  Xml.DocumentSize = DocSize;
  Xml.OperationPtr = Xml.XmlDocument;  
  EndOfData = Xml.XmlDocument + Xml.DocumentSize;
  
  Status = EFI_SUCCESS;

  InitializeListHead(&Root->TagChildren.ListStart);
  while (Xml.OperationPtr < EndOfData) {
    Status = ParseBranch(
        &Xml,
        EndOfData,
        (DRIVER_XML_DATA_HEADER*)Root
        );
    if (EFI_ERROR(Status)) {
      DEBUG((DEBUG_ERROR,"%a Error %r\n", __FUNCTION__, Status));
      break;
    }
    DEBUG((DEBUG_ERROR, "%d children on root\n", Root->TagChildren.ItemCount));
  }
  if (!EFI_ERROR(Status)){
	  *XmlTree = (DRIVER_XML_DATA_HEADER*)Root;
  }
  return EFI_SUCCESS;  
}

