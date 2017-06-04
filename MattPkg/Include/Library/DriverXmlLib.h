/** @file
  The main library definition for the XML library that uses basic libraries allowing it
  to be used in a driver.
  
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
#ifndef _DRIVER_XML_LIB_H_
#define _DRIVER_XML_LIB_H_

#pragma pack(push,1)
typedef enum _XML_DATA_TYPE {
  XmlNothing,
  XmlEmptyTag,
  XmlTag,
  XmlCloseTag,
  XmlAttribute,
  XmlChar,
  XmlPi,
  XmlDecl,
  XmlComment,
} XML_DATA_TYPE;

typedef struct _LIST_ANCHOR {
  LIST_ENTRY ListStart;
  UINTN ItemCount;
} LIST_ANCHOR;
//
// Keep as this could be useful for typecasts in the future.
// It's not used in the structs because the syntax becomes too clunky.
//
typedef struct _DRIVER_XML_DATA_HEADER {
  LIST_ENTRY DataLink;
  XML_DATA_TYPE XmlDataType;
} DRIVER_XML_DATA_HEADER;

//
// A linked list element for an XML element or tag (common names for the data object)
// It may have a list of N attributes and be arranged in a tree with N branches.
// There may also be XML character data that this tag describes how to handle.
// That can be broken up into multiple pieces so it needs to be a list as well.
//
typedef struct _DRIVER_XML_TAG {
  LIST_ENTRY DataLink;
  XML_DATA_TYPE XmlDataType; 
  CHAR8* TagName;
  LIST_ANCHOR TagAttributes;
  LIST_ANCHOR TagChildren;
} DRIVER_XML_TAG;

//
// This is used for unpacking an XML attribute object from the XML syntax.
// These are meant to exist as a list of N attributes for an element.
//
typedef struct _DRIVER_XML_ATTRIBUTE {
  LIST_ENTRY DataLink;
  XML_DATA_TYPE XmlDataType;
  CHAR8* AttributeName;
  CHAR8* AttributeData;
} DRIVER_XML_ATTRIBUTE;

//
// XML content is just string data modified by the state of the parent and other ancestors in the tree.
//
typedef struct _DRIVER_XML_CHAR_DATA {
  LIST_ENTRY DataLink;
  XML_DATA_TYPE XmlDataType;
  UINTN DataSize; //track size so we don't need to use AsciiStrLen on this data all the time.
  CHAR8* CharData;
} DRIVER_XML_CHAR_DATA;

typedef struct _DRIVER_XML_PROCESSING_INSTRUCTION {
  LIST_ENTRY DataLink;
  XML_DATA_TYPE XmlDataType; 
  CHAR8* PiTargetName;
  CHAR8* PiTargetData;
} DRIVER_XML_PROCESSING_INSTRUCTION;

//
// This is a housekeeping data structure used by the parser to 
// stream through the XML document as it is extracting chunks.
// It contains the document, the side of the document and the stream pointer OperationPtr.
//
typedef struct _XML_DOCUMENT {
  CHAR8 *XmlDocument;
  UINTN DocumentSize;
  CHAR8 *OperationPtr; 
} XML_DOCUMENT;
#pragma pack(pop)

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
  );

/**
  Finds the next DRIVER_XML_DATA_HEADER node in a linked list. 
  
  @param[in]  Node  A pointer to the current XML element node 
  @param[out] Next  A pointer to the pointer to the next Xml element node
  
  @retval EFI_SUCCESS             The next node is found
  @retval EFI_INVALID_PARAMETER   Node is NULL
**/
EFI_STATUS
GetNextXmlElement (
  IN LIST_ANCHOR *XmlDataList,
  IN DRIVER_XML_DATA_HEADER *ThisItem,
  OUT DRIVER_XML_DATA_HEADER **NextItem
  );

//debugging functions
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
  LIST_ANCHOR*  BranchDataList,
  UINTN TreeLevel
);

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
    DRIVER_XML_DATA_HEADER* Element,
    BOOLEAN Recursve,
    UINTN TreeLevel
);


//Output functions
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
    DRIVER_XML_DATA_HEADER* Element,
    XML_DOCUMENT* OutputDocument
);

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
);


//main parser call
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
  );
#endif
