//============================================================================
// File Name   : ReceiverY.cpp
// Description : Part 2
//============================================================================

#include "ReceiverY.h"

#include <string.h> // for memset()
#include <fcntl.h>
#include <stdint.h>
#include <memory> // for pointer to SS class
#include <sstream>

#include "myIO.h"
#include "ReceiverSS.h"
#include "VNPE.h"
#include "AtomicCOUT.h"

using namespace std;
using namespace Receiver_SS;

ReceiverY::
ReceiverY(int d)
:PeerY(d),
 closeProb(1),
 anotherFile(0xFF),
 NCGbyte('C'),
 goodBlk(false), 
 goodBlk1st(false), 
 syncLoss(false), // transfer will end when syncLoss becomes true
 relativeTm(0), //relative timeout starts with infinte timeout
 bytesRemaining(0),
 numLastGoodBlk(255)
{
}

/* Only called after an SOH character has been received.
The function receives the remaining characters to form a complete
block.
The function will set or reset a Boolean variable,
goodBlk. This variable will be set (made true) only if the
calculated checksum or CRC agrees with the
one received and the received block number and received complement
are consistent with each other.
Boolean member variable syncLoss will only be set to
true when goodBlk is set to true AND there is a
fatal loss of syncronization as described in the XMODEM/YMODEM
specification.
The member variable goodBlk1st will be made true only if this is the first
time that the block was received in "good" condition. Otherwise
goodBlk1st will be made false.
*/
void ReceiverY::getRestBlk()
{
	const int restBlkSz = REST_BLK_SZ_CRC;
	PE_NOT(myReadcond(mediumD, &rcvBlk[1], restBlkSz, restBlkSz, 0, 0), restBlkSz);
	// consider receiving CRC after calculating local CRC

	syncLoss = false; // but might be made true below

	//(blkNumsOk) = ( block # and its complement are matched );
	bool blkNumsOk = (rcvBlk[2] == (uint8_t) ~rcvBlk[1]);
	if (!blkNumsOk) {
		goodBlk = goodBlk1st = false;
	}
	else {
		goodBlk1st = (rcvBlk[1] == (uint8_t) (numLastGoodBlk + 1)); // but might be made false below
		if (!goodBlk1st) {
			// determine fatal loss of synchronization
            if (transferringFileD == -1 || (rcvBlk[1] != numLastGoodBlk)) {
				syncLoss = true;
				goodBlk = false;
				COUT << "(s" << (unsigned) rcvBlk[1] << ":" << (unsigned) numLastGoodBlk << ")" << flush;
				return;
			}
#define ALLOW_DEEMED_GOOD
#ifdef ALLOW_DEEMED_GOOD
			else { // transerringFileD != -1 && (rcvBlk[1] == numLastGoodBlk)
				goodBlk = true; // "deemed" good block
				COUT << "(d" << (unsigned) rcvBlk[1] << ")" << flush;
				return;
			}
#endif
		}
		// detect if data error in chunk
		// consider receiving checksum/CRC after calculating local checksum/CRC
		uint16_t CRCbytes;
		crc16ns(&CRCbytes, &rcvBlk[DATA_POS]);
		goodBlk = (*((uint16_t*) &rcvBlk[PAST_CHUNK]) == CRCbytes);
		if (!goodBlk) {
			goodBlk1st = false; // but the block was "bad".
			COUT << "(b" << (unsigned) rcvBlk[1] << ")" << flush;
			return;
		}
#ifndef ALLOW_DEEMED_GOOD
		else if (!goodBlk1st) {
			COUT << "(r" << (unsigned) rcvBlk[1] << ")" << flush; // "resent" good block
			return;
		}
#endif
		// good block for the "first" time.
		numLastGoodBlk = rcvBlk[1];
		COUT << "(f" << (unsigned) rcvBlk[1] << ")" << endl;
	}
}

//Write chunk (file data) in a received block to disk.  Update the number of bytes remaining to be written.
void ReceiverY::writeChunk()
{
    bytesRemaining -= CHUNK_SZ;
    ssize_t writeSize = (bytesRemaining < 0) ? (CHUNK_SZ + bytesRemaining) : CHUNK_SZ;
	PE_NOT(myWrite(transferringFileD, &rcvBlk[DATA_POS], writeSize), writeSize);
}

// Open the output file to hold the file being transferred.
// Initialize the number of bytes remaining to be written with the file size.
int
ReceiverY::
openFileForTransfer()
{
    COUT << "(opening: " << &rcvBlk[DATA_POS] << ")" << flush;
    mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
    const char* fileNameP = (const char *) &rcvBlk[DATA_POS];
    transferringFileD = myCreat(fileNameP, mode);
    istringstream((const char *) &rcvBlk[DATA_POS + strlen(fileNameP) + 1]) >> bytesRemaining;
//    sscanf((const char *) &rcvBlk[DATA_POS + strlen(fileNameP) + 1], "%ld", &bytesRemaining);
    return transferringFileD;
}

/* If not already closed, close file that was just received (or being received).
 * Set transferringFileD to -1 and numLastGoodBlk to 255 when file is closed.  Thus numLastGoodBlk
 * is ready for the next file to be sent.
 * Return the errno if there was an error closing the file and otherwise return 0.
 */
int
ReceiverY::
closeTransferredFile()
{
    if (transferringFileD > -1) {
        closeProb = myClose(transferringFileD);
        if (closeProb)
            return errno;
        else {
            numLastGoodBlk=255;
            transferringFileD = -1;
        }
    }
    return 0;
}

//Send CAN_LEN CAN characters in a row to the YMODEM sender, to inform it of
//	the cancelling of a file transfer
void ReceiverY::cans()
{
	// no need to space in time CAN chars coming from receiver
    char buffer[CAN_LEN];
    memset( buffer, CAN, CAN_LEN);
    PE_NOT(myWrite(mediumD, buffer, CAN_LEN), CAN_LEN);
}

// anotherFile will be zero (0) if there are no more files to be received.
uint8_t
ReceiverY::
checkForAnotherFile()
{
    return (anotherFile = rcvBlk[DATA_POS]);
}

// Store a relative timeout.  Note this is different from Parts 4 and 5 where we
//    will store an absolute timeout given a relative timeout.
void ReceiverY::tm(int timeoutUnits)
{
    relativeTm = timeoutUnits;
}

// Run the YMODEM protocol to receive files.
void ReceiverY::receiveFiles()
{
	auto myReceiverSmSp(make_shared<ReceiverSS>(this)); // or use make_unique
	myReceiverSmSp->setDebugLog(NULL);

	// Because one source of receiver behaviour is automatically generated by SmartState Studio,
	//	the behaviour has been factored out into a class ReceiverSS, but for your Part 2 submission
	//	we are not expecting a separate class as has been done here.
	while(myReceiverSmSp->isRunning()) {
		unsigned char byteToReceive;
		int byteRead = PE(myReadcond(mediumD, &byteToReceive, 1, 1, relativeTm, relativeTm));
		if (byteRead)
		    myReceiverSmSp->postEvent(SER, byteToReceive);
		else
            myReceiverSmSp->postEvent(TM);
	}

	COUT << "\n"; // insert new line.
}
