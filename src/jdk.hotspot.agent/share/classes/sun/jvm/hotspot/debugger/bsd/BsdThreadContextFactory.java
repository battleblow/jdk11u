/*
 * Copyright (c) 2002, 2021, Oracle and/or its affiliates. All rights reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 *
 * This code is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 only, as
 * published by the Free Software Foundation.
 *
 * This code is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * version 2 for more details (a copy is included in the LICENSE file that
 * accompanied this code).
 *
 * You should have received a copy of the GNU General Public License version
 * 2 along with this work; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Please contact Oracle, 500 Oracle Parkway, Redwood Shores, CA 94065 USA
 * or visit www.oracle.com if you need additional information or have any
 * questions.
 *
 */

package sun.jvm.hotspot.debugger.bsd;

import java.lang.reflect.*;
import sun.jvm.hotspot.debugger.*;
import sun.jvm.hotspot.debugger.bsd.aarch64.*;
import sun.jvm.hotspot.debugger.bsd.amd64.*;
import sun.jvm.hotspot.debugger.bsd.x86.*;
import sun.jvm.hotspot.debugger.bsd.ppc64.*;
import sun.jvm.hotspot.debugger.bsd.sparc.*;

class BsdThreadContextFactory {
   static ThreadContext createThreadContext(BsdDebugger dbg) {
      String cpu = dbg.getCPU();
      if (cpu.equals("x86")) {
         return new BsdX86ThreadContext(dbg);
      } else if (cpu.equals("amd64") || cpu.equals("x86_64")) {
         return new BsdAMD64ThreadContext(dbg);
      } else if (cpu.equals("sparc")) {
         return new BsdSPARCThreadContext(dbg);
      } else if (cpu.equals("ppc64")) {
          return new BsdPPC64ThreadContext(dbg);
      } else if (cpu.equals("aarch64")) {
          return new BsdAARCH64ThreadContext(dbg);
      } else {
        try {
          Class tcc = Class.forName("sun.jvm.hotspot.debugger.bsd." +
             cpu.toLowerCase() + ".Bsd" + cpu.toUpperCase() +
             "ThreadContext");
          Constructor[] ctcc = tcc.getConstructors();
          return (ThreadContext)ctcc[0].newInstance(dbg);
        } catch (Exception e) {
          throw new RuntimeException("cpu " + cpu + " is not yet supported");
        }
      }
   }
}
