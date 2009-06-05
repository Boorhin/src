#!/usr/bin/env python

# Copyright (C) 2009 University of Texas at Austin
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
import os, sys, tempfile
import vplot2eps
import vplot2png
import vplot2gif
import vplot2avi

pens = {
    'vpl': 'vp',
    'eps': 'ps',
    'ps': 'ps',
    'ppm': 'ppm',
    'tiff': 'tiff',
    'tif': 'tiff',
    'jpeg': 'jpeg',
    'jpg': 'jpeg',
    'png': 'png',
    'gif': 'gd',
    'mpeg': 'gd',
    'mpg': 'gd',
    'pdf': 'ps',
    'svg': 'svg',
    'avi': 'ppm'
    }

def exists(pen):
    '''check if a given pen exists'''
    bindir = os.path.join(os.environ.get('RSFROOT'),'bin')
    exe = os.path.join(bindir,pen+'pen')
    binary = os.popen('file -bi ' + exe, 'r')
    if binary.read().startswith('application'):
        return exe
    else:
        return None

def which(prog):
    '''find a program in path'''
    path = os.environ.get('PATH','')
    path = path.split(os.pathsep)
    
    for d in path:
        exe = os.path.join(d, prog)
        if os.path.isfile(exe):
            return os.path.normpath(exe)
    return None

def convert(vpl,out,format,pen,args):
    global pens
    if not os.path.isfile(vpl):
        print "\"%s\" is not a file" % vpl
        sys.exit(1)
    if not pen:
        pen = pens[format]

    exe = exists(pen)

    if not exe:
        print 'Unsupported program "%s" ' % exe

        # Offer alternatives
        convert = None
        if format == 'png':
            pens = ('png','gd','ps')
        elif format == 'gif':
            pens = ('gd','ppm')
        elif format == 'jpeg' or format == 'jpg':
            pens = ('jpeg','gd')
        elif format == 'pdf':
            pens = ('ps','pdf')
        else:
            convert = which('convert')
            if convert:
                pens = ('tiff','ppm','png','jpeg','ps')
            else:
                pens = ()

        for other in pens:
            if other != pen:
                exe = exists(other)
                if exe:
                    break
        if not exe:
            sys.exit(4)

        if convert:
            format2 = format
            if other == 'ps':
                format = 'eps'
            else:
                format = other
            out2 = out
            out = tempfile.mktemp(suffix='.'+format)

        pen = other
            
    if pen == 'gd':
        args += ' type=%s' % format

    if format == 'eps':
        vplot2png.convert(vpl,out,
                          options='color=n fat=1 fatmult=1.5 ' + args)
    elif format == 'png' and pen == 'ps':
        vplot2png.convert(vpl,out,
                          options='color=y fat=1 fatmult=1.5 ' + args)
    elif format == 'gif' and pen == 'ppm':
        vplot2gif.convert(vpl,out)
    elif format == 'avi':
        vplot2avi.convert(vpl,out)
    elif format == 'pdf' and pen == 'ps':
        eps = tempfile.mktemp(suffix='.eps')
        vplot2eps.convert(vpl,eps,
                          options='color=y fat=1 fatmult=1.5 ' + args)
        epstopdf = which('epstopdf') or which('a2ping')
        if epstopdf:
            command = 'LD_LIBRARY_PATH=%s %s %s %s --outfile=%s' % \
                (os.environ.get('LD_LIBRARY_PATH',''),
                 os.environ.get('GS_OPTIONS',''),epstopdf,eps,out)
            print command
            fail = os.system(command)
        else:
            fail = true

        os.unlink(eps)

        if fail:
            raise RuntimeError, 'Cannot convert eps to pdf' 
    else:
        # default behavior
        run = '%s %s %s > %s' % (exe,args,vpl,out)
        print run
        os.system(run)

    if convert:
        run = '%s %s %s:%s' % (convert,out,format2,out2)
        print run
        os.system(run)

if __name__ == "__main__":
    # own user interface instead of that provided by RSF's Python API
    # because this script may have users that do not have RSF

    formats = pens.keys()
    formats.sort()

    usage = '''
Usage: %s [format=] file.vpl [file2.vpl file3.vpl | file.other]

Supported formats: %s
    ''' % (sys.argv[0],' '.join(formats))

    format = None
    pen = None

    files = []
    args = []

    for arg in sys.argv[1:]:
        if '=' in arg:
            if arg[:7] == 'format=':
                format = arg[7:].lower()
            elif arg[:4] == 'pen=':
                pen = arg[4:]
            else:
                args.append(arg)
        else:
            files.append(arg)
    args = ' '.join(args)

    if format:
        if not files:
            print usage
            sys.exit(1)
        for infile in files:
            # attach format as suffix
            outfile = os.path.splitext(infile)[0]+'.'+format
    else:
        if len(files) !=2:
            print usage
            sys.exit(2)
        infile = files[0]
        outfile = files[1]
        # get format from suffix
        format = os.path.splitext(outfile)[1][1:].lower()
    
    if not format in formats:
        print 'Unknown format "%s" ' % format
        sys.exit(1)
    
    convert(infile,outfile,format,pen,args)

    sys.exit(0)
