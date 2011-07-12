/*
 * Ion xinerama module
 * based on mod_xrandr by Ragnar Rova and Tuomo Valkonen
 *
 * by Thomas Themel <themel0r@wannabehacker.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License,or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not,write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */


#include <X11/Xlib.h>
#include <X11/extensions/Xinerama.h>
#include <X11/extensions/Xrandr.h>
#include <ioncore/common.h>
#include <ioncore/global.h>
#include <ioncore/mplex.h>
#include <ioncore/group-ws.h>
#include <ioncore/sizepolicy.h>
#include <ioncore/../version.h>

#ifdef MOD_XINERAMA_DEBUG
#include <stdio.h>
#endif

char mod_xinerama_ion_api_version[]=ION_API_VERSION;

static int xinerama_event_base;
static int xinerama_error_base;

bool mod_xinerama_add_workspace(int x, int y, int width, int height, int cnt)
{
    WRootWin* rootWin = ioncore_g.rootwins;
    WFitParams fp;
    WMPlexAttachParams par = MPLEXATTACHPARAMS_INIT;

    WScreen* newScreen;
    WRegion* reg=NULL;

    fp.g.x = x;
    fp.g.y = y;
    fp.g.w = width;
    fp.g.h = height;
    fp.mode = REGION_FIT_EXACT;

    par.flags = MPLEX_ATTACH_GEOM|MPLEX_ATTACH_SIZEPOLICY|MPLEX_ATTACH_UNNUMBERED ;
    par.geom = fp.g;
    par.szplcy = SIZEPOLICY_FULL_EXACT;

    newScreen = (WScreen*) mplex_do_attach_new(&rootWin->scr.mplex, &par,
                                               (WRegionCreateFn*)create_screen, NULL);

    if(newScreen == NULL) return FALSE;

    // FIXME, maybe name it after the output for Xrandr
    newScreen->id = cnt;
    return TRUE;
}


bool mod_xinerama_init()
{
    WRootWin* rootWin = ioncore_g.rootwins;
    Display* dpy = ioncore_g.dpy;
    int scrNum,nScreens;
    XineramaScreenInfo* sInfo;
    WScreen* new;

    int nRects;
    int i;
    int major, minor;

    if (XRRQueryVersion(dpy, &major, &minor))
    {
#ifdef MOD_XINERAMA_DEBUG
        printf("Using mod_xrandr in version %i.%i", major, minor);
#endif
        XRRScreenResources *res = XRRGetScreenResourcesCurrent (dpy, DefaultRootWindow(dpy));

        for(i = 0 ; i < res->ncrtc ; ++i){
            XRRCrtcInfo *info = XRRGetCrtcInfo(dpy, res, res->crtcs[i]);
#ifdef MOD_XINERAMA_DEBUG
            printf("Screen %d:\tx=%d\ty=%d\twidth=%u\theight=%u\n",
                   i+1, info->x, info->y, info->width, info->height);
#endif

            if (!mod_xinerama_add_workspace(info->x, info->y,
                                            info->width, info->height,
                                            i))
            {
                warn(TR("Unable to create Xrandr workspace %d."), i);
                XRRFreeCrtcInfo(info);
                XRRFreeScreenResources(res);
                return FALSE;
            }
            XRRFreeCrtcInfo(info);
        }

        XRRFreeScreenResources(res);
        rootWin->scr.id = -2;
    }
    else if(XineramaQueryExtension(dpy,&xinerama_event_base, &xinerama_error_base))
    {
        XineramaScreenInfo* sInfo;
        sInfo = XineramaQueryScreens(dpy, &nRects);

        if(!sInfo)
        {
            warn(TR("Could not retrieve Xinerama screen info, sorry."));
            return FALSE ;
        }

        for(i = 0 ; i < nRects ; ++i)
        {
#ifdef MOD_XINERAMA_DEBUG
            printf("Rectangle #%d: x=%d y=%d width=%u height=%u\n",
                   i+1, sInfo[i].x_org, sInfo[i].y_org, sInfo[i].width,
                   sInfo[i].height);
#endif
            if (!mod_xinerama_add_workspace(sInfo[i].x_org, sInfo[i].y_org,
                                            sInfo[i].width, sInfo[i].height,
                                            i))
            {
                warn(TR("Unable to create Xinerama workspace %d."), i);
                XFree(sInfo);
                return FALSE;
            }
        }

        XFree(sInfo);
        rootWin->scr.id = -2;
    }
    else
        warn(TR("No Xinerama support detected, mod_xinerama won't do anything."));

    return TRUE;
}


bool mod_xinerama_deinit()
{
    return TRUE;
}
