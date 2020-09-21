package org.dtvkit.inputsource;

import android.view.Surface;

public class Platform {
   private native boolean setNativeSurface(Surface surface);
   private native void unsetNativeSurface();
   private native int getNativeSurfaceX();
   private native int getNativeSurfaceY();
   private native int getNativeSurfaceWidth();
   private native int getNativeSurfaceHeight();

   public Boolean setSurface(Surface surface) {
      unsetNativeSurface();
      if (surface != null) {
         setNativeSurface(surface);
      }
      return true;
   }

   public int getSurfaceX()
   {
      return getNativeSurfaceX();
   }

   public int getSurfaceY()
   {
      return getNativeSurfaceY();
   }

   public int getSurfaceWidth()
   {
      return getNativeSurfaceWidth();
   }

   public int getSurfaceHeight()
   {
      return getNativeSurfaceHeight();
   }

   static {
      System.loadLibrary("platform");
   }
}
