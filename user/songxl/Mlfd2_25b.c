/* 2-D Fourth-order Optimized Finite-difference wave extrapolation */
/*
  Copyright (C) 2008 University of Texas at Austin
  
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
  
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/
#include <rsf.h>
#include <math.h>
#include <limits.h>
#include "abcpass2.h"
#include "srcsm.h"
int main(int argc, char* argv[]) 
{
    int nx, nt, ix, it, nz, iz, isx, isz, nxz, na;
    int nbl, nbr, nbt, nbb;
    int nxb, nzb, jm;
    float cl, cr, ct, cb;
    float dt, dx, dz, ox, oz;
    float **new,  **old,  **cur, *wav;
    float ***B;  
    int len;
    int *s1, *s2, is;
    float *fs1, *fs2;
    sf_file out, vel, source, G, files1, files2;
    sf_init(argc,argv);
    out = sf_output("out");
    vel = sf_input("vel");   /* velocity */
    source = sf_input("in");   /* source wavlet*/
    G = sf_input("G");   /* source wavlet*/
    files1 = sf_input("s1");   /* source wavlet*/
    files2 = sf_input("s2");   /* source wavlet*/

//    if (SF_FLOAT != sf_gettype(inp)) sf_error("Need float input");
    if (SF_FLOAT != sf_gettype(vel)) sf_error("Need float input");
    if (SF_FLOAT != sf_gettype(source)) sf_error("Need float input");
    if (!sf_histint(vel,"n1",&nz)) sf_error("No n1= in input");
    if (!sf_histfloat(vel,"d1",&dz)) sf_error("No d1= in input");
    if (!sf_histfloat(vel,"o1",&oz)) oz=0.0;
    if (!sf_histint(vel,"n2",&nx)) sf_error("No n2= in input");
    if (!sf_histfloat(vel,"d2",&dx)) sf_error("No d2= in input");
    if (!sf_histfloat(vel,"o2",&ox)) ox=0.0;
    if (!sf_getfloat("dt",&dt)) sf_error("Need dt input");
    if (!sf_getint("nt",&nt)) sf_error("Need nt input");
    if (!sf_getint("isx",&isx)) sf_error("Need isx input");
    if (!sf_getint("isz",&isz)) sf_error("Need isz input");
    if (!sf_histint(G,"n1",&nxz)) sf_error("No n1= in input");
    if (!sf_histint(G,"n2",&na)) sf_error("No n2= in input");
    //if (nx*nz != nxz) sf_error("nx*nz != nxz");
    if (!sf_histint(files1,"n1",&len)) sf_error("No n1= in input");

    if (!sf_getint("nbt",&nbt)) nbt=100;
    if (!sf_getint("nbb",&nbb)) nbb=100;
    if (!sf_getint("nbl",&nbl)) nbl=100;
    if (!sf_getint("nbr",&nbr)) nbr=100;

    if (!sf_getfloat("ct",&ct)) ct = 0.002; /*decaying parameter*/
    if (!sf_getfloat("cb",&cb)) cb = 0.002; /*decaying parameter*/
    if (!sf_getfloat("cl",&cl)) cl = 0.002; /*decaying parameter*/
    if (!sf_getfloat("cr",&cr)) cr = 0.002; /*decaying parameter*/
    if (!sf_getint("jm",&jm)) jm=10;


    sf_warning("len=%d",len);

    sf_putint(out,"n1",nz);
    sf_putfloat(out,"d1",dz);
    sf_putfloat(out,"o1",oz); 
    sf_putint(out,"n2",nx);
    sf_putfloat(out,"d2",dx);
    sf_putfloat(out,"o2",ox); 
    sf_putint(out,"n3",(nt-1)/jm+1);
    sf_putfloat(out,"d3",dt);
    sf_putfloat(out,"o3",0.0); 

    sf_warning("nt=%d",nt);
    wav    =  sf_floatalloc(nt);
    sf_floatread(wav,nt,source);
    sf_warning("dt=%g",dt);

    nzb = nz + nbt + nbb;
    nxb = nx + nbl + nbr;
    sf_warning("nzb=%d",nzb);
    sf_warning("nxb=%d",nxb);
    sf_warning("nxz=%d",nxz);

    if (nxb*nzb != nxz) sf_error("nx*nz != nxz");

    old    =  sf_floatalloc2(nzb,nxb);
    cur    =  sf_floatalloc2(nzb,nxb);
    new    =  sf_floatalloc2(nzb,nxb);

    B   =  sf_floatalloc3(nzb,nxb,len);
    sf_floatread(B[0][0],nzb*nxb*len,G);

    fs1    =  sf_floatalloc(len);
    fs2    =  sf_floatalloc(len);
    s1    =  sf_intalloc(len);
    s2    =  sf_intalloc(len);
    sf_floatread(fs1,len,files1);
    sf_floatread(fs2,len,files2);
    for (ix=0; ix < len; ix++) {s1[ix] = (int) fs1[ix];}
    for (ix=0; ix < len; ix++) {s2[ix] = (int) fs2[ix];}
    free(fs1); free(fs2);

    for (ix=0; ix < nxb; ix++) {
        for (iz=0; iz < nzb; iz++) {
            cur[ix][iz] = 0.0;
            old[ix][iz] = 0.0; 
        }
    }
    srcsm_init(dx,dz);
    /* propagation in time */
    bd_init(nx,nz,nbt,nbb,nbl,nbr,ct,cb,cl,cr);
    for (it=0; it < nt; it++) {
        if(it<300) {
          cur[isx+nbl][isz+nbt] += wav[it];
          source_smooth(cur,isx+nbl,isz+nbt,wav[it]);
        }
        if(!(it%jm)) {
          for (ix=nbl; ix < nx+nbl; ix++) {
             sf_floatwrite(cur[ix]+nbt,nz,out);
           }
        } 
        for (ix=0; ix < nxb; ix++) {
            for (iz=0; iz < nzb; iz++) {
                  new[ix][iz] = 0.0; 
                }
         }  

	 for (ix=4; ix < nxb-4; ix++) {  
	     for (iz=4; iz < nzb-4; iz++) {  
                 for (is=0; is < len; is++) {
                     new[ix][iz]  += 0.5*(cur[ix+s2[is]][iz+s1[is]]+cur[ix-s2[is]][iz-s1[is]])*B[is][ix][iz];
                 }
             }
         }
	 for (ix=0; ix < nxb; ix++) {  
	     for (iz=0; iz < nzb; iz++) {  
                 new[ix][iz]  -= old[ix][iz];
                 old[ix][iz] = cur[ix][iz];
                 cur[ix][iz] = new[ix][iz];
             }
         }  
         //bd_decay(new);
         bd_decay(cur);
         bd_decay(old);
         
                 
    }
    exit(0); 
}           
           
