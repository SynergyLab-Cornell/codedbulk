#!/usr/bin/env python
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 2 as
# published by the Free Software Foundation;
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
#
# Author:  Shih-Hao Tseng (shtseng@caltech.edu)
#
import sys

def deploy_common():
    # apps/shared/local_socket/
    # controller/auto_gen.sh
    # controller/build/
    # controller/executable/
    # controller/Makefile
    # controller/settings/controller_setup.cc_part
    # controller/settings/topology/topology_macros.h
    # controller/settings/traffic/*
    # controller/sources/*
    # multicast_agent/build/
    # multicast_agent/Makefile
    # multicast_agent/sources/*
    # proxy/build/
    # proxy/Makefile
    # proxy/sources/*
    # shared/build/
    # shared/Makefile
    # shared/sources/*
    pass

def deploy(exp):
    deploy_common()
    # controller/main/traffic-gen-<exp>.cc
    # controller/settings/topology/<exp>.cc_part
    # controller/workloads/<exp>-*
    # controller/<exp>_auto_gen.sh
    pass

if __name__ == '__main__':
    if len(sys.argv) < 2:
        deploy('default')
    else:
        deploy(sys.argv[1])
