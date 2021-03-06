c this is <seife_deriv.f>
c------------------------------------------------------------------------------
c   ($Id$)
c
c Copyright 1984 by Erhard Wielandt
c This code was part of seife.f. A current version of seife.f can be obtained
c from http://www.software-for-seismometry.de/
c
c ----
c This program is free software; you can redistribute it and/or modify
c it under the terms of the GNU General Public License as published by
c the Free Software Foundation; either version 2 of the License, or
c (at your option) any later version. 
c 
c This program is distributed in the hope that it will be useful,
c but WITHOUT ANY WARRANTY; without even the implied warranty of
c MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
c GNU General Public License for more details.
c 
c You should have received a copy of the GNU General Public License
c along with this program; if not, write to the Free Software
c Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
c ----
c
c extracted from libseife.f
c
c REVISIONS and CHANGES
c    25/10/2000   V1.0   Thomas Forbriger
c
c==============================================================================
c
      subroutine seife_deriv(nil,par,x,n,dt,msg)
c  derivative (non-recursive, non-causal, symmetric-difference)
      real*8 x(n), twodt,tau
      character par*35,msg*(*)
      logical nil
      read(par,*) tau
      if(tau.eq.0.d0) tau=1.d0
      twodt=2.d0*dt/tau
      do 1 j=1,n-2
    1 x(j)=(x(j+2)-x(j))/twodt
      do 2 j=n-1,2,-1
    2 x(j)=x(j-1)
      x(n)=x(n-1)
      write(msg,'("dif - derivative by symmetric differences")')
      nil=.false.
      return
      end
c
c ----- END OF seife_deriv.f -----
