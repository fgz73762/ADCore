/* NDFileNexus.cpp
 *  Write NDArrays to TIFF files
 *
 *  Written by John Hammonds
 *    July 7, 2009
 *    Adapted from earlier NDPluginNexus from Brian Tieman.  Now take advantage of the fact that NDPluginFile is now a
 *    superclass that can handle much of the lowlying details of accumulating images.  Write only the open/read/write/close
 *    methods.  Try to use the C++ library from Nexus organisation.  Use the tiny XML parser to read the config file.  On
 *    capture allow the data to be written in chunks/slabs to conserve memory.
 */

#include <stdio.h>

#include <epicsString.h>
#include <epicsExport.h>
#include <iocsh.h>
#include <tinyxml.h>
#include <napi.h>
#include <string.h>

#include "asynNDArrayDriver.h"
#include "NDFileNexus.h"


/** Call to open the file */
asynStatus NDFileNexus::openFile( const char *fileName, NDFileOpenMode_t openMode, NDArray *pArray) {
	int status = asynSuccess;
	int addr = 0;
	int ii;
	NDAttribute *currAttr;
	char fullFilename[2*MAX_FILENAME_LEN];
	char template_path[MAX_FILENAME_LEN],
	     template_file[MAX_FILENAME_LEN];
//	char attrFileName[2*MAX_FILENAME_LEN];
#define MAX_ATTR_NAME_LEN 256
	//char attrName[MAX_ATTR_NAME_LEN];
	char programName[] = "areaDetector NDFileNexus plugin v0.1";

	/* get the filename to be used for nexus template */
	status = getStringParam(addr, NDFileNexusTemplatePath, sizeof(template_path), template_path);
	status = getStringParam(addr, NDFileNexusTemplateFile, sizeof(template_file), template_file);
//	status = getStringParam(addr, PVAttributesFile, sizeof(attrFileName), attrFileName);
	printf ("Template path: %s\nTemplateFile:%s\n", template_path, template_file);
	sprintf(fullFilename, "%s%s", template_path, template_file);
	printf("full name %s\n", fullFilename);
//	printf("attribute file name %s\n", attrFileName);
	/* Load the Nexus template file */
	this->configDoc.LoadFile(fullFilename);
	this->rootNode = this->configDoc.RootElement();
	//printf("RootNode %s\n", this->rootNode->Value());
	//pArray->report(5);
	printf("Num attributes in NDArray %d\n", pArray->numAttributes());
	currAttr = pArray->nextAttribute(NULL);
	ii = 0;


	/* Open the NeXus file */
	printf ("Opening File %s\n", fileName);
	NXopen(fileName, NXACC_CREATE5, &nxFileHandle);
	NXputattr( this->nxFileHandle, "creator", programName, strlen(programName), NX_CHAR);
	this->pPVAttributeList->getValues(pArray);
	//this->pPVAttributeList->report(5);
	return (asynSuccess);
	}

/** Call to write NDArrays to file */
asynStatus NDFileNexus::writeFile(NDArray *pArray) {
	printf ("Writing image to file\n");
	//getAttributes(pArray);
	processNode(this->rootNode, pArray);

	return (asynSuccess);
	}

/** call to read the file.  Not implementes.  This is just a dummy routine. */
asynStatus NDFileNexus::readFile(NDArray **pArray) {
	printf ("Reading image not implemented\n");
	return asynError;
	}

/** call to close the file after we are done */
asynStatus NDFileNexus::closeFile() {
	printf ("Closing Nexus file \n");

	/* close the nexus file */
	NXclose(&nxFileHandle);

	return asynSuccess;
	}

int NDFileNexus::processNode(TiXmlNode *curNode, NDArray *pArray) {
	int status = 0;
	char nodeName[256];
	char nodeValue[256];
	char nodeOuttype[40];
	char nodeSource[128];
	char nodeType[40];
	//float data;
	int numpts;
	int rank;
	NDDataType_t type;
	int ii;
	int dims[ND_ARRAY_MAX_DIMS];
	NDAttrDataType_t attrDataType;
	NDAttribute *pAttr;
	size_t attrDataSize;
	size_t nodeTextLen;
	int wordSize;
	int dataOutType;
	int numWords;
	char *pString;
	void *pValue;
	char nodeText[256];
	TiXmlNode *childNode;
	numpts = 1;
	sprintf(nodeValue, "%s", curNode->Value());
	sprintf(nodeType, "%s", curNode->ToElement()->Attribute("type"));
//	printf("%s %d\n", nodeValue, curNode->Type());
	childNode = 0;
	NXstatus stat;
	if (strcmp (nodeValue, "NXroot") == 0) {
		this->iterateNodes(curNode, pArray);
	}
	else if ((strcmp (nodeValue, "NXentry") ==0) ||
	         (strcmp (nodeValue, "NXinstrument") ==0) ||
	         (strcmp (nodeValue, "NXsample") ==0) ||
	         (strcmp (nodeValue, "NXmonitor") ==0) ||
	         (strcmp (nodeValue, "NXsource") ==0) ||
	         (strcmp (nodeValue, "NXuser") ==0) ||
	         (strcmp (nodeValue, "NXdata") ==0) ||
	         (strcmp (nodeType, "UserGroup") ==0) ) {
		sprintf(nodeName, "%s",  curNode->ToElement()->Attribute("name"));
		if (strcmp(nodeName, "(null)") == 0 ) {
			sprintf(nodeName, "%s", nodeValue);
		}
		stat = NXmakegroup(this->nxFileHandle, (const char *)nodeName, (const char *)nodeValue);
		stat |= NXopengroup(this->nxFileHandle, (const char *)nodeName, (const char *)nodeValue);
		if (stat != NX_OK ) printf("Error creating group %s\n", nodeName, nodeValue);
		this->iterateNodes(curNode, pArray);
		stat = NXclosegroup(this->nxFileHandle);
		if (stat != NX_OK ) printf("Error closing group %s\n", nodeName, nodeValue);


	}
	else if (strcmp (nodeValue, "Attr") ==0) {
		sprintf(nodeName, "%s",  curNode->ToElement()->Attribute("name"));
		sprintf(nodeSource, "%s", curNode->ToElement()->Attribute("source"));
		if ( strcmp(nodeType, "ND_ATTR") == 0 ) {
			pAttr = pArray->findAttribute(nodeSource);
			pAttr->getValueInfo(&attrDataType, &attrDataSize);
			this->getAttrTypeNSize(pAttr, &dataOutType, &wordSize);

			if (dataOutType > 0) {
				pValue = calloc( attrDataSize, wordSize );
				pString = (char *)pValue;
				pAttr->getValue(attrDataType, (char *)pValue, attrDataSize*wordSize);

				NXputattr(this->nxFileHandle, nodeName, pValue, attrDataSize*wordSize, dataOutType);
				free(pValue);
			}
		}
		if ( strcmp(nodeType, "CONST") == 0 ) {
			this->findConstText( curNode, nodeText);
			sprintf(nodeOuttype, "%s", curNode->ToElement()->Attribute("outtype"));
			if ( strcmp(nodeOuttype	, "(null)") == 0){
				sprintf(nodeOuttype, "NX_CHAR");
			}
			dataOutType = this->typeStringToVal(nodeOuttype);
			if ( dataOutType == NX_CHAR ) {
				nodeTextLen = strlen(nodeText);
			}
			else {
				nodeTextLen = 1;
			}
			pValue = allocConstValue( dataOutType, nodeTextLen);
			constTextToDataType(nodeText, dataOutType, pValue);
			NXputattr(this->nxFileHandle, nodeName, pValue, nodeTextLen, dataOutType);
			free(pValue);

		}
	}

	else {
		sprintf(nodeSource, "%s", curNode->ToElement()->Attribute("source"));
		if ( strcmp(nodeType, "ND_ATTR") == 0 ) {
			pAttr = pArray->findAttribute(nodeSource);
			pAttr->getValueInfo(&attrDataType, &attrDataSize);
			this->getAttrTypeNSize(pAttr, &dataOutType, &wordSize);

			if (dataOutType > 0) {
				pValue = calloc( attrDataSize, wordSize );
				pString = (char *)pValue;
				pAttr->getValue(attrDataType, (char *)pValue, attrDataSize);

				numWords = attrDataSize/wordSize;
				NXmakedata( this->nxFileHandle, nodeValue, dataOutType, 1, (int *)&(numWords));
				NXopendata(this->nxFileHandle, nodeValue);
				NXputdata(this->nxFileHandle, (char *)pValue);
				free(pValue);
				this->iterateNodes(curNode, pArray);
				NXclosedata(this->nxFileHandle);
			}
		}
		else if ( strcmp(nodeType, "pArray") == 0 ){

			rank = pArray->ndims;
			type = pArray->dataType;
			for (ii=0; ii<rank; ii++) {
				dims[(rank-1) - ii] = pArray->dims[ii].size;
			}

			switch(type) {
				case NDInt8:
					dataOutType = NX_INT8;
					wordSize = 1;
					break;
				case NDUInt8:
					dataOutType = NX_UINT8;
					wordSize = 1;
					break;
				case NDInt16:
					dataOutType = NX_INT16;
					wordSize = 2;
					break;
				case NDUInt16:
					dataOutType = NX_UINT16;
					wordSize = 2;
					break;
				case NDInt32:
					dataOutType = NX_INT32;
					wordSize = 4;
					break;
				case NDUInt32:
					dataOutType = NX_UINT32;
					wordSize = 4;
					break;
				case NDFloat32:
				   dataOutType = NX_FLOAT32;
					wordSize = 4;
				   break;
				case NDFloat64:
					dataOutType = NX_FLOAT64;
					wordSize = 8;
					break;
				}


			NXmakedata( this->nxFileHandle, nodeValue, dataOutType, rank, dims);
			NXopendata(this->nxFileHandle, nodeValue);
			NXputdata(this->nxFileHandle, pArray->pData);
			this->iterateNodes(curNode, pArray);
			NXclosedata(this->nxFileHandle);
		}
		else if ( strcmp(nodeType, "CONST") == 0 ){
			this->findConstText( curNode, nodeText);

			sprintf(nodeOuttype, "%s", curNode->ToElement()->Attribute("outtype"));
			if ( strcmp(nodeOuttype	, "(null)") == 0){
				sprintf(nodeOuttype, "NX_CHAR");
			}
			dataOutType = this->typeStringToVal(nodeOuttype);
			if ( dataOutType == NX_CHAR ) {
				nodeTextLen = strlen(nodeText);
			}
			else {
				nodeTextLen = 1;
			}
			pValue = allocConstValue( dataOutType, nodeTextLen);
			constTextToDataType(nodeText, dataOutType, pValue);

//			nodeTextLen = strlen(nodeText);
			NXmakedata( this->nxFileHandle, nodeValue, dataOutType, 1, (int *)&nodeTextLen);
			NXopendata(this->nxFileHandle, nodeValue);
			NXputdata(this->nxFileHandle, pValue);
			free(pValue);
			this->iterateNodes(curNode, pArray);
			NXclosedata(this->nxFileHandle);
		}
		else {
			this->findConstText( curNode, nodeText);

//			sprintf(nodeOuttype, "NX_CHAR");
			dataOutType = NX_CHAR;
			nodeTextLen = strlen(nodeText);

			pValue = allocConstValue( dataOutType, nodeTextLen);
			constTextToDataType(nodeText, dataOutType, pValue);

			NXmakedata( this->nxFileHandle, nodeValue, dataOutType, 1, (int *)&nodeTextLen);
			NXopendata(this->nxFileHandle, nodeValue);
			NXputdata(this->nxFileHandle, pValue);
			this->iterateNodes(curNode, pArray);
			NXclosedata(this->nxFileHandle);
		}
	}
	return (status);

}

void NDFileNexus::iterateNodes(TiXmlNode *curNode, NDArray *pArray) {
	TiXmlNode *childNode;
	childNode=0;
	while (childNode = curNode->IterateChildren(childNode)) {
			if (childNode->Type() <2 ){
			this->processNode(childNode, pArray);
			}
		}
	return;
}

void NDFileNexus::getAttrTypeNSize(NDAttribute *pAttr, int *retType, int *retSize) {
	int dataOutType;
	int wordSize;
	NDAttrDataType_t attrDataType;
	size_t attrDataSize;

	pAttr->getValueInfo(&attrDataType, &attrDataSize);

	switch(attrDataType) {
		case NDAttrInt8:
			dataOutType = NX_INT8;
			wordSize = 1;
			break;
		case NDAttrUInt8:
			dataOutType = NX_UINT8;
			wordSize = 1;
			break;
		case NDAttrInt16:
			dataOutType = NX_INT16;
			wordSize = 2;
			break;
		case NDAttrUInt16:
			dataOutType = NX_UINT16;
			wordSize = 2;
			break;
		case NDAttrInt32:
			dataOutType = NX_INT32;
			wordSize = 4;
			break;
		case NDAttrUInt32:
			dataOutType = NX_UINT32;
			wordSize = 4;
			break;
		case NDAttrFloat32:
		   dataOutType = NX_FLOAT32;
			wordSize = 4;
		   break;
		case NDAttrFloat64:
			dataOutType = NX_FLOAT64;
			wordSize = 8;
			break;
		case NDAttrString:
			dataOutType = NX_CHAR;
			wordSize = 1;
			break;
		case NDAttrUndefined:
		default:
			dataOutType = -1;
			wordSize = 1;
			break;
		}

	*retType = dataOutType;
	*retSize = wordSize;
}

void NDFileNexus::findConstText(TiXmlNode *curNode, char *outtext) {
	TiXmlNode *childNode;
			childNode = 0;
			childNode = curNode->IterateChildren(childNode);
			if (childNode == NULL) {
				sprintf(outtext, "");
				return;
			}
			while (childNode->Type() != 4) {
				childNode = curNode->IterateChildren(childNode);
				}
			if(childNode != NULL) {
				sprintf(outtext, "%s", childNode->Value());
			}
			else {
				sprintf(outtext, "%s", "");
			}
	return;

}

void * NDFileNexus::allocConstValue(int dataType, int length ) {
	void *pValue;
	switch (dataType) {
		case  NX_INT8:
		case NX_UINT8:
			pValue = calloc( length, sizeof(char) );
			break;
		case NX_INT16:
		case NX_UINT16:
			pValue = calloc( length, sizeof(short) );
			break;
		case NX_INT32:
		case NX_UINT32:
			pValue = calloc( length, sizeof(int) );
			break;
		case NX_FLOAT32:
			pValue = calloc( length, sizeof(float) );
		   break;
		case NX_FLOAT64:
			pValue = calloc( length, sizeof(double) );
			break;
		case NX_CHAR:
			pValue = calloc( length + 1 , sizeof(char) );
			break;
		case NDAttrUndefined:
		default:
			break;
	}
	return pValue;
}

void NDFileNexus::constTextToDataType(char *inText, int dataType, void *pValue) {
	union DATATYPE {
		char ch;
		unsigned char uch;
		short s;
		unsigned short us;
		int i;
		unsigned int ui;
		long l;
		unsigned long ul;
		float f;
		double d;
	} temp;
	int ii;
	double myDoub = 0.0;
	temp.d = 0.0;

	switch (dataType) {
		case  NX_INT8:
			sscanf((const char *)inText, "%d", &(temp.ch));
			*(char *)pValue = temp.ch;
			break;
		case NX_UINT8:
			sscanf((const char *)inText, "%d", &(temp.uch));
			*(unsigned char *)pValue = temp.uch;
			break;
		case NX_INT16:
			sscanf((const char *)inText, "%d", &(temp.s));
			*(short *)pValue = temp.s;
			break;
		case NX_UINT16:
			sscanf((const char *)inText, "%d", &(temp.us));
			*(unsigned short *)pValue = temp.us;
			break;
		case NX_INT32:
			sscanf((const char *)inText, "%d", &(temp.i));
			*(int *)pValue = temp.i;
			break;
		case NX_UINT32:
			sscanf((const char *)inText, "%d", &(temp.ui));
			*(unsigned int *)pValue = temp.ui;
			break;
		case NX_FLOAT32:
			sscanf((const char *)inText, "%f", &(temp.f));
			*(float *)pValue = temp.f;
		   break;
		case NX_FLOAT64:
			// Note here that the format code %lf may not work on all systems
			//it does seem to be the most common though.
			sscanf((const char *)inText, "%lf", &(temp.d));
			*(double *)pValue = temp.d;
			break;
		case NX_CHAR:
			for (ii = 0; ii < (int)strlen(inText); ii++) {
				((char *)pValue)[ii] = inText[ii];
			}
			((char *)pValue)[strlen(inText)] = '\0';
//			sscanf((const char *)inText, "%s", (char *)pValue);
			break;
		case NDAttrUndefined:
		default:
			break;
	}
	return;
}

int NDFileNexus::typeStringToVal( char * typeStr ) {
	if (strcmp (typeStr, "NX_CHAR") == 0 ) {
		return NX_CHAR;
	}
	else if (strcmp(typeStr, "NX_INT8") == 0) {
		return NX_INT8;
	}
	else if (strcmp(typeStr, "NX_UINT8") == 0) {
		return NX_UINT8;
	}
	else if (strcmp(typeStr, "NX_INT16") == 0) {
		return NX_INT16;
	}
	else if (strcmp(typeStr, "NX_UINT16") == 0) {
		return NX_UINT16;
	}
	else if (strcmp(typeStr, "NX_INT32") == 0) {
		return NX_INT32;
	}
	else if (strcmp(typeStr, "NX_UINT32") == 0) {
		return NX_UINT32;
	}
	else if (strcmp(typeStr, "NX_FLOAT32") == 0) {
		return NX_FLOAT32;
	}
	else if (strcmp(typeStr, "NX_FLOAT64") == 0) {
		return NX_FLOAT64;
	}
	else return -1;

}
/* asynDrvUser interface methods */
asynStatus NDFileNexus::drvUserCreate(asynUser *pasynUser, const char *drvInfo,
                                       const char **pptypeName, size_t *psize)
{
    asynStatus status;
    //const char *functionName = "drvUserCreate";

    status = this->drvUserCreateParam(pasynUser, drvInfo, pptypeName, psize,
                                      NDFileNexusParamString, NUM_ND_FILE_NEXUS_PARAMS);


    /* If not, then call the base class method, see if it is known there */
	if (status != asynSuccess){
       status = NDPluginDriver::drvUserCreate(pasynUser, drvInfo, pptypeName, psize);
	}
    return(status);
}

/** Constructor for NDFileNexus; all parameters are simply passed to NDPluginFile::NDPluginFile.
  * \param[in] portName The name of the asyn port driver to be created.
  * \param[in] queueSize The number of NDArrays that the input queue for this plugin can hold when
  *            NDPluginDriverBlockingCallbacks=0.  Larger queues can decrease the number of dropped arrays,
  *            at the expense of more NDArray buffers being allocated from the underlying driver's NDArrayPool.
  * \param[in] blockingCallbacks Initial setting for the NDPluginDriverBlockingCallbacks flag.
  *            0=callbacks are queued and executed by the callback thread; 1 callbacks execute in the thread
  *            of the driver doing the callbacks.
  * \param[in] NDArrayPort Name of asyn port driver for initial source of NDArray callbacks.
  * \param[in] NDArrayAddr asyn port driver address for initial source of NDArray callbacks.
  * \param[in] priority The thread priority for the asyn port driver thread if ASYN_CANBLOCK is set in asynFlags.
  * \param[in] stackSize The stack size for the asyn port driver thread if ASYN_CANBLOCK is set in asynFlags.
  */
NDFileNexus::NDFileNexus(const char *portName, int queueSize, int blockingCallbacks,
                       const char *NDArrayPort, int NDArrayAddr,
                       int priority, int stackSize)
    /* Invoke the base class constructor.
     * We allocate 2 NDArray of unlimited size in the NDArray pool.
     * This driver can block (because writing a file can be slow), and it is not multi-device.
     * Set autoconnect to 1.  priority and stacksize can be 0, which will use defaults. */
    : NDPluginFile(portName, queueSize, blockingCallbacks,
                   NDArrayPort, NDArrayAddr, 1, NDFileNexusLastParam,
                   2, -1, asynGenericPointerMask, asynGenericPointerMask,
                   ASYN_CANBLOCK, 1, priority, stackSize)
{
    //const char *functionName = "NDFileNexus";
	int addr = 0;

	this->supportsMultipleArrays = 1;
	connectToArrayPort();
}

/* Configuration routine.  Called directly, or from the iocsh  */

extern "C" int NDFileNexusConfigure(const char *portName, int queueSize, int blockingCallbacks,
                                   const char *NDArrayPort, int NDArrayAddr,
                                   int priority, int stackSize)
{
    new NDFileNexus(portName, queueSize, blockingCallbacks, NDArrayPort, NDArrayAddr,
                     priority, stackSize);
    return(asynSuccess);
}


/* EPICS iocsh shell commands */

static const iocshArg initArg0 = { "portName",iocshArgString};
static const iocshArg initArg1 = { "frame queue size",iocshArgInt};
static const iocshArg initArg2 = { "blocking callbacks",iocshArgInt};
static const iocshArg initArg3 = { "NDArray Port",iocshArgString};
static const iocshArg initArg4 = { "NDArray Addr",iocshArgInt};
static const iocshArg initArg5 = { "priority",iocshArgInt};
static const iocshArg initArg6 = { "stack size",iocshArgInt};
static const iocshArg * const initArgs[] = {&initArg0,
                                            &initArg1,
                                            &initArg2,
                                            &initArg3,
                                            &initArg4,
                                            &initArg5,
                                            &initArg6};
static const iocshFuncDef initFuncDef = {"NDFileNexusConfigure",7,initArgs};
static void initCallFunc(const iocshArgBuf *args)
{
    NDFileNexusConfigure(args[0].sval, args[1].ival, args[2].ival, args[3].sval, args[4].ival, args[5].ival, args[6].ival);
}

extern "C" void NDFileNexusRegister(void)
{
    iocshRegister(&initFuncDef,initCallFunc);
}

extern "C" {
epicsExportRegistrar(NDFileNexusRegister);
}