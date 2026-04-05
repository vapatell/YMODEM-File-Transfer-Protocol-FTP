//============================================================================
// File Name   : SenderY.cpp
// Description : Part 2
//============================================================================

#include "SenderY.h"

#include <iostream>
//#include <experimental/filesystem> // for C++14
#include <filesystem>
#include <stdio.h> // for snprintf()
#include <stdint.h> // for uint8_t
#include <string.h> // for memset(), and memcpy() or strncpy()
#include <errno.h>
#include <fcntl.h>	// for O_RDWR
#include <sys/stat.h>

#include "myIO.h"
#include "SenderSS.h"
#include "VNPE.h"
#include "AtomicCOUT.h"

#define REPORT_INFO

using namespace std;
using namespace std::filesystem; // C++17
//using namespace experimental::filesystem; // C++14
using namespace Sender_SS;

SenderY::
SenderY(vector<const char*> iFileNames, int d)
:PeerY(d),
 bytesRd(-1), 
 fileName(nullptr),
 fileNames(iFileNames),
 fileNameIndex(0),
 blkNum(0)
{
}

//-----------------------------------------------------------------------------

// get rid of any characters that may have arrived from the medium.
void SenderY::dumpGlitches()
{
	const int dumpBufSz = 20;
	char buf[dumpBufSz];
	int bytesRead;
	while (dumpBufSz == (bytesRead = PE(myReadcond(mediumD, buf, dumpBufSz, 0, 0, 0))));
}

// Send the block, less the block's last byte, to the receiver.
// Returns the block's last byte.
uint8_t SenderY::sendMostBlk(blkT blkBuf)
//uint8_t SenderY::sendMostBlk(uint8_t blkBuf[BLK_SZ_CRC])
{
	const int mostBlockSize = (BLK_SZ_CRC) - 1;
	PE_NOT(myWrite(mediumD, blkBuf, mostBlockSize), mostBlockSize);
	return *(blkBuf + mostBlockSize);
}

// Send the last byte of a block to the receiver
// First wait for previous part of the block to be drained
// and then dump any received glitches.
void
SenderY::
sendLastByte(uint8_t lastByte)
{
	PE(myTcdrain(mediumD)); // wait for previous part of block to be completely drained from the descriptor
	dumpGlitches();			// dump any received glitches

	PE_NOT(myWrite(mediumD, &lastByte, sizeof(lastByte)), sizeof(lastByte));
}

/* Generate a block (numbered 0) with filename and filesize (a "stat" block).
 * If fileName is empty (""), generate an empty stat block */
void SenderY::genStatBlk(blkT blkBuf, const char* fileName)
//void SenderY::genStatBlk(uint8_t blkBuf[BLK_SZ_CRC], const char* fileName)
{
    blkBuf[SOH_OH] = 0;
    blkBuf[SOH_OH + 1] = ~0;
    int index = DATA_POS;
    if (*fileName) { // (0 != strcmp("", fileName)) { // (strlen(fileName)) {
        const auto myBasename = path( fileName ).filename().string();
        auto c_basename = myBasename.c_str();
        int fileNameLengthPlus1 = strlen(c_basename) + 1;
        // check for fileNameLengthPlus1 greater than 127.
        if (fileNameLengthPlus1 + 1 > CHUNK_SZ) { // need at least one decimal digit to store st.st_size below
            COUT /* cerr */ << "Ran out of space in file info block!  Need block with 1024 bytes of data." << endl;
            exit(-1);
        }
        // On Linux: The maximum length for a file name is 255 bytes. The maximum combined length of both the file name and path name is 4096 bytes.
        memcpy(&blkBuf[index], c_basename, fileNameLengthPlus1);
        //strncpy(&blkBuf[index], c_basename, 12X);
        index += fileNameLengthPlus1;
        struct stat st;
        PE(stat(fileName, &st));
        int spaceAvailable = CHUNK_SZ + DATA_POS - index;
        int spaceNeeded = snprintf((char*)&blkBuf[index], spaceAvailable, "%ld", st.st_size); // check the value of CHUNK_SZ + DATA_POS - index
        if (spaceNeeded > spaceAvailable) {
            COUT /* cerr */ << "Ran out of space in file info block!  Need block with 1024 bytes of data." << endl;
            exit(-1);
        }
        index += spaceNeeded + 1;
    }
    uint8_t padSize = CHUNK_SZ + DATA_POS - index;
    memset(blkBuf+index, 0, padSize);

    // check here if index is greater than 128 or so.
    blkBuf[0] = SOH; // can be pre-initialized for efficiency if no 1K blocks allowed

    /* calculate and add CRC in network byte order */
    crc16ns((uint16_t*)&blkBuf[PAST_CHUNK], &blkBuf[DATA_POS]);
}

/* tries to generate a block.  Updates the
variable bytesRd with the number of bytes that were read
from the input file in order to create the block. Sets
bytesRd to 0 and does not actually generate a block if the end
of the input file had been reached when the previously generated block
was prepared or if the input file is empty (i.e. has 0 length).
*/
//void SenderY::genBlk(blkT blkBuf)
void SenderY::genBlk(uint8_t blkBuf[BLK_SZ_CRC])
{
	//read data and store it directly at the data portion of the buffer
	bytesRd = PE(myRead(transferringFileD, &blkBuf[DATA_POS], CHUNK_SZ ));
	if (bytesRd>0) {
		blkBuf[0] = SOH; // can be pre-initialized for efficiency
		//block number and its complement
		blkBuf[SOH_OH] = blkNum;
		blkBuf[SOH_OH + 1] = ~blkNum;

			//pad ctrl-z for the last block
			uint8_t padSize = CHUNK_SZ - bytesRd;
			memset(blkBuf+DATA_POS+bytesRd, CTRL_Z, padSize);

		/* calculate and add CRC in network byte order */
		crc16ns((uint16_t*)&blkBuf[PAST_CHUNK], &blkBuf[DATA_POS]);
	}
}

//Send CAN_LEN copies of CAN characters in a row to the YMODEM receiver, to inform it of
//	the cancelling of a session
void SenderY::cans()
{
	// No need to space in time CAN chars for Part 2.
	// This function will be more complicated in later parts. 
    char buffer[CAN_LEN];
    memset( buffer, CAN, CAN_LEN);
    PE_NOT(myWrite(mediumD, buffer, CAN_LEN), CAN_LEN);
}

/* Open a file to transfer unless there are none left to transfer in
 * which case set the fileName to nullptr.
 * Prepare a stat block with filename and file size, or an empty
 * stat block if there are no more files to send.
 * Initialize blkNum to 0.
*/
void SenderY::prepStatBlk()
{
    blkNum = 0;
    if (fileNameIndex < fileNames.size()) {
        fileName = fileNames[fileNameIndex];
        fileNameIndex++;
        openFileToTransfer(fileName);
        if(transferringFileD != -1) {
            genStatBlk(blkBufs[0], fileName); // prepare 0eth block
        }
    }
    else {
        transferringFileD = -2; // no more files to transfer
        genStatBlk(blkBufs[0], ""); // prepare 0eth block
        fileName = nullptr;
    }
}

/* While sending the now current block for the first time, prepare the next block if possible.
*/
void SenderY::sendBlkPrepNext()
{
	// **** this function will need to be modified ****
#ifdef REPORT_INFO
	// block will be "w"ritten to mediumD
	COUT << "\n[w" << (int)blkNum << "]" << flush;
#endif
	uint8_t lastByte = sendMostBlk(blkBufs[blkNum%2]);
    ++blkNum; // stat block just sent or previous block ACK'd
	if (fileName) {
	    genBlk(blkBufs[(blkNum)%2]); // prepare next block
	}
	sendLastByte(lastByte);
}

// Resends the block that had been sent previously to the YMODEM receiver.
void SenderY::resendBlk()
{
	// resend the block including the crc16 (or checksum if code available for that)
	//  ***** You will have to write this simple function *****
#ifdef REPORT_INFO
	// block will be "r"ewritten
	COUT << "[r" << (int)(uint8_t)(blkNum-1) << "]" << flush;
#endif
	sendLastByte(sendMostBlk(blkBufs[((uint8_t)(blkNum-1))%2]));
}

// Open a file to send and store the file descriptor.
int
SenderY::
openFileToTransfer(const char* fileName)
{
    transferringFileD = myOpen(fileName, O_RDONLY);
    return transferringFileD;
}

/* If not already closed, close file that was transferred (or being transferred).
 * Set transferringFileD to -1 when file is closed.
 * Return 0 if file is closed or 1 if file was already closed.
 */
int
SenderY::
closeTransferredFile()
{
    if (transferringFileD != -1) {
        PE(myClose(transferringFileD));
        transferringFileD = -1;
        return 0;
    }
    else
        return 1;
}

// Run the YMODEM protocol to send files.
void SenderY::sendFiles()
{
    auto mySenderSmSp(make_shared<SenderSS>(this)); // or use make_unique
    mySenderSmSp->setDebugLog(NULL);

    // Because one source of sender behaviour is automatically generated by SmartState Studio,
    //	the behaviour has been factored out into a class SenderSS, but for your Part 2 submission
    //	we are not expecting a separate class as has been done here.
    while(mySenderSmSp->isRunning()) {
        unsigned char byteToReceive;
        PE_NOT(myRead(mediumD, &byteToReceive, 1), 1);
        mySenderSmSp->postEvent(SER, byteToReceive);
    }
}

