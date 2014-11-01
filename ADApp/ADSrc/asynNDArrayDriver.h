#ifndef asynNDArrayDriver_H
#define asynNDArrayDriver_H

#include "asynPortDriver.h"
#include "NDArray.h"

/** Strings defining parameters that affect the behaviour of the detector. 
  * These are the values passed to drvUserCreate. 
  * The driver will place in pasynUser->reason an integer to be used when the
  * standard asyn interface methods are called. */
 /*                               String                 asyn interface  access   Description  */
#define NDPortNameSelfString    "PORT_NAME_SELF"    /**< (asynOctet,    r/o) Asyn port name of this driver instance */

    /* Parameters defining characteristics of the array data from the detector.
     * NDArraySizeX and NDArraySizeY are the actual dimensions of the array data,
     * including effects of the region definition and binning */
#define NDArraySizeXString      "ARRAY_SIZE_X"      /**< (asynInt32,    r/o) Size of the array data in the X direction */
#define NDArraySizeYString      "ARRAY_SIZE_Y"      /**< (asynInt32,    r/o) Size of the array data in the Y direction */
#define NDArraySizeZString      "ARRAY_SIZE_Z"      /**< (asynInt32,    r/o) Size of the array data in the Z direction */
#define NDArraySizeString       "ARRAY_SIZE"        /**< (asynInt32,    r/o) Total size of array data in bytes */
#define NDNDimensionsString     "ARRAY_NDIMENSIONS" /**< (asynInt32,    r/o) Number of dimensions in array */
#define NDDimensionsString      "ARRAY_DIMENSIONS"  /**< (asynInt32Array, r/o) Array dimensions */
#define NDDataTypeString        "DATA_TYPE"         /**< (asynInt32,    r/w) Data type (NDDataType_t) */
#define NDColorModeString       "COLOR_MODE"        /**< (asynInt32,    r/w) Color mode (NDColorMode_t) */
#define NDUniqueIdString        "UNIQUE_ID"         /**< (asynInt32,    r/o) Unique ID number of array */
#define NDTimeStampString       "TIME_STAMP"        /**< (asynFloat64,  r/o) Time stamp of array */
#define NDEpicsTSSecString      "EPICS_TS_SEC"      /**< (asynInt32,    r/o) EPOCS time stamp secPastEpoch of array */
#define NDEpicsTSNsecString     "EPICS_TS_NSEC"     /**< (asynInt32,    r/o) EPOCS time stamp nsec of array */
#define NDBayerPatternString    "BAYER_PATTERN"     /**< (asynInt32,    r/o) Bayer pattern of array  (from bayerPattern array attribute if present) */

    /* Statistics on number of arrays collected */
#define NDArrayCounterString    "ARRAY_COUNTER"     /**< (asynInt32,    r/w) Number of arrays since last reset */

#define NDAttributesFileString  "ND_ATTRIBUTES_FILE" /**< (asynOctet,    r/w) Attributes file name */

    /* The detector array data */
#define NDArrayDataString       "ARRAY_DATA"        /**< (asynGenericPointer,   r/w) NDArray data */
#define NDArrayCallbacksString  "ARRAY_CALLBACKS"   /**< (asynInt32,    r/w) Do callbacks with array data (0=No, 1=Yes) */

    /* NDArray Pool status */
#define NDPoolMaxBuffersString      "POOL_MAX_BUFFERS"
#define NDPoolAllocBuffersString    "POOL_ALLOC_BUFFERS"
#define NDPoolFreeBuffersString     "POOL_FREE_BUFFERS"
#define NDPoolMaxMemoryString       "POOL_MAX_MEMORY"
#define NDPoolUsedMemoryString      "POOL_USED_MEMORY"

/** This is the class from which NDArray drivers are derived; implements the asynGenericPointer functions 
  * for NDArray objects. 
  * For areaDetector, both plugins and detector drivers are indirectly derived from this class.
  * asynNDArrayDriver inherits from asynPortDriver.
  */
class epicsShareFunc asynNDArrayDriver : public asynPortDriver {
public:
    asynNDArrayDriver(const char *portName, int maxAddr, int numParams, int maxBuffers, size_t maxMemory,
                      int interfaceMask, int interruptMask,
                      int asynFlags, int autoConnect, int priority, int stackSize);
    virtual ~asynNDArrayDriver();
    /* These are the methods that we override from asynPortDriver */
    virtual asynStatus writeOctet(asynUser *pasynUser, const char *value, size_t maxChars,
                          size_t *nActual);
    virtual asynStatus readGenericPointer(asynUser *pasynUser, void *genericPointer);
    virtual asynStatus writeGenericPointer(asynUser *pasynUser, void *genericPointer);
    virtual asynStatus readInt32(asynUser *pasynUser, epicsInt32 *value);
    virtual asynStatus readFloat64(asynUser *pasynUser, epicsFloat64 *value);
    virtual void report(FILE *fp, int details);

    /* These are the methods that are new to this class */
    virtual int readNDAttributesFile(const char *fileName);
    virtual int getAttributes(NDAttributeList *pAttributeList);

protected:
    int NDPortNameSelf;
    #define FIRST_NDARRAY_PARAM NDPortNameSelf
    int NDArraySizeX;
    int NDArraySizeY;
    int NDArraySizeZ;
    int NDArraySize;
    int NDNDimensions;
    int NDDimensions;
    int NDDataType;
    int NDColorMode;
    int NDUniqueId;
    int NDTimeStamp;
    int NDEpicsTSSec;
    int NDEpicsTSNsec;
    int NDBayerPattern;
    int NDArrayCounter;
    int NDAttributesFile;
    int NDArrayData;
    int NDArrayCallbacks;
    int NDPoolMaxBuffers;
    int NDPoolAllocBuffers;
    int NDPoolFreeBuffers;
    int NDPoolMaxMemory;
    int NDPoolUsedMemory;
    #define LAST_NDARRAY_PARAM NDPoolUsedMemory

    NDArray **pArrays;             /**< An array of NDArray pointers used to store data in the driver */
    NDArrayPool *pNDArrayPool;     /**< An NDArrayPool object used to allocate and manipulate NDArray objects */
    class NDAttributeList *pAttributeList;  /**< An NDAttributeList object used to obtain the current values of a set of
                                          *  attributes */
};

#define NUM_NDARRAY_PARAMS ((int)(&LAST_NDARRAY_PARAM - &FIRST_NDARRAY_PARAM + 1))
#endif
