//============================================================================
// File Name   : myIO.cpp
// Description : Wrapper I/O functions
//============================================================================

#include <unistd.h>			// for read/write/close
#include <fcntl.h>			// for open/creat
#include <sys/socket.h> 		// for socketpair
#include <stdarg.h>

#include "SocketReadcond.h"

int myOpen(const char *pathname, int flags, ...) //, mode_t mode)
{
    mode_t mode = 0;
    // in theory we should check here whether mode is needed.
    va_list arg;
    va_start (arg, flags);
    mode = va_arg (arg, mode_t);
    va_end (arg);
	return open(pathname, flags, mode);
}

int myCreat(const char *pathname, mode_t mode)
{
	return creat(pathname, mode);
}

int mySocketpair( int domain, int type, int protocol, int des_array[2] )
{
    int returnVal = socketpair(domain, type, protocol, des_array);
    return returnVal;
}

ssize_t myRead( int des, void* buf, size_t nbyte )
{
	return read(des, buf, nbyte );
}

ssize_t myWrite( int des, const void* buf, size_t nbyte )
{
	return write(des, buf, nbyte );
}

int myClose( int des )
{
	return close(des);
}

int myTcdrain(int des)
{ //is also included for purposes of the course.
	return 0;
}

/* Arguments:
des
    The file descriptor associated with the terminal device that you want to read from.
buf
    A pointer to a buffer into which readcond() can put the data.
n
    The maximum number of bytes to read.
min, time, timeout
    When used in RAW mode, these arguments override the behavior of the MIN and TIME members of the terminal's termios structure. For more information, see...
 *
 *  https://developer.blackberry.com/native/reference/core/com.qnx.doc.neutrino.lib_ref/topic/r/readcond.html
 *
 *  */
int myReadcond(int des, void * buf, int n, int min, int time, int timeout)
{
	return wcsReadcond(des, buf, n, min, time, timeout );
}

