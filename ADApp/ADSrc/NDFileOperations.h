/*
 * NDFileOperations.h
 *
 *  Created on: 1 Nov 2014
 *      Author: Jonathan
 */

#ifndef NDFILEOPERATIONS_H_
#define NDFILEOPERATIONS_H_

#include "asynDriver.h"
class asynNDArrayDriver;

/** Maximum length of a filename or any of its components */
#define MAX_FILENAME_LEN 256

/** Enumeration of file saving modes */
typedef enum {
    NDFileModeSingle,       /**< Write 1 array per file */
    NDFileModeCapture,      /**< Capture NDNumCapture arrays into memory, write them out when capture is complete.
                              *  Write all captured arrays to a single file if the file format supports this */
    NDFileModeStream        /**< Stream arrays continuously to a single file if the file format supports this */
} NDFileMode_t;

typedef enum {
    NDFileWriteOK,
    NDFileWriteError
} NDFileWriteStatus_t;

/* File name related parameters for saving data.
 * Drivers are not required to implement file saving, but if they do these parameters
 * should be used.
 * The driver will normally combine NDFilePath, NDFileName, and NDFileNumber into
 * a file name that order using the format specification in NDFileTemplate.
 * For example NDFileTemplate might be "%s%s_%d.tif" */
#define NDFilePathString        "FILE_PATH"         /**< (asynOctet,    r/w) The file path */
#define NDFilePathExistsString  "FILE_PATH_EXISTS"  /**< (asynInt32,    r/w) File path exists? */
#define NDFileNameString        "FILE_NAME"         /**< (asynOctet,    r/w) The file name */
#define NDFileNumberString      "FILE_NUMBER"       /**< (asynInt32,    r/w) The next file number */
#define NDFileTemplateString    "FILE_TEMPLATE"     /**< (asynOctet,    r/w) The file format template; C-style format string */
#define NDAutoIncrementString   "AUTO_INCREMENT"    /**< (asynInt32,    r/w) Autoincrement file number; 0=No, 1=Yes */
#define NDFullFileNameString    "FULL_FILE_NAME"    /**< (asynOctet,    r/o) The actual complete file name for the last file saved */
#define NDFileFormatString      "FILE_FORMAT"       /**< (asynInt32,    r/w) The data format to use for saving the file.  */
#define NDAutoSaveString        "AUTO_SAVE"         /**< (asynInt32,    r/w) Automatically save files */
#define NDWriteFileString       "WRITE_FILE"        /**< (asynInt32,    r/w) Manually save the most recent array to a file when value=1 */
#define NDReadFileString        "READ_FILE"         /**< (asynInt32,    r/w) Manually read file when value=1 */
#define NDFileWriteModeString   "WRITE_MODE"        /**< (asynInt32,    r/w) File saving mode (NDFileMode_t) */
#define NDFileWriteStatusString "WRITE_STATUS"      /**< (asynInt32,    r/w) File write status */
#define NDFileWriteMessageString "WRITE_MESSAGE"    /**< (asynOctet,    r/w) File write message */
#define NDFileNumCaptureString  "NUM_CAPTURE"       /**< (asynInt32,    r/w) Number of arrays to capture */
#define NDFileNumCapturedString "NUM_CAPTURED"      /**< (asynInt32,    r/o) Number of arrays already captured */
#define NDFileCaptureString     "CAPTURE"           /**< (asynInt32,    r/w) Start or stop capturing arrays */
#define NDFileDeleteDriverFileString  "DELETE_DRIVER_FILE"  /**< (asynInt32,    r/w) Delete driver file */
#define NDFileLazyOpenString    "FILE_LAZY_OPEN"    /**< (asynInt32,    r/w) Don't open file until first frame arrives in Stream mode */

/** Mixer class that contains file oriented operations used by some plugins and drivers.
 *  To use, add this class to the list of base classes and call the writeOctet function
 *  from the override of the asynNDArrayDriver::writeOctet function.  The asyn parameters
 *  and file operations will then be available for use.
 */
class NDFileOperations {
public:
	NDFileOperations(asynNDArrayDriver* drv);
	~NDFileOperations();
    int checkPath();
    int createFileName(int maxChars, char *fullFileName);
    int createFileName(int maxChars, char *filePath, char *fileName);
    asynStatus writeOctet(asynUser *pasynUser);
protected:
    int NDFilePath;
#define FILEOPS_FIRST_NDARRAY_PARAM NDFilePath
    int NDFilePathExists;
    int NDFileName;
    int NDFileNumber;
    int NDFileTemplate;
    int NDAutoIncrement;
    int NDFullFileName;
    int NDFileFormat;
    int NDAutoSave;
    int NDWriteFile;
    int NDReadFile;
    int NDFileWriteMode;
    int NDFileWriteStatus;
    int NDFileWriteMessage;
    int NDFileNumCapture;
    int NDFileNumCaptured;
    int NDFileCapture;
    int NDFileDeleteDriverFile;
    int NDFileLazyOpen;
#define FILEOPS_LAST_NDARRAY_PARAM NDFileLazyOpen
    asynNDArrayDriver* drv;
};

#define FILEOPS_NUM_NDARRAY_PARAMS ((int)(&FILEOPS_LAST_NDARRAY_PARAM - &FILEOPS_FIRST_NDARRAY_PARAM + 1))
#endif /* NDFILEOPERATIONS_H_ */
