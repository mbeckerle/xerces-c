/*
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The ASF licenses this file to You under the Apache License, Version 2.0
 * (the "License"); you may not use this file except in compliance with
 * the License.  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * $Id$
 */


// ---------------------------------------------------------------------------
//  Includes
// ---------------------------------------------------------------------------
#include "SAX2Count.hpp"
#include <xercesc/util/PlatformUtils.hpp>
#include <xercesc/sax2/SAX2XMLReader.hpp>
#include <xercesc/sax2/XMLReaderFactory.hpp>
#if defined(XERCES_NEW_IOSTREAMS)
#include <fstream>
#else
#include <fstream.h>
#endif
#include <xercesc/util/OutOfMemoryException.hpp>

#include <iostream>
#include <fstream>
#include <string>
#include <cstring>
#include <cstddef>
#include <regex>
#include <sstream>

// ---------------------------------------------------------------------------
//  Local helper methods
// ---------------------------------------------------------------------------
void usage()
{
    XERCES_STD_QUALIFIER cout << "\nUsage:\n"
            "    SAX2Count [options] <XML file | List file>\n\n"
            "This program invokes the SAX2XMLReader, and then prints the\n"
            "number of elements, attributes, spaces and characters found\n"
            "in each XML file, using SAX2 API.\n\n"
            "Options:\n"
            "    -l          Indicate the input file is a List File that has a list of xml files.\n"
            "                Default to off (Input file is an XML file).\n"
            "    -v=xxx      Validation scheme [always | never | auto*].\n"
            "    -f          Disable full schema constraint checking processing. Defaults to enabled.\n"
            "    -d          When schema validation is on, skip DTD. Default is to skip DTD validation if schema validation is on.\n"
            "    -p          Disable namespace-prefixes feature. Defaults to enabled.\n"
            "    -n          Disable namespace processing. Defaults to enabled.\n"
            "                NOTE: THIS IS OPPOSITE FROM OTHER SAMPLES.\n"
            "    -s          Disable schema processing. Defaults to enabled.\n"
            "                NOTE: THIS IS OPPOSITE FROM OTHER SAMPLES.\n"
            "    -i          Disable identity constraint checking. Defaults to enabled.\n"
            "                NOTE: THIS IS OPPOSITE FROM OTHER SAMPLES.\n"
            "    -locale=ll_CC specify the locale, default: en_US.\n"
            "    -noNameSpaceSchema=schemafile specify a no-namespace schema file. Implies -v=always.\n"
            "    -schema=schemafile specify a schema file. Implies -v=always.\n"
            "    -?          Show this help.\n\n"
            "  * = Default if not provided explicitly.\n"
         << XERCES_STD_QUALIFIER endl;
}

/**
 * Gets the Target Namespace URI from an XML Schema (XSD) file.
 * 
 * Does this by string/regex hacking. Not using an XML parser.
 * Hence, this can be fooled by targetNamespace declaration being
 * commented out, or being spread across lines, or containing quoted
 * characters that would fool the regex. (quotation marks)
 * Or containing whitespace, even if escaped.
 *
 * Also this is assuming the charset of the XSD file is ascii-based as
 * in ascii or utf-8. If it's UTF-16 this will fail.
*/
const char* getTargetNamespaceURIFromXMLSchemaFile(const char* xsdFile) {
    const char* result;
    std::ifstream file(xsdFile); 
    std::string line;

    if (!file.is_open()) {
        std::cerr << "Could not open the file '" << xsdFile << "'." << std::endl;
        return nullptr;
    }

    // it is group 2 of this pattern which is 1 to 80 non whitespace chars
    // that we want. 
    std::regex pattern("\\s*targetNamespace=(['\"])(\\S{1,80})\\1\\s?");
    std::smatch match;
    while (std::getline(file, line)) {
        if (std::regex_search(line, match, pattern)) {
            // for (size_t i = 0; i < match.size(); ++i) {
            //     std::ssub_match sub_match = match[i];
            //     std::string piece = sub_match.str();
            //     std::cout << "Match " << i << ": " << piece << std::endl;
            // }
        result = strdup(match[2].str().c_str());
        break;
        }
    }
    file.close();
    return result;
}

// ---------------------------------------------------------------------------
//  Program entry point
// ---------------------------------------------------------------------------
int main(int argC, char* argV[])
{
    // Check command line and extract arguments.
    if (argC < 2)
    {
        usage();
        return 1;
    }

    const char*                  xmlFile      = 0;
    SAX2XMLReader::ValSchemes    valScheme    = SAX2XMLReader::Val_Auto;
    const char*                  noNameSpaceSchemaFile = 0;
    const char*                  targetSchemaFile = 0;
    bool                         doNamespaces = true;
    bool                         doSchema = true;
    bool                         schemaFullChecking = true; // changed. Makes no sense for this to be off if identityConstraintChecking is on by default
    bool                         identityConstraintChecking = true;
    bool                         doList = false;
    bool                         errorOccurred = false;
    bool                         namespacePrefixes = true; // changed. Makes no sense for this to be off by default.
    bool                         recognizeNEL = false;
    char                         localeStr[64];
    bool                         skipDTD = true;
    memset(localeStr, 0, sizeof localeStr);

    int argInd;
    int sz;

    for (argInd = 1; argInd < argC; argInd++)
    {
        char* arg = argV[argInd];

        // Break out on first parm not starting with a dash
        if (argV[argInd][0] != '-')
            break;
        // Watch for special case help request
        if (!strcmp(argV[argInd], "-?"))
        {
            usage();
            return 2;
        }
         else if (!strncmp(argV[argInd], "-v=", 3)
              ||  !strncmp(argV[argInd], "-V=", 3))
        {
            const char* const parm = &argV[argInd][3];

            if (!strcmp(parm, "never"))
                valScheme = SAX2XMLReader::Val_Never;
            else if (!strcmp(parm, "auto"))
                valScheme = SAX2XMLReader::Val_Auto;
            else if (!strcmp(parm, "always"))
                valScheme = SAX2XMLReader::Val_Always;
            else
            {
                XERCES_STD_QUALIFIER cerr << "Unknown -v= value: " << parm << XERCES_STD_QUALIFIER endl;
                return 2;
            }
        }
         else if (!strcmp(argV[argInd], "-n")
              ||  !strcmp(argV[argInd], "-N"))
        {
            doNamespaces = false;
        }
         else if (!strcmp(argV[argInd], "-s")
              ||  !strcmp(argV[argInd], "-S"))
        {
            doSchema = false;
        }
         else if (!strcmp(argV[argInd], "-d")
              ||  !strcmp(argV[argInd], "-D"))
        {
            skipDTD = true;
        }
         else if (!strcmp(argV[argInd], "-f")
              ||  !strcmp(argV[argInd], "-F"))
        {
            schemaFullChecking = true;
        }
         else if (!strcmp(argV[argInd], "-i")
              ||  !strcmp(argV[argInd], "-I"))
        {
            identityConstraintChecking = false;
        }
         else if (!strcmp(argV[argInd], "-l")
              ||  !strcmp(argV[argInd], "-L"))
        {
            doList = true;
        }
         else if (!strcmp(argV[argInd], "-p")
              ||  !strcmp(argV[argInd], "-P"))
        {
            namespacePrefixes = true;
        }
         else if (!strcmp(argV[argInd], "-special:nel"))
        {
            // turning this on will lead to non-standard compliance behaviour
            // it will recognize the unicode character 0x85 as new line character
            // instead of regular character as specified in XML 1.0
            // do not turn this on unless really necessary
             recognizeNEL = true;
        }
         else if (!strncmp(argV[argInd], "-locale=", 8))
        {
             // Get out the end of line
             strncpy(localeStr, &(argV[argInd][8]), sizeof localeStr);
        }
         else if (!strncmp(arg, "-noNameSpaceSchema=", sz = sizeof("-noNameSpaceSchema=") - 1)) 
        {
             noNameSpaceSchemaFile = arg + sz;
        }
         else if (!strncmp(arg, "-schema=", sz = sizeof("-schema=") - 1))
        {
             targetSchemaFile = arg + sz;
        }
         else
        {
             XERCES_STD_QUALIFIER cerr << "Unknown option '" << arg
                << "', ignoring it\n" << XERCES_STD_QUALIFIER endl;
        }
    }

    // Look for conflicting options
    // -s, -n, and -p cannot be used with -schema or -noNameSpaceSchema
    if (targetSchemaFile != 0 || noNameSpaceSchemaFile != 0) {
        // there is a schema provided so
        // namespace-prefixes must be enabled,
        // namespace processing must be enabled,
        // schema processing must be enabled.
      if (!doSchema ||
          !doNamespaces ||
      !namespacePrefixes) {
            std::cerr << "Options -s, -n, and -p are incompatible with -schema and -noNameSpaceSchema." << std::endl;
            usage();
            return 2;
      }
      if (valScheme == SAX2XMLReader::Val_Never) {
                std::cerr << "Options -v=never is incompatible with -schema and -noNameSpaceSchema." << std::endl;
                usage();
                return 2;
      }
      valScheme = SAX2XMLReader::Val_Always;
    }

    //
    //  There should be only one and only one parameter left, and that
    //  should be the file name.
    //
    if (argInd != argC - 1)
    {
        usage();
        return 1;
    }

    // Initialize the XML4C2 system
    try
    {
        if (strlen(localeStr))
        {
            XMLPlatformUtils::Initialize(localeStr);
        }
        else
        {
            XMLPlatformUtils::Initialize();
        }

        if (recognizeNEL)
        {
            XMLPlatformUtils::recognizeNEL(recognizeNEL);
        }
    }

    catch (const XMLException& toCatch)
    {
        XERCES_STD_QUALIFIER cerr << "Error during initialization! Message:\n"
            << StrX(toCatch.getMessage()) << XERCES_STD_QUALIFIER endl;
        return 1;
    }

    //
    //  Create a SAX parser object. Then, according to what we were told on
    //  the command line, set it to validate or not.
    //
    SAX2XMLReader* parser = XMLReaderFactory::createXMLReader();
    parser->setFeature(XMLUni::fgSAX2CoreNameSpaces, doNamespaces);
    parser->setFeature(XMLUni::fgXercesSchema, doSchema);
    parser->setFeature(XMLUni::fgXercesHandleMultipleImports, true);
    parser->setFeature(XMLUni::fgXercesSchemaFullChecking, schemaFullChecking);
    parser->setFeature(XMLUni::fgXercesIdentityConstraintChecking, identityConstraintChecking);
    parser->setFeature(XMLUni::fgSAX2CoreNameSpacePrefixes, namespacePrefixes);
    parser->setFeature(XMLUni::fgXercesSkipDTDValidation, skipDTD);
    if (skipDTD) {
        parser->setProperty(XMLUni::fgXercesScannerName, (void *)XMLUni::fgSGXMLScanner);
    }
    else {
        parser->setProperty(XMLUni::fgXercesScannerName, (void *)XMLUni::fgIGXMLScanner);
    }

    if (valScheme == SAX2XMLReader::Val_Auto)
    {
        parser->setFeature(XMLUni::fgSAX2CoreValidation, true);
        parser->setFeature(XMLUni::fgXercesDynamic, true);
    }
    if (valScheme == SAX2XMLReader::Val_Never)
    {
        parser->setFeature(XMLUni::fgSAX2CoreValidation, false);
    }
    if (valScheme == SAX2XMLReader::Val_Always)
    {
        parser->setFeature(XMLUni::fgSAX2CoreValidation, true);
        parser->setFeature(XMLUni::fgXercesDynamic, false);
    }
    if (noNameSpaceSchemaFile != NULL) {
        /* Need to set the schema for no-namespace */
        XMLCh *schemaPath = XMLString::transcode(noNameSpaceSchemaFile);
        parser->setProperty(
                XMLUni::fgXercesSchemaExternalNoNameSpaceSchemaLocation,
                schemaPath);
        XMLString::release(&schemaPath);
    }
    else if (targetSchemaFile != NULL) {
        //
        // This is rather painful. 
        //
        // Xerces C simply doesn't support the behavior we want, which is 
        // to stipulate the schema file which MUST MATCH the namespace 
        // and ROOT element of the schema file being parsed.
        //
        // It just doesn't do that. 
        //
        // Rather, the string we pass for the schema location
        // is a alternating list of namespaceURI and fileLocation strings.
        // This is just like with xsi:schemaLocation if it was in the instance document. 
        //
        // The problem is that we don't know (nor care) about the namespace. It
        // either works, or doesn't.
        //
        // But we can't even ask Xerces C to do the parse and validate unless
        // we know the URI of the targetNamespace. 
        // So we have to get it from the 
        // targetNamespace attributes of the xs:schema of the schema file, or
        // from the namespace of the root element of the instance document
        // 
        // We're going to get it by string hacking of the XML schema looking for 
        // the targetNamespace declaration. 
        //
        // This is fallable, because it's not doing an XML parse of the schema 
        // so a commented out targetNamespace declaration, or one that spans lines
        // would fool our logic.
        //
        // It's actually worse than that, as we're just assuming that opening the
        // XSD in the default charset it going to let us search for the targetNamespace
        // declaration. This will work in ascii-derived charsets like UTF-8, but will NOT
        // work for UTF-16. 
        // 
        std::ostringstream oss;
        const char* uri = getTargetNamespaceURIFromXMLSchemaFile(targetSchemaFile);
        oss << uri << " " << targetSchemaFile;
        const char* schemaLocationPairs = strdup(oss.str().c_str());
        parser->setProperty(
                XMLUni::fgXercesSchemaExternalSchemaLocation,
                XMLString::transcode(schemaLocationPairs));
    }
    //
    //  Create our SAX handler object and install it on the parser, as the
    //  document and error handler.
    //
    SAX2CountHandlers handler;
    parser->setContentHandler(&handler);
    parser->setErrorHandler(&handler);
    parser->setLexicalHandler(&handler);

    //
    //  Get the starting time and kick off the parse of the indicated
    //  file. Catch any exceptions that might propogate out of it.
    //
    unsigned long duration;

    bool more = true;
    XERCES_STD_QUALIFIER ifstream fin;

    // the input is a list file
    if (doList)
        fin.open(argV[argInd]);

    if (fin.fail()) {
        XERCES_STD_QUALIFIER cerr <<"Cannot open the list file: " << argV[argInd] << XERCES_STD_QUALIFIER endl;
        return 2;
    }

    while (more)
    {
        char fURI[1000];
        //initialize the array to zeros
        memset(fURI,0,sizeof(fURI));

        if (doList) {
            if (! fin.eof() ) {
                fin.getline (fURI, sizeof(fURI));
                if (!*fURI)
                    continue;
                else {
                    xmlFile = fURI;
                    XERCES_STD_QUALIFIER cerr << "==Parsing== " << xmlFile << XERCES_STD_QUALIFIER endl;
                }
            }
            else
                break;
        }
        else {
            xmlFile = argV[argInd];
            more = false;
        }

        //reset error count first
        handler.resetErrors();

        try
        {
            const unsigned long startMillis = XMLPlatformUtils::getCurrentMillis();
            parser->parse(xmlFile);
            if (handler.getWithDTD()) {
                parser->setProperty(XMLUni::fgXercesScannerName, (void *)XMLUni::fgDGXMLScanner);
                parser->parse(xmlFile);
            }
            // XERCES_STD_QUALIFIER cout << "withDTD: " << handler.getWithDTD() << std::endl;

            const unsigned long endMillis = XMLPlatformUtils::getCurrentMillis();
            duration = endMillis - startMillis;
        }
        catch (const OutOfMemoryException&)
        {
            XERCES_STD_QUALIFIER cerr << "OutOfMemoryException" << XERCES_STD_QUALIFIER endl;
            errorOccurred = true;
            continue;
        }
        catch (const XMLException& e)
        {
            XERCES_STD_QUALIFIER cerr << "\nError during parsing: '" << xmlFile << "'\n"
                << "Exception message is:  \n"
                << StrX(e.getMessage()) << "\n" << XERCES_STD_QUALIFIER endl;
            errorOccurred = true;
            continue;
        }

        catch (...)
        {
            XERCES_STD_QUALIFIER cerr << "\nUnexpected exception during parsing: '" << xmlFile << "'\n";
            errorOccurred = true;
            continue;
        }


        // Print out the stats that we collected and time taken
        if (!handler.getSawErrors())
        {
            XERCES_STD_QUALIFIER cout << xmlFile << ": " << duration << " ms ("
                << handler.getElementCount() << " elems, "
                << handler.getAttrCount() << " attrs, "
                << handler.getSpaceCount() << " spaces, "
                << handler.getCharacterCount() << " chars)" << XERCES_STD_QUALIFIER endl;
        }
        else
            errorOccurred = true;
    }

    if (doList)
        fin.close();

    //
    //  Delete the parser itself.  Must be done prior to calling Terminate, below.
    //
    delete parser;

    // And call the termination method
    XMLPlatformUtils::Terminate();

    if (errorOccurred)
        return 4;
    else
        return 0;

}


