/** @file
  Internal function prototypes for the string handling functions used in the XML parser.
  
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
  
  @par WC3 XML specification https://www.w3.org/TR/REC-xml/
  
**/

#include <uefi.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/BaseLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DebugLib.h>
#include <DriverXmlStringHandlers.h>

VOID
DbgShowChars (
  UINTN  NumChars,
  IN CHAR8* Chars
  );

/**
  Check if a string contains a minimum number of characters.

  @param[in] Str          Pointer to the string/character array to be checked.
  @param[in] MinLen       The number of charters we wish to see if the array meets or exceeds.
  
  @retval TRUE  There are at least MinLen characters in the string.
  @retval FALSE There are not MinLen characters in the string.
**/
BOOLEAN 
IsStrLenAtLeast (
  IN CHAR8* Str,
  IN UINTN  MinLen
  ) 
{
  UINTN i;
  for (i=0;i <= MinLen; i++){
    if (Str[i] == '\0') return FALSE;
  }
  return TRUE;
}

/**
  Check if a character is whitespace based on the WC3 XML specification. 
  Spec says the definition is:
  S    ::=    (#x20 | #x9 | #xD | #xA)+

  @param[in] Character  The character to be tested.
  
  @retval TRUE   The character is a whitespace character.
  @retval FALSE  The character is not a whitespace character.
**/
BOOLEAN
IsAsciiWhitespace (
  IN CHAR8 Character   
  )
{

  switch(Character) {
    case ' ': //0x20
    case '\t': //0x09
    case '\r': //0x0D
    case '\n': //0x0A
      return TRUE;
    default:
      break;
  }
  return FALSE;
}

/**
  Check if a character is a valid name start character as defined in the XML spec. 
  Spec says the definition is:
  NameStartChar    ::=    ":" | [A-Z] | "_" | [a-z] | <a bunch for UTF16 and other encodings>
  @param[in] AsciiChar    The character to be tested.
  
  @retval TRUE   The character is a valid start character.
  @retval FALSE  The character is not a valid start character.
**/
BOOLEAN
IsAsciiNameStartChar (
  IN CHAR8 AsciiChar    
  )
{
  if (AsciiChar >= 'A' && AsciiChar <= 'Z') {
      return TRUE;
  }
  if (AsciiChar >= 'a' && AsciiChar <= 'z') {
      return TRUE;
  }
  if (AsciiChar == '_' || AsciiChar == ':') {
      return TRUE;
  }
  return FALSE;
}

/**
  Check if a character is a valid name character as per the  XML spec. 
  Spec says the definition is:
  NameChar     ::=    NameStartChar | "-" | "." | [0-9] | <a bunch for UTF16 and other encodings>
  
  @param[in] Character  The character to test..
  
  @retval TRUE   The character is valid in a name.
  @retval FALSE  The character is not valid in a name.
**/
BOOLEAN
IsAsciiNameChar (
  IN CHAR8 Character      
  )
{
  if (IsAsciiNameStartChar (Character)) {
    return TRUE;
  }
  if (Character >= '0' && Character <= '9') {
    return TRUE;
  }
  if (Character == '-' || Character == '.') {
    return TRUE;  
  }
  return FALSE;
}

/**
  Check if a character is a valid according the XML spec. 
  This library only supports ASCII so the unicode won't be considered valid. 
  When/if unicode support is added, the specific handler will need to make sure to fully implement this

  Spec says the definition is:
  Char     ::=    #x9 | #xA | #xD | [#x20-#xD7FF])
  
  @param[in] Character    The character to be tested.
  
  @retval TRUE            The character is a valid XML character.
  @retval FALSE           The character is not a valid XML character.
**/
BOOLEAN
IsAsciiXmlChar (
  IN CHAR8 Character      
  )
{
  if (Character == '\t' 
    || Character == '\n' 
    || Character == '\r') 
  {
    return TRUE;  
  }
  //
  // check the range of printable characters based on the spec
  //
  if (Character >= ' ' && Character <= '~') {
    return TRUE;  
  }  
  return FALSE;
}

/**
  Check if the substring begins with the right characters to be an XML tag.

  Spec says the definition is:
  STag     ::=    '<' Name (S Attribute)* S? '>'
  
  @param[in] XmlString    The string to be tested.
  
  @retval TRUE            The string starts appropriately for an element.
  @retval FALSE           The string does not start appropriately for an element.
**/

BOOLEAN
IsAsciiXmlTag (
  IN CHAR8* XmlString
  )
{
  //
  // <a> would be a valid tag. 
  // This means 3 chars is the minimum.
  //
  if (!IsStrLenAtLeast (XmlString,3)) {
    return FALSE;
  }
  //
  // All tags must start with a '<'
  //
  if (XmlString[0] != '<') {
    return FALSE;
  }
  //
  // Check if this is a close tag
  //
  if (XmlString[1] == '/') {
    if(!IsAsciiNameStartChar (XmlString[2])) {
      return FALSE;
    } else {
      return TRUE;
    }
  }
  //
  // Check if the first character of the tag name is valid.
  //
  if (!IsAsciiNameStartChar (XmlString[1])) {
    return FALSE;
  }
  return TRUE;
}

/**
  Check if the element is the empty tag meaning it will have no children.
  
  Spec says the definition is:
  EmptyElemTag     ::=    '<' Name (S Attribute)* S? '/>'
    
  @param[in] XmlString  The string to be tested.
  
  @retval TRUE   This is an empty tag.
  @retval FALSE  This is not an empty tag.
**/
BOOLEAN IsAsciiEmptyElementXmlTag (
  IN CHAR8* XmlString
  )
{
  UINTN XmlStrLen;
  
  // 
  // We will need the string length to check the terminating characters of the tag.
  //
  XmlStrLen = AsciiStrLen (XmlString);
  //
  //<a/> is the shortest empty element possible.
  // This means 4 characters is the minimum for this tag type.
  //
  if (XmlStrLen < 4) {
    return FALSE;
  }

  if (XmlString[XmlStrLen - 1] != '>'){
    return FALSE;
  }
  if (XmlString[XmlStrLen - 2] != '/'){
    return FALSE;
  }
  return TRUE;
}

/**
  Check if the string is a close tag 
  Spec says the definition is:
  ETag     ::=    '</' Name S? '>'
  
  @param[in] XmlString    The string to be tested.
  
  @retval TRUE            The string is a close tag.
  @retval FALSE           The string is not a close tag.
**/

BOOLEAN
IsAsciiXmlCloseTag(
  IN CHAR8* XmlString
)
{
  //
  // See if the string is long enough to be a tag.
  //
  if (!IsStrLenAtLeast (XmlString,3)) {
      return FALSE;
  }
  //
  // Doesn't start with the correct character, not a tag.
  //
  if (XmlString[0] != '<') {
    return FALSE;
  }
  //
  // Then see if the rest conforms to the spec
  //
  if (XmlString[1] == '/') {
    if(!IsAsciiNameStartChar(XmlString[2])) {
      return FALSE;
    } else {
      return TRUE;
    }
  }
  return FALSE;
}

/**
  Check a substring to see if it terminates an element 

  @param[in] XmlString    The string to be tested.
  
  @retval TRUE            The string is a close tag.
  @retval FALSE           The string is not a close tag.
**/
BOOLEAN
IsAsciiXmlTagEndStr (
  IN CHAR8* XmlString  
){
  //
  // All XML element MUST end with a '>'
  //
  if (XmlString[0] == '>') {
      return TRUE;
  }
  //
  // Empty elements end '/>' so check both characters
  //
  if (XmlString[0] == '/' 
    && XmlString[1] == '>') 
  {
    return TRUE;
  }
  return FALSE;
}

/**
  Check if a tag has at least one attribute. 
  If the caller provides the optional FirstAttribute, it will be returned
  pointing to the first attribute. 
  
  Spec says the definition is:
  Attribute    ::=    Name Eq AttValue
  
  @param[in] XmlString                    The string to be tested.
  @param[in out optional] FirstAttribute  The optional pointer to the first attribute to return.
  
  @retval TRUE            The tag has attributes.
  @retval FALSE           The tag does not have attributes.
**/
BOOLEAN
IsAsciiXmlTagWithAttributes (
  IN CHAR8*               XmlString,
  IN OUT OPTIONAL CHAR8** FirstAttribute
)
{
  UINTN i;
  UINTN XmlStrLen;
  
  XmlStrLen = AsciiStrLen(XmlString);
  //
  // Make sure we are looking at a tag first.
  //

  if (!IsAsciiXmlTag (XmlString)) {
    return FALSE;
  }

  //
  // Skip over the tag name. 
  // The attribute should start after the whitespace following the tag name.
  //
  i = 0;
  while (i < XmlStrLen) {
    if (IsAsciiWhitespace (XmlString[i])) {
      break;
    }
    i++;
  }
  //
  // We got here either because we ran out of string (no attributes)
  // or because we hit a whitespace character. 
  // Do a length check first.
  //
  if (i >= XmlStrLen) {
    return FALSE;
  }
  //
  // We still have string so we have whitespace to skip over.
  //
  while (i < XmlStrLen) {
    if (!IsAsciiWhitespace (XmlString[i])) {
      break;
    }
    i++;
  }
  //
  // Make sure we didn't just have whitespace between the tag name and the end of the tag.
  //
  if (IsAsciiXmlTagEndStr (&XmlString[i])) {
    return FALSE;
  }
  //
  // Checks passed, this appears to be an attribute (or the tag is malformed).
  // Later attribute extraction will validate the attributes.
  //
  if (FirstAttribute != NULL) {
    *FirstAttribute = &XmlString[i];
  }
  return TRUE;
}

/**
  Check a string to see if it is a processor instruction (PI)
  
  Spec says the definition is:
  PI     ::=    '<?' PITarget (S (Char* - (Char* '?>' Char*)))? '?>'
  
  @param[in] XmlString    The string to be tested.
  
  @retval TRUE            The string is a processing instruction.
  @retval FALSE           The string is not a processing instruction.
**/
BOOLEAN
IsAsciiPI (
  IN CHAR8* XmlString
  )
{
  if (!IsStrLenAtLeast (XmlString,2)) {
    return FALSE;
  }
  //
  // PIs mist start with '<?' so check for that sequence
  //
  if (XmlString[0] != '<') {
    return FALSE;
  }
  if (XmlString[1] != '?') {
    return FALSE;
  }
  return TRUE;
}

/**
  There seem to be a broad category of elements that start '<!' without a good umbrella
  term for them. I'll just call them declarations until I learn of something better.
  
  
  Definitions for the various elements:
    CDATA:
      CDSect     ::=    CDStart CData CDEnd
      CDStart    ::=    '<![CDATA['
      CData    ::=    (Char* - (Char* ']]>' Char*))
      CDEnd    ::=    ']]>'
    DOCTYPE:
      doctypedecl    ::=    '<!DOCTYPE' S Name (S ExternalID)? S? ('[' intSubset ']' S?)? '>'
    ELEMENT:
      elementdecl    ::=    '<!ELEMENT' S Name S contentspec S? '>'
    ATTLIST:
      AttlistDecl    ::=    '<!ATTLIST' S Name AttDef* S? '>'
    INCLUDE:
      includeSect    ::=    '<![' S? 'INCLUDE' S? '[' extSubsetDecl ']]>'
    IGNORE:
      ignoreSect     ::=    '<![' S? 'IGNORE' S? '[' ignoreSectContents* ']]>'
    ENTITY:
      GEDecl     ::=    '<!ENTITY' S Name S EntityDef S? '>'
      PEDecl     ::=    '<!ENTITY' S '%' S Name S PEDef S? '>'
    NOTATION:
      NotationDecl     ::=    '<!NOTATION' S Name S (ExternalID | PublicID) S? '>'
    These all look like they can be extracted as substrings via a common handler.
    
  @param[in] XmlString    The string to be tested.
  
  @retval TRUE            The string is a directive starting with '<!'.
  @retval FALSE           The string is not a banged directive starting with '<!'.
**/
BOOLEAN
IsAsciiDeclaration (
  IN CHAR8 *XmlString
  )
{
  if (!IsStrLenAtLeast (XmlString, 2)) {
    return FALSE;
  }
  //
  // There are a variety of declarations specified in the XML spec, they all start with '<!' 
  //
  if (XmlString[0] != '<') {
    return FALSE;
  }
  if (XmlString[1] != '!') {
    return FALSE;
  }
  //
  // The one exception case here are comments.
  // Those start '<!--'
  // There is a dedicated handler for those so return false if it looks like a comment.
  // 
  if(XmlString[2] == '-') {
    return FALSE;
  }
  return TRUE;
}

/**
  Check if a string is a comment.
  
  Spec says the definition is:
  Comment    ::=    '<!--' ((Char - '-') | ('-' (Char - '-')))* '-->'
  
  @param[in] XmlString  The string to be tested.
  
  @retval TRUE   The string is a comment.
  @retval FALSE  The string is not a comment.
**/
BOOLEAN
IsAsciiComment (
  IN CHAR8 *XmlString
  )
{
  if (!IsStrLenAtLeast (XmlString, 4)) {
    return FALSE;
  }
  //
  // Comments start with the sequence '<!--".
  //
  if (XmlString[0] != '<') {
    return FALSE;
  }
  if (XmlString[1] != '!') {
    return FALSE;
  }
  if(XmlString[2] != '-') {
    return FALSE;
  }
  if(XmlString[3] != '-') {
    return FALSE;
  }
  return TRUE;   
}

/**
  Extract a PI substring from the bulk text of an XML document.

  @param[in out] XmlDoc  A housekeeping data structure for raw XML text. 
                         Pointers in the structure are updated in this call.
  @param[out] XmlString  The XML substring extracted for this element.
                         This is a copy of the string that the caller must free.
  
  @retval EFI_END_OF_FILE  The EOF was reached before the chunk could be extracted.
  @retval EFI_SUCCESS      The chunk was successfully extracted and returned in XmlString.
**/
EFI_STATUS
AsciiExtractPI(
  IN OUT XML_DOCUMENT* XmlDoc,
  OUT CHAR8**          XmlString
)
{
  CHAR8* LocalPtr;
  UINTN  Length;
  CHAR8* EndOfData;
  
  Length = 0;
  
  EndOfData = XmlDoc->XmlDocument + XmlDoc->DocumentSize;
  
  
  //
  // From the XML spec, processing instructions use a ? between the < and > to denote this type of data.
  //
  LocalPtr = XmlDoc->OperationPtr;
  //
  // The "EndOfData - 3" is to ensure we do not access beyond the end of the buffer, 
  // that we have the required number of chars to test.
  // 
  while (LocalPtr < (EndOfData - 3)) {
    Length++;
    LocalPtr++;
    if (LocalPtr[0] == '?' && LocalPtr[1] == '>') {
      //
      // Advance over the PI close
      //
      Length += 2;
      LocalPtr += 2;
      break;
    }
  }
  
  if (LocalPtr > EndOfData) {
      return EFI_END_OF_FILE;
  }
  *XmlString = AllocateZeroPool (Length + 1);
  gBS->CopyMem(
         *XmlString,
         XmlDoc->OperationPtr,
         Length
       );
  XmlDoc->OperationPtr = LocalPtr;
  return EFI_SUCCESS;
}


/**
  Extract a comment from the XML document.
  TODO: Fix this to handle nested comments.
  
  Spec says the definition is:
  Comment    ::=    '<!--' ((Char - '-') | ('-' (Char - '-')))* '-->'

  @param[in out] XmlDoc  A housekeeping data structure for raw XML text. 
                         Pointers in the structure are updated in this call.
  @param[out] XmlString  The XML substring extracted for this element.
                         This is a copy of the string that the caller must free.
  
  @retval EFI_END_OF_FILE  The EOF was reached before the chunk could be extracted.
  @retval EFI_SUCCESS      The chunk was successfully extracted and returned in XmlString.
**/

EFI_STATUS
AsciiExtractComment (
  IN OUT XML_DOCUMENT* XmlDoc,
  OUT CHAR8**          XmlString
  )
{
  CHAR8* LocalPtr;
  UINTN  Length;
  CHAR8* EndOfData;
  
  EndOfData = XmlDoc->XmlDocument + XmlDoc->DocumentSize;
  LocalPtr = XmlDoc->OperationPtr;
  Length = 0;
  //
  // Make sure there is enough data so we don't look beyond the EOF
  //
  while (LocalPtr < (EndOfData - 3)) {
    Length++;
    LocalPtr++;

    //
    // Look for the "--> which ends a comment block
    //
    if (LocalPtr[0] == '-' && LocalPtr[1] == '-' && LocalPtr[2] == '>'){
      Length += 3;
      LocalPtr += 3;
      break;
    }
  }
  if (LocalPtr > EndOfData) {
      return EFI_END_OF_FILE;
  }
  *XmlString = AllocateZeroPool (Length + 1);
  gBS->CopyMem(
         *XmlString,
         XmlDoc->OperationPtr,
         Length
       );
  XmlDoc->OperationPtr = LocalPtr;
  return EFI_SUCCESS;
}


/**
  A few element types end in ']]>' but with different starts. CDATA is one example, the set of conditionals are another.
  This is a generic handler for those cases. It assumes the caller has a valid element and just scans
  for the proper terminating sequence.  

  @param[in out] XmlDoc  A housekeeping data structure for raw XML text. 
                         Pointers in the structure are updated in this call.
  @param[out] XmlString  The XML substring extracted for this element.
                         This is a copy of the string that the caller must free.
  
  @retval EFI_END_OF_FILE  The EOF was reached before the chunk could be extracted.
  @retval EFI_SUCCESS      The chunk was successfully extracted and returned in XmlString.
**/
EFI_STATUS
AsciiExtractBoxedData (
    IN OUT XML_DOCUMENT* XmlDoc,
    OUT CHAR8**          XmlString
)
{
  CHAR8* LocalPtr;
  UINTN  Length;
  CHAR8* EndOfData;
  
  EndOfData = XmlDoc->XmlDocument + XmlDoc->DocumentSize;
  LocalPtr = XmlDoc->OperationPtr;
  Length = 0;
  
  //
  // ensure the buffer is large enough so we don't go beyond the EOF.
  // 
  while (LocalPtr < (EndOfData - 3)) {
    Length++;
    LocalPtr++;

    if (LocalPtr[0] == ']' && LocalPtr[1] == ']' && LocalPtr[2] == '>'){
      Length += 3;
      LocalPtr += 3;
      break;
    }
  }
  if (LocalPtr > EndOfData) {
    return EFI_END_OF_FILE;
  }
  
  *XmlString = AllocateZeroPool (Length + 1);
  gBS->CopyMem(
         *XmlString,
         XmlDoc->OperationPtr,
         Length
       );
  XmlDoc->OperationPtr = LocalPtr;
  return EFI_SUCCESS;
}

/**
  Extract a declaration. These are elements that start '<!' except for comments. 
  See IsAsciiDeclaration for more info.

  @param[in out] XmlDoc  A housekeeping data structure for raw XML text. 
                         Pointers in the structure are updated in this call.
  @param[out] XmlString  The XML substring extracted for this element.
                         This is a copy of the string that the caller must free.
  
  @retval EFI_END_OF_FILE  The EOF was reached before the chunk could be extracted.
  @retval EFI_SUCCESS      The chunk was successfully extracted and returned in XmlString.
**/
EFI_STATUS
AsciiExtractDeclaration (
    IN OUT XML_DOCUMENT* XmlDoc,
    OUT CHAR8**          XmlString
)
{
  CHAR8*     StrStart;
  EFI_STATUS Status;
  CHAR8*     LocalPtr;
  CHAR8*     EndOfData;
  UINTN      Length;
  
  EndOfData = XmlDoc->XmlDocument + XmlDoc->DocumentSize;
  StrStart = XmlDoc->OperationPtr;
  
  //
  // Certain elements that use []s
  //
  if (StrStart[2] == '[') {
    Status = AsciiExtractBoxedData(
               XmlDoc,
               XmlString
             );
    return Status;
  }
  
  //
  // Comments, <!--...-->
  //
  if  (StrStart[2] == '-'){
    Status = AsciiExtractComment (
               XmlDoc, 
               XmlString
             );
    return Status;   
  }
  //
  // Extract data without []s
  //
  LocalPtr = StrStart;
  Length = 0;

  //
  // Prevent reads beyond the EOF
  //
  while (LocalPtr < (EndOfData - 3)) {
    Length++;
    LocalPtr++;
    if (*(LocalPtr) == '>') {
        break;
    }
  }
  
  if (LocalPtr > EndOfData) {
      return EFI_END_OF_FILE;
  }
  
  *XmlString = AllocateZeroPool (Length + 1);
  gBS->CopyMem(
         *XmlString, 
         XmlDoc->OperationPtr, 
         Length
       );
  XmlDoc->OperationPtr = LocalPtr + 1;
  return EFI_SUCCESS;
}

/**
  Extract an XML tag. This covers open tags, close tags, and empty tags.

  @param[in out] XmlDoc  A housekeeping data structure for raw XML text. 
                         Pointers in the structure are updated in this call.
  @param[out] XmlString  The XML substring extracted for this element.
                         This is a copy of the string that the caller must free.
  
  @retval EFI_END_OF_FILE  The EOF was reached before the chunk could be extracted.
  @retval EFI_SUCCESS      The chunk was successfully extracted and returned in XmlString.
**/
EFI_STATUS
AsciiExtractXmlTag (
    IN OUT XML_DOCUMENT* XmlDoc,
    OUT CHAR8**          XmlString
)
{
  CHAR8* LocalPtr;
  UINTN  Length;
  CHAR8* EndOfData;
  
  EndOfData = XmlDoc->XmlDocument + XmlDoc->DocumentSize;
  LocalPtr = XmlDoc->OperationPtr;
  Length = 0;
  
  while (LocalPtr < (EndOfData)) {
    Length++;
    LocalPtr++;
    if (*LocalPtr == '>') {
      Length++;
      LocalPtr++;
      break;
    }
  }
  if (LocalPtr > EndOfData) {
      return EFI_END_OF_FILE;
  }
  *XmlString = AllocateZeroPool (Length + 1);
  gBS->CopyMem(
         *XmlString,
         XmlDoc->OperationPtr,
         Length
       );
  XmlDoc->OperationPtr = LocalPtr;
  return EFI_SUCCESS;
}


/**
  Extract an attribute from a tag. A tag can have n number of attributes, there doesn't appear to be a limit.
  So extract one and return a pointer to the end of the current attribute. 
  The caller may call this function until it returns an error indicating there are no more attributes. 

  @param[in out] InOutStr  A pointer to some part of the tag preceding an attribute.
                           Leading whitespace will be consumed until a character is found.
                           On output, this points to the next part of the tag after the 
                           attribute that was extracted.

  @param[out] Attribute    The The attribute name extracted from the tag.
                           The string is allocated in a new buffer.
  @param[out] AttribData   The The attribute data extracted from the tag.
                           The string is allocated in a new buffer.
  
  @retval EFI_INVALID_PARAMETER  The attribute appears to be malformed.
  @retval EFI_NOT_FOUND          No attribute was found to be extracted.
                                 This may happen if a tag does not have attributes 
                                 or if there is not an attribute after the InOutStr pointer.
  @retval EFI_SUCCESS            The attribute information was successfully extracted and returned in XmlString.
**/
EFI_STATUS
AsciiExtractAttribute (
  IN OUT CHAR8** InOutStr,
  OUT CHAR8**    Attribute,
  OUT CHAR8**    AttribData
  ) 
{
  CHAR8* StrPtr;
  CHAR8* StrPtr2;
  UINTN  i;
  UINTN  j;
  UINTN  StrLength;
  CHAR8* InputEnd;
  CHAR8* LocalAttrib;
  CHAR8* LocalAttribData;
  CHAR8  QuoteTypeChar;
  
  StrPtr = *InOutStr;
  StrLength = AsciiStrLen(StrPtr);
  LocalAttrib = NULL;
  LocalAttribData = NULL;
  i=0;
  *Attribute = NULL;
  *AttribData = NULL;
  
  //
  // Consume leading whitespace
  //
  InputEnd = &StrPtr[StrLength];
  while (&StrPtr[i] < InputEnd) {
    if (!IsAsciiWhitespace (StrPtr[i])) break;
    i++;
  }
  
  //
  // Check to see if attribute or close of the tag.
  //
  if (IsAsciiXmlTagEndStr (&StrPtr[i])) {
    *InOutStr = &StrPtr[i];
    return EFI_NOT_FOUND;
  }
  
  // 
  // Possible attribute, mark the start.
  //
  StrPtr = &StrPtr[i];
  i = 0;
  
  //
  // Get the name part of the attribute
  //
  while (&StrPtr[i] < InputEnd) {
    if (StrPtr[i] == '=') {
        break;
    }
    
    //
    // Spec allows for optional whitespace that also indicated the end of a name.
    // This will need to be consumed. 
    //
    if (IsAsciiWhitespace (StrPtr[i])) {
        break;
    }
    i++;
  }
  
  //
  // Copy out the attribute name string so we don't lose it. 
  //
  LocalAttrib = AllocateZeroPool (i + 1);
  gBS->CopyMem(
        LocalAttrib,
        StrPtr,
        i
      );
  StrPtr2 = &StrPtr[i];
  j = 0;
  
  //
  // Consume optional whitespace if it exists
  //
  if (IsAsciiWhitespace(StrPtr2[j])) {
    StrPtr2++;
  }
  //
  // Malformed attribute
  //
  if (StrPtr2[j] != '=') {
    FreePool (LocalAttrib);
    return EFI_INVALID_PARAMETER;
  }
  j++;
  //
  // More whitespace consumption code
  //
  while (IsAsciiWhitespace(StrPtr2[j]) && &StrPtr2[j] < InputEnd) {
    j++;
  }
  
  //
  // Record the start of the attribute data.
  //
  StrPtr2 = &StrPtr2[j];
  j = 0;

  if (StrPtr2[j] != '\"' && StrPtr2[j] != '\'') {
    FreePool (LocalAttrib);
    return EFI_INVALID_PARAMETER;
  }
  
  //
  // An attribute can use either a single or a double quote to enclose data
  // This allows the other character to be used data. 
  // But we need to track what the opener is so we can ignore the other and find the closer.
  //
  QuoteTypeChar = StrPtr2[j];
  StrPtr2++;

  j=0;

  while (&StrPtr2[j] < InputEnd) {
    if (StrPtr2[j] == QuoteTypeChar) {
        break;
    }
    j++;
  }
  if (StrPtr2[j] != QuoteTypeChar){
    FreePool (LocalAttrib);
    return EFI_INVALID_PARAMETER;
  }
  //
  // It's possible to have no data
  //
  if (j > 0) {
    LocalAttribData = AllocateZeroPool(j + 1);
    gBS->CopyMem(
           LocalAttribData,
           StrPtr2,
           j
         );
    *AttribData = LocalAttribData;
  } else {
    LocalAttribData = NULL;
  }
  *Attribute = LocalAttrib;
  //
  // Update the string pointer to be beyond the current attribute.
  //
  *InOutStr = &StrPtr2[j+1];
  return EFI_SUCCESS;
  
}


/**
  This extracts information that is not markup. This is the content part of the XML document.

  @param[in out] XmlDoc     A housekeeping data structure for raw XML text. 
                            Pointers in the structure are updated in this call.
  @param[out]    XmlString  The XML substring extracted for this element.
                            This is a copy of the string that the caller must free.
  
  @retval EFI_END_OF_FILE  The EOF was reached before the chunk could be extracted.
  @retval EFI_SUCCESS      The chunk was successfully extracted and returned in XmlString.
**/
EFI_STATUS
AsciiExtractCharData (
  IN OUT XML_DOCUMENT* XmlDoc,
  OUT CHAR8**          XmlString
  )
{
  CHAR8* LocalPtr;
  UINTN  Length;
  CHAR8* EndOfData;

  
  EndOfData = XmlDoc->XmlDocument + XmlDoc->DocumentSize;
  LocalPtr = XmlDoc->OperationPtr;
  Length = 0;

  while (LocalPtr < EndOfData) {
    Length++;
    LocalPtr++;
    if (*LocalPtr == '<'){
      break;
    }
  }
  *XmlString = AllocateZeroPool (Length + 1);
  gBS->CopyMem(
        *XmlString,
        XmlDoc->OperationPtr,
        Length
      );
  if (LocalPtr > EndOfData) {
    XmlDoc->OperationPtr = EndOfData;
  } else {
    XmlDoc->OperationPtr = LocalPtr;
  }
  return EFI_SUCCESS;
}

/**
  Extract a chunk of XML data whether that is markup or char data. 
  Return that chunk to the caller.


  @param[in out] XmlDoc      A housekeeping data structure for raw XML text. 
                             Pointers in the structure are updated in this call.
  @param[out] XmlString      The XML substring extracted for this element.
                             This is a copy of the string that the caller must free.
  @param[out] ExtractedType  The type of XML data that was extracted.
  
  @retval EFI_END_OF_FILE  The EOF was reached before the chunk could be extracted.
  @retval EFI_SUCCESS      The chunk was successfully extracted and returned in XmlString.
**/
EFI_STATUS
AsciiExtractMarkupOrText(
  IN OUT XML_DOCUMENT*  XmlDoc,
  OUT CHAR8**           XmlString,
  OUT XML_DATA_TYPE*    ExtractedType 
  )
{
  UINTN      Length;
  CHAR8*     StrStart;
  CHAR8*     LocalPtr;
  CHAR8*     EndOfData;
  EFI_STATUS Status;
  
  EndOfData = XmlDoc->XmlDocument + XmlDoc->DocumentSize;
  Length = 0;
  
  *ExtractedType = XmlNothing;
  StrStart = XmlDoc->OperationPtr; 
  if (StrStart > EndOfData) {
    return EFI_END_OF_FILE;
  }
  
  //
  // advance over leading whitespace
  //
  while (IsAsciiWhitespace (*StrStart)) {
    StrStart++;
    if (StrStart > EndOfData) {
      return EFI_END_OF_FILE;
    }
  }
  
  LocalPtr = StrStart;
  //
  // Check to see if this is markup
  //
  if (*StrStart == '<') {
    //
    // Update the stream pointer to reflect consuming the whitespace.
    // Do it here because char data can have leading whitespace
    //
    XmlDoc->OperationPtr = StrStart;
    
    // 
    // Is the string long enough to support being a element?
    //
    if (EndOfData - StrStart < 4) {
      //
      // Malformed XML
      //
      return EFI_DEVICE_ERROR;
    }
    
    if (*ExtractedType == XmlNothing && IsAsciiComment (StrStart)){
      Status = AsciiExtractComment(
                XmlDoc,
                XmlString
              );
      *ExtractedType = XmlComment;
    }
    
    if (*ExtractedType == XmlNothing && IsAsciiPI (StrStart)){
      Status = AsciiExtractPI(
                 XmlDoc,
                 XmlString
               );
      *ExtractedType = XmlPi;
    }
    
    if (*ExtractedType == XmlNothing && IsAsciiDeclaration (StrStart)){
        Status = AsciiExtractDeclaration(
                   XmlDoc,
                   XmlString
                 );
        *ExtractedType = XmlDecl;
    }

    if (*ExtractedType == XmlNothing && IsAsciiXmlTag (StrStart)){
        Status = AsciiExtractXmlTag(
                   XmlDoc,
                   XmlString
                   );
        if (IsAsciiXmlCloseTag (*XmlString)) {
          *ExtractedType = XmlCloseTag;
        } else if (IsAsciiEmptyElementXmlTag (*XmlString)){
          *ExtractedType = XmlEmptyTag;
        } else {
            *ExtractedType = XmlTag;
        }
    }
    
    //
    // Assume character data but we should never get here
    //
    if (*ExtractedType == XmlNothing){
      *ExtractedType = XmlNothing;
      Status = AsciiExtractCharData(
                 XmlDoc,
                 XmlString
               );
    }
    
    //
    // Malformed XML
    //
    if ((XmlDoc->OperationPtr) > EndOfData) {
      XmlDoc->OperationPtr = EndOfData;
      DEBUG ((DEBUG_ERROR, "%a: End of data\n", __FUNCTION__));
      return EFI_END_OF_FILE;
    }
    //
    // End of char data. 
    // This could occur outside of a set of tags so we don't know if we have an unmatched tag yet.
    // Code further up should check that.
    //
    return EFI_SUCCESS;
  } 
  while (LocalPtr < EndOfData) {
    Length++;
    LocalPtr++;
    if (*(LocalPtr) == '<') break;
  }
  //
  // We know we didn't have whitespace.
  // so assume we have char data.
  //
  *ExtractedType = XmlChar;
  Status = AsciiExtractCharData (
             XmlDoc,
             XmlString
           );
  return EFI_SUCCESS;
}

/**
  Extract the tag name from a raw tag substring.

  @param[in] TagData          A pointer to the raw chunk that was extracted
  @param[in out] ElementName  A pointer to return the newly allocated buffer to the caller.
    
  @retval EFI_INVALID_PARAMETER  There was an invalid character in the chunk.
  @retval EFI_SUCCESS            The chunk was successfully extracted and returned in XmlString.
**/
EFI_STATUS
AsciiGetTagNameFromElement (
  CHAR8* TagData,
  CHAR8** ElementName
  )
{
  CHAR8* StrPtr;
  UINTN ElementLength;
  UINTN TagLen;
  
  if (TagData[0] != '<') {
    return EFI_INVALID_PARAMETER;
  }
  
  ElementLength = 0;
  TagLen = AsciiStrLen (TagData);
  StrPtr = TagData;
  StrPtr++;
  //
  // if whitespace, it needs to be skipped, advance until not whitespace
  //
  if (StrPtr[0] == '/' ) StrPtr++;
  if (IsAsciiNameStartChar(StrPtr[0])){
    ElementLength++;
    while (!IsAsciiWhitespace (StrPtr[ElementLength]) && 
        IsAsciiNameChar (StrPtr[ElementLength]) && 
        &StrPtr[ElementLength] < &TagData[TagLen]) 
    {
        ElementLength++;
    }
    //
    // We should have the element name at this point.
    // But we may have hit a bad character so we need to see if we ended on whitespace or not
    //
    if (!IsAsciiWhitespace (StrPtr[ElementLength]) && 
      !IsAsciiXmlTagEndStr (&StrPtr[ElementLength])) 
    {
      DEBUG ((DEBUG_ERROR, "Encountered and invalid character 0x%x\n", StrPtr[ElementLength]));
      return EFI_INVALID_PARAMETER;
    }
    *ElementName = AllocateZeroPool (ElementLength + 1);
    gBS->CopyMem(
           *ElementName, 
           StrPtr, 
           ElementLength
         );
    return EFI_SUCCESS;
  } else {
    //
    //invalid XML character
    //
    return EFI_INVALID_PARAMETER;
  }
  //
  // For the compiler
  //
  return EFI_SUCCESS;
}

/**
  Extract the processor instruction target and data from a PI

  @param[in] PiData            A pointer to the raw chunk that was extracted
  @param[in out] PiTargetName  A pointer to return the PT target name in a newly allocated buffer.
  @param[in out] PiTargetData  A pointer to return the PT data for the PCI target in a newly allocated buffer. 
    
  @retval EFI_INVALID_PARAMETER  There was an invalid character in the chunk.
  @retval EFI_SUCCESS            The chunk was successfully extracted and returned in XmlString.
**/

EFI_STATUS
AsciiGetPIData (
  IN CHAR8*   PiData,
  OUT CHAR8** PiTargetName,
  OUT CHAR8** PiTargetData
  )
{
  CHAR8* StrPtr;
  CHAR8* InputEnd;
  UINTN  PiTargetLength;
  UINTN  PiDataLength;
  UINTN  PiLen;
  UINTN  i;
  
  PiTargetLength = 0;
  PiDataLength = 0;
  PiLen = AsciiStrLen (PiData);
  i = 0;
  
  if (PiData[0] != '<' || PiData[1] != '?') return EFI_INVALID_PARAMETER;
  InputEnd = &PiData[PiLen];
  StrPtr = &PiData[2];
  if (IsAsciiNameStartChar(StrPtr[0])){
    PiTargetLength++;
    while (!IsAsciiWhitespace(StrPtr[PiTargetLength]) && 
        IsAsciiNameChar(StrPtr[PiTargetLength]) && 
        &StrPtr[PiTargetLength] < &PiData[PiLen]) 
    {
      PiTargetLength++;
    }
    if (!IsAsciiWhitespace(StrPtr[PiTargetLength]) && !IsAsciiXmlTagEndStr(&StrPtr[PiTargetLength])) {
      DEBUG((DEBUG_ERROR,"Encountered and invalid character 0x%x\n",StrPtr[PiTargetLength]));
      return EFI_INVALID_PARAMETER;
    }
    *PiTargetName = AllocateZeroPool(PiTargetLength + 1);
    gBS->CopyMem(*PiTargetName,StrPtr,PiTargetLength);
  } else {
    //
    // Invalid XML character
    //
    return EFI_INVALID_PARAMETER;
  }
  //
  // Now get the PI data
  //
  StrPtr += PiTargetLength;
  i = 0;
  
  
  *PiTargetData = NULL;
  //
  // XML spec says all characters are available as data and has no 
  // requirements for mark up. 
  // Treat all chars after the white space until the closing ?> as data
  //
  while (&StrPtr[i] < InputEnd) {
    if (!IsAsciiWhitespace(StrPtr[i])) {
        break;
    }
    i++;
  }
  //
  // See if we hit the end of the markup without getting data.
  //
  StrPtr = &StrPtr[i];
  if (StrPtr[0] == '?' && StrPtr[1] == '>') {
    //
    // Nothing there, just return NULL.
    // 
    DEBUG((DEBUG_ERROR, "Failed to get PI target data\n"));
    return EFI_SUCCESS;
  }
  PiDataLength=0;
  //
  //extract the data.
  //
  while (IsAsciiXmlChar(StrPtr[PiDataLength])){
    if (StrPtr[PiDataLength] == '?' && StrPtr[PiDataLength+1] == '>') {
      break;
    }
    PiDataLength++;
  }
  if(PiDataLength >= 1){

    *PiTargetData = AllocateZeroPool(PiDataLength + 1);
    gBS->CopyMem(*PiTargetData, StrPtr, PiDataLength);
  }
  return EFI_SUCCESS;
}
