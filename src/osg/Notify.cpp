/* -*-c++-*- OpenSceneGraph - Copyright (C) 1998-2006 Robert Osfield 
 *
 * This library is open source and may be redistributed and/or modified under  
 * the terms of the OpenSceneGraph Public License (OSGPL) version 0.0 or 
 * (at your option) any later version.  The full license is in LICENSE file
 * included with this distribution, and on the openscenegraph.org website.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
 * OpenSceneGraph Public License for more details.
*/
#include <osg/Notify>
#include <osg/ApplicationUsage>
#include <osg/ref_ptr>
#include <string>
#include <stdlib.h>
#include <stdio.h>
#include <sstream>
#include <iostream>

namespace osg
{

class NullStreamBuffer : public std::streambuf
{
private:
    std::streamsize xsputn(const std::streambuf::char_type *str, std::streamsize n)
    {
        return n;
    }
};

struct NullStream : public std::ostream
{
public:
    NullStream():
        std::ostream(&_buffer) {}

protected:
    NullStreamBuffer _buffer;
};

/** Stream buffer calling notify handler when buffer is synchronized (usually on std::endl).
 * Stream stores last notification severity to pass it to handler call.
 */
struct NotifyStreamBuffer : public std::stringbuf
{
    NotifyStreamBuffer() : _severity(osg::NOTICE)
    {
    }

    void setNotifyHandler(osg::NotifyHandler *handler) { _handler = handler; }
    osg::NotifyHandler *getNotifyHandler() const { return _handler.get(); }

    /** Sets severity for next call of notify handler */
    void setCurrentSeverity(osg::NotifySeverity severity) { _severity = severity; }
    osg::NotifySeverity getCurrentSeverity() const { return _severity; }

private:

    int sync()
    {
        sputc(0); // string termination
        if (_handler.valid())
            _handler->notify(_severity, pbase());
        pubseekpos(0, std::ios_base::out); // or str(std::string())
        return 0;
    }

    osg::ref_ptr<osg::NotifyHandler> _handler;
    osg::NotifySeverity _severity;
};

struct NotifyStream : public std::ostream
{
public:
    NotifyStream():
        std::ostream(&_buffer) {}

    void setCurrentSeverity(osg::NotifySeverity severity)
    {
        _buffer.setCurrentSeverity(severity);
    }

    osg::NotifySeverity getCurrentSeverity() const
    {
        return _buffer.getCurrentSeverity();
    }

protected:
    NotifyStreamBuffer _buffer;
};

}

using namespace osg;

static osg::ApplicationUsageProxy Notify_e0(osg::ApplicationUsage::ENVIRONMENTAL_VARIABLE, "OSG_NOTIFY_LEVEL <mode>", "FATAL | WARN | NOTICE | DEBUG_INFO | DEBUG_FP | DEBUG | INFO | ALWAYS");

osg::NotifySeverity g_NotifyLevel = osg::NOTICE;
osg::NullStream g_NullStream;
osg::NotifyStream g_NotifyStream;

void osg::setNotifyLevel(osg::NotifySeverity severity)
{
    osg::initNotifyLevel();
    g_NotifyLevel = severity;
}


osg::NotifySeverity osg::getNotifyLevel()
{
    osg::initNotifyLevel();
    return g_NotifyLevel;
}

void osg::setNotifyHandler(osg::NotifyHandler *handler)
{
    osg::NotifyStreamBuffer *buffer = static_cast<osg::NotifyStreamBuffer *>(g_NotifyStream.rdbuf());
    if (buffer)
        buffer->setNotifyHandler(handler);
}

osg::NotifyHandler *getNotifyHandler()
{
    osg::initNotifyLevel();
    osg::NotifyStreamBuffer *buffer = static_cast<osg::NotifyStreamBuffer *>(g_NotifyStream.rdbuf());
    return buffer ? buffer->getNotifyHandler() : 0;
}

bool osg::initNotifyLevel()
{
    static bool s_NotifyInit = false;

    if (s_NotifyInit) return true;
    
    // g_NotifyLevel
    // =============

    g_NotifyLevel = osg::NOTICE; // Default value

    char* OSGNOTIFYLEVEL=getenv("OSG_NOTIFY_LEVEL");
    if (!OSGNOTIFYLEVEL) OSGNOTIFYLEVEL=getenv("OSGNOTIFYLEVEL");
    if(OSGNOTIFYLEVEL)
    {

        std::string stringOSGNOTIFYLEVEL(OSGNOTIFYLEVEL);

        // Convert to upper case
        for(std::string::iterator i=stringOSGNOTIFYLEVEL.begin();
            i!=stringOSGNOTIFYLEVEL.end();
            ++i)
        {
            *i=toupper(*i);
        }

        if(stringOSGNOTIFYLEVEL.find("ALWAYS")!=std::string::npos)          g_NotifyLevel=osg::ALWAYS;
        else if(stringOSGNOTIFYLEVEL.find("FATAL")!=std::string::npos)      g_NotifyLevel=osg::FATAL;
        else if(stringOSGNOTIFYLEVEL.find("WARN")!=std::string::npos)       g_NotifyLevel=osg::WARN;
        else if(stringOSGNOTIFYLEVEL.find("NOTICE")!=std::string::npos)     g_NotifyLevel=osg::NOTICE;
        else if(stringOSGNOTIFYLEVEL.find("DEBUG_INFO")!=std::string::npos) g_NotifyLevel=osg::DEBUG_INFO;
        else if(stringOSGNOTIFYLEVEL.find("DEBUG_FP")!=std::string::npos)   g_NotifyLevel=osg::DEBUG_FP;
        else if(stringOSGNOTIFYLEVEL.find("DEBUG")!=std::string::npos)      g_NotifyLevel=osg::DEBUG_INFO;
        else if(stringOSGNOTIFYLEVEL.find("INFO")!=std::string::npos)       g_NotifyLevel=osg::INFO;
        else std::cout << "Warning: invalid OSG_NOTIFY_LEVEL set ("<<stringOSGNOTIFYLEVEL<<")"<<std::endl;
 
    }

    // Setup standard notify handler
    osg::NotifyStreamBuffer *buffer = dynamic_cast<osg::NotifyStreamBuffer *>(g_NotifyStream.rdbuf());
    if (buffer && !buffer->getNotifyHandler())
        buffer->setNotifyHandler(new StandardNotifyHandler);

    s_NotifyInit = true;

    return true;

}

bool osg::isNotifyEnabled( osg::NotifySeverity severity )
{
    return severity<=g_NotifyLevel;
}

std::ostream& osg::notify(const osg::NotifySeverity severity)
{
    static bool initialized = false;
    if (!initialized) 
    {
        initialized = osg::initNotifyLevel();
    }

    if (severity<=g_NotifyLevel)
    {
        g_NotifyStream.setCurrentSeverity(severity);
        return g_NotifyStream;
    }
    return g_NullStream;
}

void osg::StandardNotifyHandler::notify(osg::NotifySeverity severity, const char *message)
{
    if (severity <= osg::WARN) 
        fputs(message, stderr);
    else
        fputs(message, stdout);
}

#if defined(WIN32) && !defined(__CYGWIN__)

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

void osg::WinDebugNotifyHandler::notify(osg::NotifySeverity severity, const char *message)
{
    OutputDebugStringA(message);
}

#endif
