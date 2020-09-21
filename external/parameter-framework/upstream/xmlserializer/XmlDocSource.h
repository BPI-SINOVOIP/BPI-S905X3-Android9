/*
 * Copyright (c) 2011-2015, Intel Corporation
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation and/or
 * other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once
#include "XmlElement.h"
#include "XmlSerializingContext.h"

#include "NonCopyable.hpp"

#include <string>

struct _xmlDoc;
struct _xmlNode;
struct _xmlError;

/**
  * The CXmlDocSource is used by CXmlDocSink.
  * The interaction between the xml source and xml sink is defined
  * in the process method of CXmlDocSink. One can subclass CXmlDocSource
  * for different purposes by implementing the populate method and then
  * use it with any existing implementation of CXmlDocSink.
  */
class CXmlDocSource : private utility::NonCopyable
{
public:
    /**
      * Constructor
      *
      * @param[out] pDoc a pointer to the xml document that will be filled by the class
      * @param[in] pRootNode a pointer to the root element of the document.
      * @param[in] bValidateWithSchema a boolean that toggles schema validation
      */
    CXmlDocSource(_xmlDoc *pDoc, bool bValidateWithSchema = false, _xmlNode *pRootNode = NULL);

    /**
      * Constructor
      *
      * @param[out] pDoc a pointer to the xml document that will be filled by the class
      * @param[in] strRootElementType a string containing the root element type
      * @param[in] strRootElementName a string containing the root element name
      * @param[in] strNameAttributeName a string containing the name of the root name attribute
      * @param[in] bValidateWithSchema a boolean that toggles schema validation
      */
    CXmlDocSource(_xmlDoc *pDoc, bool bValidateWithSchema,
                  const std::string &strRootElementType = "",
                  const std::string &strRootElementName = "",
                  const std::string &strNameAttributeName = "");

    /**
      * Destructor
      */
    virtual ~CXmlDocSource();

    /**
      * Method called by the CXmlDocSink::process method.
      *
      * @param[out] serializingContext is used as error output
      *
      * @return false if there are any error
      */
    virtual bool populate(CXmlSerializingContext &serializingContext);

    /**
      * Method that returns the root element of the Xml tree.
      *
      * @param[out] xmlRootElement a reference to the CXmleElement destination
      */
    void getRootElement(CXmlElement &xmlRootElement) const;

    /**
      * Getter method.
      *
      * @return the root element's name
      */
    std::string getRootElementName() const;

    /** Get the Schemas' base (folder) URI
     */
    std::string getSchemaBaseUri();

    /** Set the Schema's base (folder) URI
     *
     * The schema for validating the XML document will be searched for in that
     * folder.
     *
     * @param[in] uri The Schemas' base URI
     */
    void setSchemaBaseUri(const std::string &uri);

    /**
      * Getter method.
      * Method that returns the root element's attribute with name matching strAttributeName.
      *
      * @param[in] strAttributeName is a std::string used to find the corresponding attribute
      *
      * @return the value of the root's attribute named as strAttributeName
      */
    std::string getRootElementAttributeString(const std::string &strAttributeName) const;

    /**
      * Getter method.
      * Method that returns the xmlDoc contained in the Source.
      * (Can be used in a Doc Sink)
      *
      * @return the document _pDoc
      */
    _xmlDoc *getDoc() const;

    /**
    * Method that checks that the xml document has been correctly parsed.
    *
    * @return false if any error occurs during the parsing
    */
    virtual bool isParsable() const;

    /**
     * Helper method to build final URI from base and relative uri/path
     *
     * base = "/path/to/file.xml"
     *  - uri                   - returned value
     *  .                       /path/to/
     *  file.xsd                /path/to/file.xsd
     *  ../from/file.xsd        /path/from/file.xsd
     *  /path2/file.xsd         /path2/file.xsd
     *
     * @param[in] base uri, if base is directory, it must contains separator last
     * @param[in] relative uri
    *
    * @return new made URI if succeeded relative input otherwise
     */
    static std::string mkUri(const std::string &base, const std::string &relative);

    /**
     * Helper method for creating an xml document from either a file or a
     * string.
     *
     * @param[in] source either a filename or a string representing an xml document
     * @param[in] fromFile true if source is a filename, false if source is an xml
     *            represents an xml document
     * @param[in] xincludes if true, process xincludes tags
     * @param[in] serializingContext will receive any serialization error
     */
    static _xmlDoc *mkXmlDoc(const std::string &source, bool fromFile, bool xincludes,
                             CXmlSerializingContext &serializingContext);

protected:
    /**
      * Doc
      */
    _xmlDoc *_pDoc;

    /**
      * Root node
      */
    _xmlNode *_pRootNode;

private:
    /** Method that check the validity of the document with the xsd file.
      *
      * @return true if document is valid, false if any error occures
      */
    bool isInstanceDocumentValid();
    std::string getSchemaUri() const;

    /**
      * Element type info
      */
    std::string _strRootElementType;

    /**
      * Element name info
      */
    std::string _strRootElementName;

    /**
      * Element name attribute info
      */
    std::string _strNameAttributeName;

    /**
      * Boolean that enables the validation via xsd files
      */
    bool _bValidateWithSchema;

    std::string _schemaBaseUri;
};
