#ifndef TRACE_LOGGER_HH
#define TRACE_LOGGER_HH


#include <climits>
#if (__cplusplus >= 201103L)
#   include <cstdint>
#else
extern "C"
{
#   include <stdint.h>
}
#endif

#include <cstdarg>
#include <cstdio>

#include <exception>


namespace khmer
{


// TODO: Move to a more generic location.
struct InvalidStreamBuffer : public std:: exception
{ };


struct TraceLogger
{
    
    enum
    {
	TLVL_ALL	= 0,
	TLVL_DEBUG9, TLVL_DEBUG8, TLVL_DEBUG7, TLVL_DEBUG6, TLVL_DEBUG5,
	TLVL_DEBUG4, TLVL_DEBUG3, TLVL_DEBUG2, TLVL_DEBUG1, TLVL_DEBUG0,
	TLVL_INFO9, TLVL_INFO8, TLVL_INFO7, TLVL_INFO6, TLVL_INFO5,
	TLVL_INFO4, TLVL_INFO3, TLVL_INFO2, TLVL_INFO1, TLVL_INFO0,
	TLVL_WARNING	= 30,
	TLVL_ERROR	= 40,
	TLVL_CRITICAL	= 50,
	TLVL_NONE	= 255
    };
    
    TraceLogger( uint8_t const level, FILE * stream_handle );
    TraceLogger(
	uint8_t const level, char const * const file_name_format, ...
    );
    ~TraceLogger( );

    inline void	    operator( )(
	uint8_t const level, char const * const format, ...
    ) const
#ifdef WITH_INTERNAL_TRACING
    {
	va_list varargs;
	
	if (_level <= level)
	{
	    va_start( varargs, format );
	    vfprintf( _stream_handle, format, varargs );
	    va_end( varargs );
	    fflush( _stream_handle );
	}

    }
#else	// WITH_INTERNAL_TRACING
    { }
#endif	// !WITH_INTERNAL_TRACING

private:
    
    uint8_t	    _level;
    bool	    _shared_stream;
    FILE *	    _stream_handle;

};



} // namespace khmer


#endif // TRACE_LOGGER_HH

// vim: set ft=cpp sts=4 sw=4 tw=80:
