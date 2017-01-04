#pragma once
#include "el_plugin_api.h"
extern pELHost dll_host; 
extern pELUnitConverter dll_uc;
extern pELLayout dll_layout;
extern const ELGenericFunctionTable *dll_gen;
extern const ELThemeColors *dll_theme;
extern pELCANSniffer dll_cana;
extern pELCANSniffer dll_canb;
extern const ELLogInfo *dll_log;
extern const ELHostInfo *dll_host_info;
extern pELDialogs dll_dialogs;
extern pELTarget dll_target;
extern pELModeler dll_model;
extern pEL3DData dll_3d;

#include <string>

#ifndef CPP_11
// Need to supply own version of recursive mutex and lock guard.
namespace std {
    template<class _Mutex>
    struct lock_guard {  // class with destructor that unlocks mutex
        typedef _Mutex mutex_type;

        explicit lock_guard(_Mutex& _Mtx) : _MyMutex(_Mtx) { // construct and lock
            _MyMutex.lock();
        }

        ~lock_guard() {// unlock
            _MyMutex.unlock();
        }

    private:
        _Mutex& _MyMutex;
    };

    class recursive_mutex {
    public: /* typedefs */
        typedef PCRITICAL_SECTION native_handle_type;
        native_handle_type native_handle() { return &m_critical_section; }

    public:
        recursive_mutex() {
            memset(&m_critical_section, 0x00, sizeof(m_critical_section));
            InitializeCriticalSection(&m_critical_section);
        }
        ~recursive_mutex() {
            DeleteCriticalSection(&m_critical_section);
        }

        void lock() { EnterCriticalSection(&m_critical_section); }
        bool try_lock() { return TryEnterCriticalSection(&m_critical_section) != 0; }
        void unlock() { LeaveCriticalSection(&m_critical_section); }

    private:
        CRITICAL_SECTION m_critical_section;
    };
}
#else
# include <mutex>
#endif

#include <iomanip>

// Used by the hex_upper iomanipulator function
struct hex_upper_manip { const int w; };

// Custom ostream manipulator for displaying hex numbers as uppercase and zero-padded
hex_upper_manip hex_upper(int width);
ostream &operator<<(ostream &out, const hex_upper_manip& h);

/***
brightness
This function will take a standard color, and return how bright it is.
Returns:
float - A value between 0 and 1, where 0 is fully dark, and 1 is fully light.
***/
float brightness(COLORREF bgr);

/***
color_at
This function is just helpful is taking the large nest of calls from host structures and returning a single color.

Parameters:
i - Formatting information of the channel
cg - The color gradient, ranges used by the channel.
f - The current value of the channel.

Returns:
COLOREF - The standard color mapped to the value.
***/
COLORREF color_at(const ELChannelFormattingInfo &i, const ELColorGradient &cg, float f);

/***
print
This function will take a value and return a string with the proper formatting based off options set by a user/host.

Parameters:
nf - The formatting options to apply.  Generally set by a user using the host...
f - The value.

Returns:
std::string - A standard string with the formatting applied.
***/
//extern std::string print(const ELNumberFormat &nf, float f);
/***
scan
This function will take in a standard string with host style formatting and return the corresponding float value.

Parameter:
str - The standard string to convert.

Returns:
float - The value of the converted string.
***/
//extern float scan(const std::string &str);
/***
flags
This function is just a faster way to check if a certain status applies to a channel.

Parameters:
i - The channel we are checking, i is just smaller to type...
flag - The actual flag that is to be checked.

Returns:
bool - true iff the flag is set in the channel, false otherwise.
***/
bool flags(pELChannelInfo i, CHANNELFLAGS flag);

/***
type
This function is a quick way to determine if the channel is of a given type. Needed as the type field in
the channel has many possible settings.

Parameter:
i - The channel being checked.
type - The type to look for.

Returns:
bool - true iff the channel is of the given type, false otherwise.
***/
bool type(pELChannelInfo i, CHANNELTYPE type);

/***
logged
This function is a quick way to determine if the host has log data for the channel.

Parameters:
i - The channel to check.

Returns:
bool - true iff the host has logging information about the given channel. false otherwise.
***/
bool logged(pELChannelInfo i);

/***
in_log
Similar to logged above, this function sees if the host is currently logging this channel or being logged.

Parameters:
i - The channel to check.

Returns:
bool - true if the host is logging the channel or if the host has log data for the channel. false otherwise.
***/
bool in_log(ELChannelInfo &i);

/***
live
This function is used to check if the log on the host is current set to display live data.

Parameters:
i - The log information as known by the host.

Returns:
bool - true iff the host is set to display live data. false otherwise.
***/
bool live(const ELLogInfo *i);

/***
adjust_name
Due to units being changeable (for display reasons) by the user, this function is used to adjust a given channel
to display the proper units.  Keeping everything uniform.

Parameters:
info - The channel whose name we are adjusting.
which - The part of the channel we are adjusting.  PRIMARY being the actual channel.
string - A buffer that can be easily reused to hold the new display name.
out - The standard string we are setting, or appending, the new display name.
append - A flag to either append, or set, the new display name.
***/
void adjust_name(ELChannelInfo &info, INFOSELECT which, pELString &string, std::string &out, bool append);

/***
move
This funny function is used to adjust a gradient to the values that match the display unit of a channel.
A conversion is called for each value, the host supplies the new value.

Parameters:
src - The essential information about a channel.
dest - The gradient that will obtain the changes.
***/
void move(const ELChannelCommonInfo &src, ELColorGradient &dest);

/***
move
This other funny function does the opposite of its sister function.  This will take a converted gradient and set the channels
information to be that of the conversion.  Used mainly because layouts do not store display unit information.

Parameters:
src - The converted gradient we are unconverting?
dest - The essential channel information we are restoring.
***/
void move(const ELColorGradient &src, ELChannelCommonInfo &dest);
