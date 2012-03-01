/*
 * Multihead support for ion3 using xrandr and xinerama
 *
 * by Bernd Wachter <bwachter@lart.info>
 *
 * based on mod_xinerama by Thomas Themel <themel0r@wannabehacker.com>
 * based on mod_xrandr by Ragnar Rova and Tuomo Valkonen
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

#ifdef HAVE_XINERAMA
#include <X11/extensions/Xinerama.h>
#endif

#ifdef HAVE_XRANDR
#include <X11/extensions/Xrandr.h>
#endif

#include <ioncore/common.h>
#include <ioncore/eventh.h>
#include <ioncore/event.h>
#include <ioncore/global.h>
#include <ioncore/mplex.h>
#include <ioncore/group-ws.h>
#include <ioncore/sizepolicy.h>
#include <ioncore/../version.h>

#ifdef MOD_MULTIHEAD_DEBUG
#include <stdio.h>
#endif

#if (!defined HAVE_XRANDR) && (!defined HAVE_XINERAMA)
#error "Neither HAVE_XRANDR nor HAVE_XINERAMA is defined"
#endif

char mod_multihead_ion_api_version[]=ION_API_VERSION;

static int event_base;
static int error_base;

#ifdef HAVE_XRANDR
static bool hasXrandr=FALSE;
#endif

bool mod_multihead_add_workspace(int x, int y, int width, int height, int cnt)
{
    WRootWin* rootWin = ioncore_g.rootwins;
    WFitParams fp;
    WMPlexAttachParams par = MPLEXATTACHPARAMS_INIT;

    WScreen* newScreen;

#ifdef MOD_MULTIHEAD_DEBUG
    printf("Screen %d:\tx=%d\ty=%d\twidth=%u\theight=%u\n",
           cnt+1, x, y, width, height);
#endif

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

    /* FIXME, maybe name it after the output for Xrandr */
    newScreen->id = cnt;
    return TRUE;
}

#ifdef HAVE_XRANDR
bool mod_multihead_handle_xrandr_event(XEvent *ev){
    if (hasXrandr && ev->type == event_base + RRScreenChangeNotify){

        return TRUE;
    }
    return FALSE;
}
#endif

bool mod_multihead_init()
{
    WRootWin* rootWin = ioncore_g.rootwins;
    Display* dpy = ioncore_g.dpy;
    int i;

#ifdef HAVE_XRANDR
    if (XRRQueryExtension(dpy, &event_base, &error_base)){
        XRRScreenResources *res;
        int offline;
#ifdef MOD_MULTIHEAD_DEBUG
        int major, minor;
        XRRQueryVersion(dpy, &major, &minor);
        printf("Using Xrandr API in version %i.%i", major, minor);
#endif

        offline=0;
        hasXrandr=TRUE;

        res = XRRGetScreenResourcesCurrent (dpy, DefaultRootWindow(dpy));

        for(i = 0 ; i < res->ncrtc ; ++i){
            XRRCrtcInfo *info = XRRGetCrtcInfo(dpy, res, res->crtcs[i]);

            /* ignore disabled CRTCs, and make sure we still have sequential */
            /* screen numbers */
            /* FIXME: do some magic for disabling used screens to get */
            /* intuitiv results */
            if (info->x==0 && info->y==0 && info->width==0 && info->height==0){
                offline++;
                continue;
            }

            if (!mod_multihead_add_workspace(info->x, info->y,
                                             info->width, info->height,
                                             i-offline)){
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
#endif
#if (defined HAVE_XRANDR) && (defined HAVE_XINERAMA)
    else
#endif
#ifdef HAVE_XINERAMA
        if(XineramaQueryExtension(dpy,&event_base, &error_base)){
            XineramaScreenInfo* sInfo;
            int nRects;

            sInfo = XineramaQueryScreens(dpy, &nRects);

            if(!sInfo){
                warn(TR("Could not retrieve Xinerama screen info, sorry."));
                return FALSE ;
            }

            for(i = 0 ; i < nRects ; ++i){
                if (!mod_multihead_add_workspace(sInfo[i].x_org, sInfo[i].y_org,
                                                 sInfo[i].width, sInfo[i].height,
                                                 i)){
                    warn(TR("Unable to create Xinerama workspace %d."), i);
                    XFree(sInfo);
                    return FALSE;
                }
            }

            XFree(sInfo);
            rootWin->scr.id = -2;
        }
#endif
#if (defined HAVE_XRANDR) || (defined HAVE_XINERAMA)
        else
#endif
            warn(TR("No Xinerama or Xrandr support detected, mod_multihead won't do anything."));


#ifdef HAVE_XRANDR
    if (hasXrandr){
        XRRSelectInput(dpy, rootWin->dummy_win, RRScreenChangeNotifyMask);
        hook_add(ioncore_handle_event_alt,(WHookDummy *)mod_multihead_handle_xrandr_event);
    }
#endif

    return TRUE;
}


bool mod_multihead_deinit(){
#ifdef HAVE_XRANDR
    hook_remove(ioncore_handle_event_alt,
                (WHookDummy *)mod_multihead_handle_xrandr_event);
#endif

    return TRUE;
}
