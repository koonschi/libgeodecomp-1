#ifndef _libgeodecomp_io_ioexception_h_
#define _libgeodecomp_io_ioexception_h_

#include <cstring>
#include <stdexcept>

namespace LibGeoDecomp {

/**
 * exception class which is suitable for all I/O errors. More specific
 * error conditions can be represented as subclasses.
 */
class IOException : public std::runtime_error
{
public:
    /**
     * Initializes a new IOException object which means that an error described
     * by @a msg occured during the processing of @a file, resulting in an OS
     * error described by @a error (usually from 'errno'). If @a error is left
     * out, no OS error code will be displayed. The @a fatal flag indicates if
     * this error is regarded as reason for program termination.
     */
    IOException(std::string msg, 
                std::string file, 
                int error = 0, 
                bool fatal = true) :
        std::runtime_error(msg), 
        myFile(file), 
        myError(error), 
        myFatal(fatal) 
    {}

    virtual ~IOException() throw ()
    {}
    
    virtual std::string file()
    {
        return myFile;
    }


    virtual int error()
    {
        return myError;
    }


    virtual bool fatal()
    {
        return myFatal;
    }

    /**
     * Generate a human readable error description suitable for stderr output.
     */
    virtual std::string toString()
    {
        std::string out(std::string(what()) + " `" + myFile + "'");
        if (myError != 0) {
            out += std::string(": ") + strerror(myError);
        }
        return out;
    }

private:
    std::string myFile;
    int myError;
    bool myFatal;
};


/**
 * More specific error class for errors during file opening.
 */
class FileOpenException : public IOException
{
public:
    FileOpenException(
        std::string msg, std::string file, int error = 0, bool fatal = true) : 
        IOException(msg, file, error, fatal) 
    {}
};


/**
 * More specific error class for errors during writing.
 */
class FileWriteException : public IOException
{
public:
    FileWriteException(
        std::string msg, std::string file, int error = 0, bool fatal = true) : 
        IOException(msg, file, error, fatal) 
    {}
};


/**
 * More specific error class for errors during reading.
 */
class FileReadException : public IOException
{
public:
    FileReadException(
        std::string msg, std::string file, int error = 0, bool fatal = true) : 
        IOException(msg, file, error, fatal) 
    {}
};

}

#endif
