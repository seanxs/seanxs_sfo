#pragma once
#ifndef _TRACE_H
#define _TRACE_H

#include "Common.h"

EXTERN_C_START

NTSTATUS RegEtw(OUT PREGHANDLE hEtw);
NTSTATUS UnRegEtw(IN REGHANDLE hEtw);

#define WPP_CHECK_FOR_NULL_STRING  //to prevent exceptions due to NULL strings

// {A95D3F79-ABC2-4B3C-AF4A-2D9C03968654}
static const GUID seanxs_sfo_etw =
{ 0xa95d3f79, 0xabc2, 0x4b3c, { 0xaf, 0x4a, 0x2d, 0x9c, 0x3, 0x96, 0x86, 0x54 } };


#define WPP_CONTROL_GUIDS                                              \
    WPP_DEFINE_CONTROL_GUID(seanxs_sfo_TraceGuid, (A95D3F79,ABC2,4B3C,AF4A,2D9C03968654),\
        WPP_DEFINE_BIT(TRACE_ENTRY)                              \
		WPP_DEFINE_BIT(TRACE_DIRCTRL)                              \
        WPP_DEFINE_BIT(TRACE_INSTANCE)                                   \
        WPP_DEFINE_BIT(TRACE_CREATE)                                   \
		WPP_DEFINE_BIT(TRACE_CLOSE)                                   \
        WPP_DEFINE_BIT(TRACE_CLEANUP)                                   \
        WPP_DEFINE_BIT(TRACE_QUERYINFO)                                   \
        WPP_DEFINE_BIT(TRACE_SETINFO)                                   \
        WPP_DEFINE_BIT(TRACE_UNLOAD)                                    \
		WPP_DEFINE_BIT(MYDRIVER_ALL_INFO)	\
        )                             

#define WPP_FLAG_LEVEL_LOGGER(flag, level)                                  \
    WPP_LEVEL_LOGGER(flag)

#define WPP_FLAG_LEVEL_ENABLED(flag, level)                                 \
    (WPP_LEVEL_ENABLED(flag) &&                                             \
     WPP_CONTROL(WPP_BIT_ ## flag).Level >= level)

#define WPP_LEVEL_FLAGS_LOGGER(lvl,flags) \
           WPP_LEVEL_LOGGER(flags)

#define WPP_LEVEL_FLAGS_ENABLED(lvl, flags) \
           (WPP_LEVEL_ENABLED(flags) && WPP_CONTROL(WPP_BIT_ ## flags).Level >= lvl)

//           
// WPP orders static parameters before dynamic parameters. To support the Trace function
// defined below which sets FLAGS=MYDRIVER_ALL_INFO, a custom macro must be defined to
// reorder the arguments to what the .tpl configuration file expects.
//
#define WPP_RECORDER_FLAGS_LEVEL_ARGS(flags, lvl) WPP_RECORDER_LEVEL_FLAGS_ARGS(lvl, flags)
#define WPP_RECORDER_FLAGS_LEVEL_FILTER(flags, lvl) WPP_RECORDER_LEVEL_FLAGS_FILTER(lvl, flags)

//
// This comment block is scanned by the trace preprocessor to define our
// Trace function.
//
// begin_wpp config
// FUNC Trace{FLAGS=MYDRIVER_ALL_INFO}(LEVEL, MSG, ...);
// FUNC TraceEvents(LEVEL, FLAGS, MSG, ...);
// end_wpp
//

EXTERN_C_END

#endif // !_TRACE_H

