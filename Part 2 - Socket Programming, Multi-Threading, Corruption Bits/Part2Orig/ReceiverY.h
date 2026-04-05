#ifndef RECEIVER_H
#define RECEIVER_H

#include "PeerY.h"

class ReceiverY : public PeerY
{
public:
	ReceiverY(int d);

	void getRestBlk();	// get the remaining bytes (132) of a block
	void writeChunk();

	int
	//ReceiverY::
	openFileForTransfer()
	;

    int
    //ReceiverY::
    closeTransferredFile()
    ;

	void cans();		// send CAN characters

	uint8_t
	//ReceiverY::
	checkForAnotherFile()
	;

	void
	// ReceiverY::
	tm(int timeoutUnits)
	;
    void receiveFiles();

    int closeProb;       // return errno from myClose() in closeTransferredFile() indicating error.  0 if no error.
    uint8_t anotherFile; // there is a(nother) file to receive.  reset after getting good block #1

	uint8_t NCGbyte;	// Either a 'C' (for CRC16) or a NAK (for checksum) sent by receiver to initiate transfers

	/* A Boolean variable that indicates whether the
	 *  block just received should be ACKed (true) or NAKed (false).*/
	bool goodBlk;

	/* A Boolean variable that indicates that a good copy of a block
	 *  being sent has been received for the first time.  It is an
	 *  indication that the data in a data block can be written to disk.
	 */
	bool goodBlk1st;

	/* A Boolean variable that indicates whether or not a fatal loss
	 *  of synchronization has been detected.*/
	bool syncLoss;

	/* A variable which counts the number of responses in a
	 *  row sent because of problems like communication
	 *  problems. An initial NAK (or 'C') does not add to the count. The reception
	 *  of a particular block in good condition for the first time resets the count. */
//	unsigned errCnt;	// found in PeerY.h

private:
	int relativeTm;     // relative timeout duration
	off_t bytesRemaining;   // the number of bytes remaining to be written.

	// blkT rcvBlk;		// a received block
	uint8_t rcvBlk[BLK_SZ_CRC];		// a received block

	uint8_t numLastGoodBlk; // the number of the last good block
	// ********** you can add more data members if needed *******
};

#endif
