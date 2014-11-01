/*
 * NDFileOperations.cpp
 *
 *  Created on: 1 Nov 2014
 *      Author: Jonathan
 */

#include "NDFileOperations.h"
#include "asynNDArrayDriver.h"

/** Constructor.
 *  Creates the asyn parameters.
 */
NDFileOperations::NDFileOperations(asynNDArrayDriver* drv) : drv(drv) {
    drv->createParam(NDFilePathString,             asynParamOctet,           &NDFilePath);
    drv->createParam(NDFilePathExistsString,       asynParamInt32,           &NDFilePathExists);
    drv->createParam(NDFileNameString,             asynParamOctet,           &NDFileName);
    drv->createParam(NDFileNumberString,           asynParamInt32,           &NDFileNumber);
    drv->createParam(NDFileTemplateString,         asynParamOctet,           &NDFileTemplate);
    drv->createParam(NDAutoIncrementString,        asynParamInt32,           &NDAutoIncrement);
    drv->createParam(NDFullFileNameString,         asynParamOctet,           &NDFullFileName);
    drv->createParam(NDFileFormatString,           asynParamInt32,           &NDFileFormat);
    drv->createParam(NDAutoSaveString,             asynParamInt32,           &NDAutoSave);
    drv->createParam(NDWriteFileString,            asynParamInt32,           &NDWriteFile);
    drv->createParam(NDReadFileString,             asynParamInt32,           &NDReadFile);
    drv->createParam(NDFileWriteModeString,        asynParamInt32,           &NDFileWriteMode);
    drv->createParam(NDFileWriteStatusString,      asynParamInt32,           &NDFileWriteStatus);
    drv->createParam(NDFileWriteMessageString,     asynParamOctet,           &NDFileWriteMessage);
    drv->createParam(NDFileNumCaptureString,       asynParamInt32,           &NDFileNumCapture);
    drv->createParam(NDFileNumCapturedString,      asynParamInt32,           &NDFileNumCaptured);
    drv->createParam(NDFileCaptureString,          asynParamInt32,           &NDFileCapture);
    drv->createParam(NDFileDeleteDriverFileString, asynParamInt32,           &NDFileDeleteDriverFile);
    drv->createParam(NDFileLazyOpenString,         asynParamInt32,           &NDFileLazyOpen);
    /* Set the initial values of FileSave, FileRead, and FileCapture so the readbacks are initialized */
    drv->setIntegerParam(NDWriteFile, 0);
    drv->setIntegerParam(NDReadFile, 0);
    drv->setIntegerParam(NDFileCapture, 0);
    drv->setIntegerParam(NDFileWriteStatus, 0);
    drv->setStringParam (NDFileWriteMessage, "");
    /* We set FileTemplate to a reasonable value because it cannot be defined in the database, since it is a
     * waveform record. However, the waveform record does not currently read the driver value for initialization! */
    drv->setStringParam (NDFileTemplate, "%s%s_%3.3d.dat");
    drv->setIntegerParam(NDFileNumCaptured, 0);
}

/** Destructor.
 *  Does nothing at the moment.
 */
NDFileOperations::~NDFileOperations() {
}

/** Checks whether the directory specified NDFilePath parameter exists.
  *
  * This is a convenience function that determinesthe directory specified NDFilePath parameter exists.
  * It sets the value of NDFilePathExists to 0 (does not exist) or 1 (exists).
  * It also adds a trailing '/' character to the path if one is not present.
  * Returns a error status if the directory does not exist.
  */
int NDFileOperations::checkPath()
{
    /* Formats a complete file name from the components defined in NDStdDriverParams */
    int status = asynError;
    char filePath[MAX_FILENAME_LEN];
    int hasTerminator=0;
    struct stat buff;
    size_t len;
    int isDir=0;
    int pathExists=0;

    status = drv->getStringParam(NDFilePath, sizeof(filePath), filePath);
    len = strlen(filePath);
    if (len == 0) return(asynSuccess);
    /* If the path contains a trailing '/' or '\' remove it, because Windows won't find
     * the directory if it has that trailing character */
    if ((filePath[len-1] == '/') || (filePath[len-1] == '\\')) {
        filePath[len-1] = 0;
        len--;
        hasTerminator=1;
    }
    status = stat(filePath, &buff);
    if (!status) isDir = (S_IFDIR & buff.st_mode);
    if (!status && isDir) {
        pathExists = 1;
        status = asynSuccess;
    }
    /* If the path did not have a trailing terminator then add it if there is room */
    if (!hasTerminator) {
        if (len < MAX_FILENAME_LEN-2) strcat(filePath, "/");
        drv->setStringParam(NDFilePath, filePath);
    }
    drv->setIntegerParam(NDFilePathExists, pathExists);
    return(status);
}

/** Build a file name from component parts.
  * \param[in] maxChars  The size of the fullFileName string.
  * \param[out] fullFileName The constructed file name including the file path.
  *
  * This is a convenience function that constructs a complete file name
  * from the NDFilePath, NDFileName, NDFileNumber, and
  * NDFileTemplate parameters. If NDAutoIncrement is true then it increments the
  * NDFileNumber after creating the file name.
  */
int NDFileOperations::createFileName(int maxChars, char *fullFileName)
{
    /* Formats a complete file name from the components defined in NDStdDriverParams */
    int status = asynSuccess;
    char filePath[MAX_FILENAME_LEN];
    char fileName[MAX_FILENAME_LEN];
    char fileTemplate[MAX_FILENAME_LEN];
    int fileNumber;
    int autoIncrement;
    int len;

    this->checkPath();
    status |= drv->getStringParam(NDFilePath, sizeof(filePath), filePath);
    status |= drv->getStringParam(NDFileName, sizeof(fileName), fileName);
    status |= drv->getStringParam(NDFileTemplate, sizeof(fileTemplate), fileTemplate);
    status |= drv->getIntegerParam(NDFileNumber, &fileNumber);
    status |= drv->getIntegerParam(NDAutoIncrement, &autoIncrement);
    if (status) return(status);
    len = epicsSnprintf(fullFileName, maxChars, fileTemplate,
                        filePath, fileName, fileNumber);
    if (len < 0) {
        status |= asynError;
        return(status);
    }
    if (autoIncrement) {
        fileNumber++;
        status |= drv->setIntegerParam(NDFileNumber, fileNumber);
    }
    return(status);
}

/** Build a file name from component parts.
  * \param[in] maxChars  The size of the fullFileName string.
  * \param[out] filePath The file path.
  * \param[out] fileName The constructed file name without file file path.
  *
  * This is a convenience function that constructs a file path and file name
  * from the NDFilePath, NDFileName, NDFileNumber, and
  * NDFileTemplate parameters. If NDAutoIncrement is true then it increments the
  * NDFileNumber after creating the file name.
  */
int NDFileOperations::createFileName(int maxChars, char *filePath, char *fileName)
{
    /* Formats a complete file name from the components defined in NDStdDriverParams */
    int status = asynSuccess;
    char fileTemplate[MAX_FILENAME_LEN];
    char name[MAX_FILENAME_LEN];
    int fileNumber;
    int autoIncrement;
    int len;

    this->checkPath();
    status |= drv->getStringParam(NDFilePath, maxChars, filePath);
    status |= drv->getStringParam(NDFileName, sizeof(name), name);
    status |= drv->getStringParam(NDFileTemplate, sizeof(fileTemplate), fileTemplate);
    status |= drv->getIntegerParam(NDFileNumber, &fileNumber);
    status |= drv->getIntegerParam(NDAutoIncrement, &autoIncrement);
    if (status) return(status);
    len = epicsSnprintf(fileName, maxChars, fileTemplate,
                        name, fileNumber);
    if (len < 0) {
        status |= asynError;
        return(status);
    }
    if (autoIncrement) {
        fileNumber++;
        status |= drv->setIntegerParam(NDFileNumber, fileNumber);
    }
    return(status);
}

/** Call when asyn clients call pasynOctet->write().
  * This function performs special actions on file related writes.
  * It assumes the parameter library has already been set.
  * \param[in] pasynUser pasynUser structure that encodes the reason and address.
  * \param[in] value Address of the string to write.
  * \param[in] nChars Number of characters to write.
  * \param[out] nActual Number of characters actually written. */
asynStatus NDFileOperations::writeOctet(asynUser *pasynUser, const char *value,
                                    size_t nChars, size_t *nActual)
{
    int addr=0;
    int function = pasynUser->reason;
    asynStatus status = asynSuccess;

    status = getAddress(pasynUser, &addr); if (status != asynSuccess) return(status);

    if (function == NDFilePath) {
        this->checkPath();
    }
     /* Do callbacks so higher layers see any changes */
    status = (asynStatus)drv->callParamCallbacks(addr, addr);
    return status;
}
