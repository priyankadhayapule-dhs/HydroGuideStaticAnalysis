#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "thorpuck.h"

static const char* thorpuck_event_location_string(thorpuck_location_t loc)
{
    switch(loc) {
        case THORPUCK_LOC_LEFT:
            return "Left";
        case THORPUCK_LOC_RIGHT:
            return "Right";
        case THORPUCK_LOC_PALM:
            return "Palm";
        case THORPUCK_LOC_TRACKPAD:
            return "Trackpad";

        default:
            return "Unknown Location";
    }
}

static const char* thorpuck_event_type_string(thorpuck_event_type_t type)
{
    switch(type) {
        case THORPUCK_EVENT_PRESS:
            return "Press";
        case THORPUCK_EVENT_RELEASE:
            return "Release";
        case THORPUCK_EVENT_PRESS_REPEAT:
            return "Press Repeat";
        case THORPUCK_EVENT_NO_SIGNAL:
            return "No Signal";
        case THORPUCK_EVENT_GESTURE:
            return "Gesture";
        default:
            return "Unknown Event Type";
    }
}

static const char* thorpuck_gesture_string(thorpuck_gesture_t gesture)
{
    switch(gesture) {
        case THORPUCK_GESTURE_ONE_FINGER_TOUCHDOWN:
            return "One Finger Touchdown";
        case THORPUCK_GESTURE_ONE_FINGER_LIFT_OFF:
            return "One Finger Lift Off";
        case THORPUCK_GESTURE_ONE_FINGER_SINGLE_CLICK:
            return "One Finger Single Click";
        case THORPUCK_GESTURE_ONE_FINGER_DOUBLE_CLICK:
            return "One Finger Double Click";
        case THORPUCK_GESTURE_ONE_FINGER_CLICK_AND_DRAG:
            return "One Finger Click and Drag";
        case THORPUCK_GESTURE_ONE_FINGER_FLICK_UP:
            return "One Finger Flick Up";
        case THORPUCK_GESTURE_ONE_FINGER_FLICK_DOWN:
            return "One Finger Flick Down";
        case THORPUCK_GESTURE_ONE_FINGER_FLICK_RIGHT:
            return "One Finger Flick Right";
        case THORPUCK_GESTURE_ONE_FINGER_FLICK_LEFT:
            return "One Finger Flick Left";
        default:
            return "Unknown Gesture";
    }
}

static void thorpuck_event_cb(thorpuck_t* t, void* priv, thorpuck_event_t* e)
{
    if(e) {
        //special case for trackpad move since it uses x,y data
        if(e->loc == THORPUCK_LOC_TRACKPAD && e->type == THORPUCK_EVENT_GESTURE)
        {
            printf("Trackpad(%02X) - Gesture(%02X): %s(%02X)\n",
                   e->loc,
                   e->type,
                   thorpuck_gesture_string(e->gesture),
                   e->gesture);
        }
        else if(e->loc == THORPUCK_LOC_TRACKPAD)
        {
            printf("Trackpad(%02X) - %s(%02X): ( %d, %d )\n",
                   e->loc,
                   thorpuck_event_type_string(e->type),
                   e->type,
                   e->x,
                   e->y);
        }
        else
        {
            printf("%s(%02X) - %s(%02X)\n",
                   thorpuck_event_location_string(e->loc),
                   e->loc,
                   thorpuck_event_type_string(e->type),
                   e->type);
        }
    }
}

int main(int argc, char** argv)
{
    int             key = 0;
    int             qkey = 'q';

    thorpuck_t* t = thorpuck_open(NULL, thorpuck_event_cb, NULL);
    if(t) {
        do
        {
            key = getchar();
        } while (qkey != key);
        
        printf("Closing\n");
        thorpuck_close(t);
        t = NULL;
    }

    return 0;
}
