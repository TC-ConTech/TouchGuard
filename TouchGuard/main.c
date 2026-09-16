//
//  main.c
//  TouchGuard
//
//  Created by SyntaxSoft 2016.
//

#include <stdio.h>
#include <sys/time.h>
#include <ApplicationServices/ApplicationServices.h>
#include <CoreFoundation/CoreFoundation.h>
#include <dispatch/dispatch.h>
#include <unistd.h>

#define MAJOR_VERSION  1
#define MINOR_VERSION  4


#define DISABLE_TAP_PB               1
#define DISABLE_DEBUG_MESSAGES       2
#define ENABLE_TAPENABLE_MESSSAGE    4
#define ENABLE_TAPDISABLE_MESSAGE    8
// 0x10 = 10 in base-16 (16 in base-10)
#define ENABLE_TAPIGNORE_MESSAGE   0x10


static int mgi_flag = 0;
static long timerInterval = 1;
static int consecutiveDisableCount = 0;
static int consecutiveIgnoreCount = 0;
static CFMachPortRef g_eventTap = NULL;

void PrintCurrentTime()
{
    struct timeval  tv;
    struct tm* tm_info;
    char ac_buffer[256];

    gettimeofday(&tv, NULL);

    tm_info = localtime(&tv.tv_sec);

    strftime(ac_buffer, sizeof(ac_buffer), "%b %d %Y %H:%M:%S", tm_info);

    printf("%s %dmsec",ac_buffer, tv.tv_usec/1000);

}

static int64_t  dispatchCount = 0;

void dispatchCallBack(void * pv_context)
{
    int64_t count = (int64_t) pv_context;

    if(count == dispatchCount)
    {
        mgi_flag &= (~(DISABLE_TAP_PB));
        int countPrinted = 0;
        if((mgi_flag&DISABLE_DEBUG_MESSAGES) == 0)
        {
            if(consecutiveDisableCount && (mgi_flag&ENABLE_TAPDISABLE_MESSAGE))
            {
                printf("DisableCount %d ", consecutiveDisableCount);
                countPrinted = 1;
            }

            if(consecutiveIgnoreCount && (mgi_flag&ENABLE_TAPIGNORE_MESSAGE))
            {
                int tapCount = (consecutiveIgnoreCount/2)-1;
                if(tapCount)
                {
                    printf("IgnoreCount %d", tapCount);
                    countPrinted = 1;
                }
            }

            if(countPrinted)
            {
                printf("\n");
            }

            consecutiveDisableCount = 0;
            consecutiveIgnoreCount = 0;
            if(mgi_flag&ENABLE_TAPENABLE_MESSSAGE)
            {
                printf("Enabled Tap \n");
            }
        }
    }
}


void dispatchDisableTap()
{
    if((mgi_flag&(DISABLE_DEBUG_MESSAGES)) == 0)
    {
        if(mgi_flag&DISABLE_TAP_PB)
        {
            ++consecutiveDisableCount;
        }
        else
        {
            if(mgi_flag&ENABLE_TAPDISABLE_MESSAGE)
            {
                printf("Disabled tap\n");
            }
        }
    }
    mgi_flag |= (DISABLE_TAP_PB);
    ++dispatchCount;
    dispatch_time_t time = dispatch_time(DISPATCH_TIME_NOW, NSEC_PER_MSEC*timerInterval);
    dispatch_after_f(time, dispatch_get_main_queue(), (void *)dispatchCount, dispatchCallBack);
}



CGEventRef eventCallBack(CGEventTapProxy proxy, CGEventType type, CGEventRef event, void *refcon)
{
    // macOS can disable an event tap (callback timeout, or on sleep/wake). When
    // it does, it delivers one of these events; re-enable the tap so TouchGuard
    // keeps working instead of silently going dead with the process still alive.
    if(type == kCGEventTapDisabledByTimeout || type == kCGEventTapDisabledByUserInput)
    {
        if(g_eventTap)
        {
            CGEventTapEnable(g_eventTap, true);
        }
        return event;
    }

    //if (type != kCGEventMouseMoved)
    if(type == kCGEventKeyUp)
    {
        dispatchDisableTap();
    }
    else if( (type == kCGEventLeftMouseDown)    || (type == kCGEventLeftMouseUp)
          || (type == kCGEventRightMouseDown)   || (type == kCGEventRightMouseUp)
          || (type == kCGEventOtherMouseDown)   || (type == kCGEventOtherMouseUp)
          || (type == kCGEventLeftMouseDragged) || (type == kCGEventRightMouseDragged)
          || (type == kCGEventOtherMouseDragged) )
    {
        // Suppress the whole accidental-touch family while armed: taps (Down/Up)
        // and the Dragged events a sliding fingertip produces (a real tap is
        // Down->Dragged->Up) - a leaked Dragged is the "click was suppressed but
        // the caret still jumped" case.
        //
        // Pointer movement (kCGEventMouseMoved) is deliberately NOT suppressed:
        // returning NULL for it does not stop the on-screen cursor on modern
        // macOS (verified on 15.7.7 - the window server moves the cursor below
        // the event-tap stream), and a moving pointer never repositions the text
        // caret on its own. Blocking the taps/drags is what actually prevents the
        // caret from jumping while typing.
        // if tap is disabled return NULL
        if(mgi_flag&DISABLE_TAP_PB)
        {
            if((mgi_flag&DISABLE_DEBUG_MESSAGES) == 0)
            {
                if(consecutiveIgnoreCount == 0)
                {
                    if(mgi_flag&ENABLE_TAPIGNORE_MESSAGE)
                    {
                        PrintCurrentTime();
                        printf(": Ignoring tap\n");
                    }
                }
                ++consecutiveIgnoreCount;
            }

            return NULL;
        }
    }


    return event;
}

int main(int argc, const char * argv[])
{
    // Line-buffer stdout so log lines flush immediately. Under the LaunchAgent
    // stdout is a file, which is block-buffered by default - that would hide
    // all output until ~4KB accumulates, making a working agent look dead.
    setvbuf(stdout, NULL, _IOLBF, 0);

    CFMachPortRef eventTap;
    CFRunLoopSourceRef eventRunLoop;
    int count = 1;

    mgi_flag |= ENABLE_TAPIGNORE_MESSAGE;

    while(count < argc)
    {
        if(strcasecmp("-nodebug", argv[count]) == 0)
        {
            mgi_flag |= DISABLE_DEBUG_MESSAGES; //bitwise or
        }
        else if(strcasecmp("-time", argv[count]) == 0)
        {
            float value = 0;
            ++count;
            if(count >= argc)
            {
                exit(1);
            }
            value = strtof(argv[count], NULL);
            if(value > 0)
            {
                // converting seconds to milliseconds
                timerInterval = value*1000;
            }
        }
        else if(strcasecmp("-version", argv[count]) == 0)
        {
            printf("Version %d.%d\n",MAJOR_VERSION,MINOR_VERSION);
        }
        else if(strcasecmp("-TapEnableMsg", argv[count]) == 0)
        {
            mgi_flag |= ENABLE_TAPENABLE_MESSSAGE;
        }
        else if(strcasecmp("-TapDisableMsg", argv[count]) == 0)
        {
            mgi_flag |= ENABLE_TAPDISABLE_MESSAGE;
        }
        count++;
    }

    if((mgi_flag&DISABLE_DEBUG_MESSAGES) == 0)
    {
        printf("Disable interval %ld milliSeconds\n",timerInterval);
    }


    // macOS 10.14+ requires Accessibility (TCC) trust to create an active
    // event tap. A process launched by launchd (e.g. a LaunchAgent at login)
    // does not inherit a terminal's trust, so request it and wait for the user
    // to grant it rather than silently failing with exit(1) (the reason
    // autostart never worked on modern macOS).
    if(!AXIsProcessTrusted())
    {
        const void *keys[]   = { kAXTrustedCheckOptionPrompt };
        const void *values[] = { kCFBooleanTrue };
        CFDictionaryRef options = CFDictionaryCreate(NULL, keys, values, 1,
            &kCFCopyStringDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
        AXIsProcessTrustedWithOptions(options);
        CFRelease(options);
        fprintf(stderr, "TouchGuard: Accessibility permission required. Enable "
            "TouchGuard under System Settings > Privacy & Security > Accessibility.\n");
        while(!AXIsProcessTrusted())
        {
            sleep(2);
        }
        fprintf(stderr, "TouchGuard: Accessibility granted; starting.\n");
    }

    eventTap = CGEventTapCreate(kCGHIDEventTap, kCGHeadInsertEventTap, kCGEventTapOptionDefault, kCGEventMaskForAllEvents, eventCallBack, NULL);

    if(!eventTap)
        exit(1);

    g_eventTap = eventTap;

    eventRunLoop = CFMachPortCreateRunLoopSource(kCFAllocatorDefault, eventTap, 0);
    CFRunLoopAddSource(CFRunLoopGetCurrent(), eventRunLoop, kCFRunLoopCommonModes);
    CGEventTapEnable(eventTap, true);


    CFRunLoopRun();

    exit(0);
}
