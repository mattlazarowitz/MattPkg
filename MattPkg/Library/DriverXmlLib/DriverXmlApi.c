/** @file
  General API functions meant to make working with the XML tree easier.
  
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
#include <DriverXmlStringHandlers.h>
#include <Library/DebugLib.h>
#include <Library/BaseLib.h>
#include <Library/MemoryAllocationLib.h>

/**
  Finds the next DRIVER_XML_DATA_HEADER node in a linked list. 
  
  @param[in]  Node  A pointer to the current XML element node 
  @param[out] Next  A pointer to the pointer to the next Xml element node
  
  @retval EFI_SUCCESS             The next node is found
  @retval EFI_INVALID_PARAMETER   Node is NULL
**/
EFI_STATUS
GetNextXmlElement (
  IN LIST_ANCHOR*              XmlDataList,
  IN DRIVER_XML_DATA_HEADER*   ThisItem,
  OUT DRIVER_XML_DATA_HEADER** NextItem
  )
{
  if (ThisItem == NULL || NextItem == NULL) {
  DEBUG ((DEBUG_ERROR, "%a : Input elements cannot be null\n", __FUNCTION__));
  return EFI_INVALID_PARAMETER;
  }
  if (IsNodeAtEnd (&XmlDataList->ListStart, &ThisItem->DataLink)){
    *NextItem = NULL;
    //
    // No next element. 
    // Loops using this function terminate on error.
    // This error seems appropriate to use in this case.
    // 
    return EFI_NOT_FOUND;
  }
  *NextItem = (DRIVER_XML_DATA_HEADER*)GetNextNode (&XmlDataList->ListStart, &ThisItem->DataLink);
  return EFI_SUCCESS;
}

/**
  Find a specific attribute in the list based on its name.
  TODO: Find a better way than with string comparisons (apply a hash table?). 
  
  @param[in]     ElementName         The Attribute Name to look for
  @param[in]     List        A pointer to the linked List of attributes
  @param[in,out] Node        A pointer to the pointer to the node with matching Name
  
  @retval EFI_SUCCESS            A node with matching key was found
  @retval EFI_NOT_FOUND          There is no node with matching key in List, or List is empty
  @retval EFI_INVALID_PARAMETER  List is NULL
**/
EFI_STATUS
GetXmlAttributeByName (
  IN CHAR8*                     Name,
  IN LIST_ANCHOR*               AttributeList,
  IN OUT DRIVER_XML_ATTRIBUTE** Node
  )
{
  DRIVER_XML_ATTRIBUTE*   LocalAttribute;
  DRIVER_XML_DATA_HEADER* LocalXmlData;

  
  if (AttributeList == NULL) {
      DEBUG ((DEBUG_ERROR, "%a : Invalid parameter AttributeList\n", __FUNCTION__));
      return EFI_INVALID_PARAMETER;
  }
  if (AttributeList->ItemCount == 0) {
    return EFI_NOT_FOUND;
  }

  LocalXmlData = (DRIVER_XML_DATA_HEADER*)&AttributeList->ListStart;
  while (GetNextXmlElement (AttributeList,LocalXmlData,&LocalXmlData) == EFI_SUCCESS) {
    if (LocalXmlData == NULL) {
      //Sanity check
      break;
    }
    if (LocalXmlData->XmlDataType != XmlAttribute) {
      DEBUG((DEBUG_ERROR, "Encountered a non-attribute in the attribute list.\n This is unexpected.\n"));
      continue;
    }
    LocalAttribute = (DRIVER_XML_ATTRIBUTE*)LocalXmlData;
    if (AsciiStrCmp (LocalAttribute->AttributeName, Name) == 0) {
      *Node = LocalAttribute;
      return EFI_SUCCESS;
    }
  }
  return EFI_NOT_FOUND;
}

/**
  Looks for a tag matching the provided name in the provided list. 
  This will work recursively traversing branches 
  TODO: Find a better way than with string comparisons (apply a hash table?).
  
  @param[in]     TagName     The Attribute Name to look for
  @param[in]     List        A pointer to the linked List of Elements
  @param[in,out] Node        A pointer to the pointer to the node with matching Name
  
  @retval EFI_SUCCESS            A node with matching key was found
  @retval EFI_NOT_FOUND          There is no node with matching key in List, or List is empty
  @retval EFI_INVALID_PARAMETER  List is NULL
**/
EFI_STATUS
GetXmlTagByName (
  IN CHAR8*               TagName,
  IN LIST_ANCHOR*         ElementList,
  IN OUT DRIVER_XML_TAG** OutputTag
  )
{
  DRIVER_XML_DATA_HEADER* LocalXmlData;
  DRIVER_XML_TAG*         Tag;

  if (ElementList == NULL || OutputTag == NULL) {
    DEBUG ((DEBUG_ERROR, "%a : Inputs cannot be NULL\n", __FUNCTION__ ));
    return EFI_INVALID_PARAMETER;
  }
  if (ElementList->ItemCount == 0) {
    return EFI_NOT_FOUND;
  }
  
  LocalXmlData = (DRIVER_XML_DATA_HEADER*)&ElementList->ListStart;
  while (GetNextXmlElement (ElementList, LocalXmlData, &LocalXmlData) == EFI_SUCCESS) {
    if (LocalXmlData == NULL) {
      //Sanity check
      break;
    }
    if(LocalXmlData->XmlDataType == XmlTag || LocalXmlData->XmlDataType == XmlEmptyTag) {
      Tag = (DRIVER_XML_TAG*)LocalXmlData;
    }
    //
    // Found a match
    //
    if (AsciiStrCmp(Tag->TagName, TagName) == 0) {
      *OutputTag = Tag;
      return EFI_SUCCESS;
    }
    else if (LocalXmlData->XmlDataType == XmlTag && Tag->TagChildren.ItemCount > 0) {
      //
      // Look in children if available
      //
      GetXmlTagByName (
        TagName,
        &Tag->TagChildren,
        OutputTag
      );
    }
    return EFI_NOT_FOUND;
  }
  return EFI_NOT_FOUND;
}

