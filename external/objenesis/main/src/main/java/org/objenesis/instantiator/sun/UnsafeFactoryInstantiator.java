/**
 * Copyright 2006-2017 the original author or authors.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
package org.objenesis.instantiator.sun;

import org.objenesis.ObjenesisException;
import org.objenesis.instantiator.ObjectInstantiator;
import org.objenesis.instantiator.annotations.Instantiator;
import org.objenesis.instantiator.annotations.Typology;

import java.lang.reflect.Field;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;

/**
 * Instantiates an object, WITHOUT calling it's constructor, using
 * sun.misc.Unsafe.allocateInstance(). Unsafe and its methods are implemented by most
 * modern JVMs.
 *
 * @author Henri Tremblay
 * @see ObjectInstantiator
 */
@SuppressWarnings("restriction")
@Instantiator(Typology.STANDARD)
public class UnsafeFactoryInstantiator<T> implements ObjectInstantiator<T> {

   private static Object unsafe;
   private static Method allocateInstance;
   private final Class<T> type;

   public UnsafeFactoryInstantiator(Class<T> type) {
      if (unsafe == null) {
         Class<?> unsafeClass;
         try {
             unsafeClass = Class.forName("sun.misc.Unsafe");
         } catch (ClassNotFoundException e) {
             throw new ObjenesisException(e);
         }
         Field f;
         try {
            f = unsafeClass.getDeclaredField("theUnsafe");
         } catch (NoSuchFieldException e) {
            throw new ObjenesisException(e);
         }
         f.setAccessible(true);
         try {
            unsafe = f.get(null);
         } catch (IllegalAccessException e) {
            throw new ObjenesisException(e);
         }
         try {
             allocateInstance = unsafeClass.getMethod("allocateInstance", Class.class);
         } catch (NoSuchMethodException e) {
             throw new ObjenesisException(e);
         }
      }
      this.type = type;
   }

   public T newInstance() {
      try {
         return type.cast(allocateInstance.invoke(unsafe, type));
      } catch (InvocationTargetException e) {
         throw new ObjenesisException(e.getCause());
      } catch (Exception e) {
         throw new ObjenesisException(e);
      }
   }
}
